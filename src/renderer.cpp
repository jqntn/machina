#include <machina/renderer.hpp>

#include <machina/ecs.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace machina {

Matrix
raylibMatrixFromTransform(const std::array<float, 16>& transform)
{
  // USD/Gf stores row-vector transforms; raylib expects OpenGL-style columns.
  return { transform[0], transform[4], transform[8],  transform[12],
           transform[1], transform[5], transform[9],  transform[13],
           transform[2], transform[6], transform[10], transform[14],
           transform[3], transform[7], transform[11], transform[15] };
}

namespace {

constexpr int environmentRadianceMapSlot = MATERIAL_MAP_HEIGHT;
constexpr int environmentIrradianceMapSlot = MATERIAL_MAP_BRDF;
constexpr std::string_view materialShaderName = "material_shader";

struct DrawCommand
{
  std::size_t mesh = 0;
  std::size_t material = 0;
  Matrix modelMatrix = MatrixIdentity();
};

unsigned char
colorByte(float value)
{
  return static_cast<unsigned char>(std::clamp(value, 0.0F, 1.0F) * 255.0F);
}

std::string
sanitizeIdentifier(std::string_view value)
{
  std::string result;
  result.reserve(value.size());

  for (const unsigned char character : value) {
    if (std::isalnum(character) != 0 || character == '_') {
      result.push_back(static_cast<char>(character));
    } else {
      result.push_back('_');
    }
  }

  if (result.empty()) {
    return "input";
  }

  if (std::isdigit(static_cast<unsigned char>(result.front())) != 0) {
    result.insert(result.begin(), '_');
  }

  return result;
}

std::string
shaderCacheKey(const MaterialDescription& material)
{
  std::vector<std::pair<std::string, std::string>> inputs;
  inputs.reserve(material.inputs.size());
  for (const MaterialInput& input : material.inputs) {
    inputs.emplace_back(input.name, input.type);
  }
  std::ranges::sort(inputs);

  std::string key = material.nodeCategory + '\n' + material.nodeType;
  for (const auto& [name, type] : inputs) {
    key += '\n';
    key += name;
    key += ':';
    key += type;
  }

  return key;
}

std::string
materialUniformName(const MaterialInput& input)
{
  return std::string(materialShaderName) + '_' + sanitizeIdentifier(input.name);
}

std::string
normalizedInputValue(std::string value)
{
  for (char& character : value) {
    if (character == ',' || character == '(' || character == ')') {
      character = ' ';
    }
  }

  return value;
}

bool
parseFloatValues(const std::string& value,
                 std::array<float, 4>& parsed,
                 std::size_t count)
{
  std::istringstream stream(normalizedInputValue(value));
  for (std::size_t index = 0; index < count; ++index) {
    if (!(stream >> parsed[index])) {
      return false;
    }
  }

  std::string extra;
  return !(stream >> extra);
}

bool
parseIntValue(const std::string& value, int& parsed)
{
  std::istringstream stream(normalizedInputValue(value));
  if (!(stream >> parsed)) {
    return false;
  }

  std::string extra;
  return !(stream >> extra);
}

bool
parseBoolValue(std::string value, int& parsed)
{
  for (char& character : value) {
    character =
      static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }

  if (value == "true" || value == "1") {
    parsed = 1;
    return true;
  }

  if (value == "false" || value == "0") {
    parsed = 0;
    return true;
  }

  return false;
}

bool
configureUniformValue(const MaterialInput& input,
                      UploadedMaterialUniform& uniform)
{
  if (input.type == "float") {
    uniform.uniformType = SHADER_UNIFORM_FLOAT;
    return parseFloatValues(input.value, uniform.floatValues, 1);
  }

  if (input.type == "vector2") {
    uniform.uniformType = SHADER_UNIFORM_VEC2;
    return parseFloatValues(input.value, uniform.floatValues, 2);
  }

  if (input.type == "vector3" || input.type == "color3") {
    uniform.uniformType = SHADER_UNIFORM_VEC3;
    return parseFloatValues(input.value, uniform.floatValues, 3);
  }

  if (input.type == "integer") {
    uniform.uniformType = SHADER_UNIFORM_INT;
    return parseIntValue(input.value, uniform.intValues[0]);
  }

  if (input.type == "boolean") {
    uniform.uniformType = SHADER_UNIFORM_INT;
    return parseBoolValue(input.value, uniform.intValues[0]);
  }

  return false;
}

void
configureShader(Shader& shader)
{
  const int position = GetShaderLocationAttrib(shader, "i_position");
  if (position >= 0) {
    shader.locs[SHADER_LOC_VERTEX_POSITION] = position;
  }

  const int texcoord = GetShaderLocationAttrib(shader, "i_texcoord_0");
  if (texcoord >= 0) {
    shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] = texcoord;
  }

  const int normal = GetShaderLocationAttrib(shader, "i_normal");
  if (normal >= 0) {
    shader.locs[SHADER_LOC_VERTEX_NORMAL] = normal;
  }

  const int tangent = GetShaderLocationAttrib(shader, "i_tangent");
  if (tangent >= 0) {
    shader.locs[SHADER_LOC_VERTEX_TANGENT] = tangent;
  }

  const int mvp = GetShaderLocation(shader, "u_worldViewProjectionMatrix");
  if (mvp >= 0) {
    shader.locs[SHADER_LOC_MATRIX_MVP] = mvp;
  }

  const int model = GetShaderLocation(shader, "u_worldMatrix");
  if (model >= 0) {
    shader.locs[SHADER_LOC_MATRIX_MODEL] = model;
  }

  const int normalMatrix =
    GetShaderLocation(shader, "u_worldInverseTransposeMatrix");
  if (normalMatrix >= 0) {
    shader.locs[SHADER_LOC_MATRIX_NORMAL] = normalMatrix;
  }
}

UploadedMaterial
uploadedMaterial(Material material)
{
  UploadedMaterial uploaded = {};
  uploaded.material = material;
  uploaded.viewPositionLocation =
    GetShaderLocation(material.shader, "u_viewPosition");
  uploaded.envRadianceLocation =
    GetShaderLocation(material.shader, "u_envRadiance");
  uploaded.envIrradianceLocation =
    GetShaderLocation(material.shader, "u_envIrradiance");
  uploaded.envLightIntensityLocation =
    GetShaderLocation(material.shader, "u_envLightIntensity");
  uploaded.envRadianceMipsLocation =
    GetShaderLocation(material.shader, "u_envRadianceMips");
  uploaded.envRadianceSamplesLocation =
    GetShaderLocation(material.shader, "u_envRadianceSamples");
  uploaded.activeLightCountLocation =
    GetShaderLocation(material.shader, "u_numActiveLightSources");
  uploaded.lightTypeLocation =
    GetShaderLocation(material.shader, "u_lightData[0].type");
  uploaded.lightDirectionLocation =
    GetShaderLocation(material.shader, "u_lightData[0].direction");
  uploaded.lightColorLocation =
    GetShaderLocation(material.shader, "u_lightData[0].color");
  uploaded.lightIntensityLocation =
    GetShaderLocation(material.shader, "u_lightData[0].intensity");
  return uploaded;
}

std::vector<UploadedMaterialUniform>
uploadedMaterialUniforms(const Shader& shader,
                         const MaterialDescription& material,
                         std::vector<Diagnostic>& diagnostics)
{
  std::vector<UploadedMaterialUniform> uniforms;

  for (const MaterialInput& input : material.inputs) {
    UploadedMaterialUniform uniform = {};
    uniform.location =
      GetShaderLocation(shader, materialUniformName(input).c_str());
    if (!configureUniformValue(input, uniform)) {
      diagnostics.push_back({ "Material " + material.path +
                              " has unsupported or invalid input " +
                              input.name + " of type " + input.type });
      continue;
    }

    if (uniform.location >= 0) {
      uniforms.push_back(uniform);
    }
  }

  return uniforms;
}

void
setIntUniform(const Shader& shader, int location, int value)
{
  if (location >= 0) {
    SetShaderValue(shader, location, &value, SHADER_UNIFORM_INT);
  }
}

void
setFloatUniform(const Shader& shader, int location, float value)
{
  if (location >= 0) {
    SetShaderValue(shader, location, &value, SHADER_UNIFORM_FLOAT);
  }
}

void
setVec3Uniform(const Shader& shader, int location, Vector3 value)
{
  if (location >= 0) {
    SetShaderValue(shader, location, &value, SHADER_UNIFORM_VEC3);
  }
}

void
setMaterialParameterUniforms(const UploadedMaterial& material)
{
  const Shader& shader = material.material.shader;
  for (const UploadedMaterialUniform& uniform : material.parameterUniforms) {
    if (uniform.location < 0) {
      continue;
    }

    switch (uniform.uniformType) {
      case SHADER_UNIFORM_INT:
      case SHADER_UNIFORM_IVEC2:
      case SHADER_UNIFORM_IVEC3:
      case SHADER_UNIFORM_IVEC4:
        SetShaderValue(shader,
                       uniform.location,
                       uniform.intValues.data(),
                       uniform.uniformType);
        break;
      default:
        SetShaderValue(shader,
                       uniform.location,
                       uniform.floatValues.data(),
                       uniform.uniformType);
        break;
    }
  }
}

Vector3
fallbackTangent(const Vec3& normal)
{
  const Vector3 n = Vector3Normalize(Vector3{ normal.x, normal.y, normal.z });
  const Vector3 reference = std::abs(n.y) < 0.999F
                              ? Vector3{ 0.0F, 1.0F, 0.0F }
                              : Vector3{ 1.0F, 0.0F, 0.0F };
  return Vector3Normalize(Vector3CrossProduct(reference, n));
}

Texture2D
loadNeutralEnvironmentTexture()
{
  Image image = GenImageColor(4, 2, Color{ 214, 218, 222, 255 });
  Texture2D texture = LoadTextureFromImage(image);
  UnloadImage(image);

  SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
  return texture;
}

void
setMaterialSampler(Material& material, int mapSlot, int location)
{
  if (location < 0) {
    return;
  }

  material.shader.locs[SHADER_LOC_MAP_DIFFUSE + mapSlot] = location;
  material.maps[mapSlot].texture = loadNeutralEnvironmentTexture();
}

void
configureMaterialEnvironmentMaps(UploadedMaterial& material)
{
  setMaterialSampler(material.material,
                     environmentRadianceMapSlot,
                     material.envRadianceLocation);
  setMaterialSampler(material.material,
                     environmentIrradianceMapSlot,
                     material.envIrradianceLocation);
}

void
configureStaticLightingUniforms(const UploadedMaterial& material)
{
  const Shader& shader = material.material.shader;
  const Vector3 direction =
    Vector3Normalize(Vector3{ -4.076245F, -5.903862F, 1.005454F });
  const Vector3 color = { 1.0F, 1.0F, 1.0F };
  const int envMips = 1;
  const int envSamples = 8;

  setFloatUniform(shader, material.envLightIntensityLocation, 0.4F);
  setIntUniform(shader, material.envRadianceMipsLocation, envMips);
  setIntUniform(shader, material.envRadianceSamplesLocation, envSamples);
  setIntUniform(shader, material.activeLightCountLocation, 1);
  setIntUniform(shader, material.lightTypeLocation, 1);
  setVec3Uniform(shader, material.lightDirectionLocation, direction);
  setVec3Uniform(shader, material.lightColorLocation, color);
  setFloatUniform(shader, material.lightIntensityLocation, 2.0F);
}

void
configureFrameUniforms(const UploadedMaterial& material, const Camera& camera)
{
  setVec3Uniform(
    material.material.shader, material.viewPositionLocation, camera.position);
}

bool
drawCommandLess(const std::vector<UploadedMaterial>& materials,
                const DrawCommand& left,
                const DrawCommand& right)
{
  const unsigned int leftShader = materials[left.material].material.shader.id;
  const unsigned int rightShader = materials[right.material].material.shader.id;

  if (leftShader != rightShader) {
    return leftShader < rightShader;
  }

  if (left.material != right.material) {
    return left.material < right.material;
  }

  return left.mesh < right.mesh;
}

Mesh
uploadMesh(const MeshDescription& description)
{
  Mesh mesh = {};
  mesh.vertexCount = static_cast<int>(description.vertices.size());
  mesh.triangleCount = static_cast<int>(description.indices.size() / 3);

  mesh.vertices = static_cast<float*>(
    MemAlloc(description.vertices.size() * 3 * sizeof(float)));
  mesh.normals = static_cast<float*>(
    MemAlloc(description.vertices.size() * 3 * sizeof(float)));
  mesh.tangents = static_cast<float*>(
    MemAlloc(description.vertices.size() * 4 * sizeof(float)));
  mesh.texcoords = static_cast<float*>(
    MemAlloc(description.vertices.size() * 2 * sizeof(float)));
  mesh.indices = static_cast<unsigned short*>(
    MemAlloc(description.indices.size() * sizeof(unsigned short)));

  for (std::size_t index = 0; index < description.vertices.size(); ++index) {
    const MeshVertex& vertex = description.vertices[index];
    mesh.vertices[index * 3 + 0] = vertex.position.x;
    mesh.vertices[index * 3 + 1] = vertex.position.y;
    mesh.vertices[index * 3 + 2] = vertex.position.z;
    mesh.normals[index * 3 + 0] = vertex.normal.x;
    mesh.normals[index * 3 + 1] = vertex.normal.y;
    mesh.normals[index * 3 + 2] = vertex.normal.z;
    const Vector3 tangent = fallbackTangent(vertex.normal);
    mesh.tangents[index * 4 + 0] = tangent.x;
    mesh.tangents[index * 4 + 1] = tangent.y;
    mesh.tangents[index * 4 + 2] = tangent.z;
    mesh.tangents[index * 4 + 3] = 1.0F;
    mesh.texcoords[index * 2 + 0] = vertex.texcoord.x;
    mesh.texcoords[index * 2 + 1] = vertex.texcoord.y;
  }

  std::memcpy(mesh.indices,
              description.indices.data(),
              description.indices.size() * sizeof(unsigned short));

  UploadMesh(&mesh, false);
  return mesh;
}

}

Renderer::~Renderer()
{
  for (UploadedMaterial& material : materials) {
    material.material.shader.id = rlGetShaderIdDefault();
    material.material.shader.locs = rlGetShaderLocsDefault();
    UnloadMaterial(material.material);
  }

  for (Shader& shader : shaders) {
    UnloadShader(shader);
  }

  for (Mesh& mesh : meshes) {
    UnloadMesh(mesh);
  }
}

std::vector<Diagnostic>
Renderer::load(const LevelDescription& level,
               const MaterialXShaderGenerator& generator)
{
  std::vector<Diagnostic> diagnostics;
  std::unordered_map<std::string, std::size_t> shaderCache;

  for (const MaterialDescription& materialDescription : level.materials) {
    const std::string cacheKey = shaderCacheKey(materialDescription);
    std::size_t shaderIndex = 0;
    const auto cachedShader = shaderCache.find(cacheKey);
    if (cachedShader != shaderCache.end()) {
      shaderIndex = cachedShader->second;
    } else {
      ShaderGenerationResult generated =
        generator.generate(materialDescription);
      if (!generated.ok()) {
        diagnostics.insert(diagnostics.end(),
                           generated.diagnostics.begin(),
                           generated.diagnostics.end());
        return diagnostics;
      }

      Shader shader =
        LoadShaderFromMemory(generated.shader.vertexSource.c_str(),
                             generated.shader.fragmentSource.c_str());
      if (shader.id == 0) {
        diagnostics.push_back(
          { "raylib failed to compile generated MaterialX shader for " +
            materialDescription.path });
        return diagnostics;
      }

      configureShader(shader);
      shaderIndex = shaders.size();
      shaders.push_back(shader);
      shaderCache.emplace(cacheKey, shaderIndex);
    }

    Material material = LoadMaterialDefault();
    material.shader = shaders[shaderIndex];
    material.maps[MATERIAL_MAP_DIFFUSE].color =
      Color{ colorByte(materialDescription.baseColor[0]),
             colorByte(materialDescription.baseColor[1]),
             colorByte(materialDescription.baseColor[2]),
             255 };
    UploadedMaterial uploaded = uploadedMaterial(material);
    uploaded.parameterUniforms = uploadedMaterialUniforms(
      material.shader, materialDescription, diagnostics);
    if (!diagnostics.empty()) {
      return diagnostics;
    }

    configureMaterialEnvironmentMaps(uploaded);
    configureStaticLightingUniforms(uploaded);
    materials.push_back(uploaded);
  }

  for (const MeshDescription& meshDescription : level.meshes) {
    meshes.push_back(uploadMesh(meshDescription));
  }

  return diagnostics;
}

void
Renderer::draw(entt::registry& registry, const Camera& camera) const
{
  std::vector<DrawCommand> drawCommands;

  for (const UploadedMaterial& material : materials) {
    configureFrameUniforms(material, camera);
  }

  const auto view = registry.view<const Renderable, const Transform>();
  for (const entt::entity entity : view) {
    const auto& renderable = view.get<const Renderable>(entity);
    const auto& transform = view.get<const Transform>(entity);

    if (renderable.mesh >= meshes.size() ||
        renderable.material >= materials.size()) {
      continue;
    }

    drawCommands.push_back({ renderable.mesh,
                             renderable.material,
                             raylibMatrixFromTransform(transform.world) });
  }

  std::ranges::sort(drawCommands,
                    [this](const DrawCommand& left, const DrawCommand& right) {
                      return drawCommandLess(materials, left, right);
                    });

  std::size_t activeMaterial = materials.size();
  for (const DrawCommand& command : drawCommands) {
    const UploadedMaterial& material = materials[command.material];
    if (command.material != activeMaterial) {
      setMaterialParameterUniforms(material);
      activeMaterial = command.material;
    }

    DrawMesh(meshes[command.mesh], material.material, command.modelMatrix);
  }
}

}
