#include "machina/Renderer.hpp"

#include "machina/Ecs.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace machina {
namespace {

Matrix
matrixFrom(const std::array<float, 16>& value)
{
  return { value[0],  value[1],  value[2],  value[3], value[4],  value[5],
           value[6],  value[7],  value[8],  value[9], value[10], value[11],
           value[12], value[13], value[14], value[15] };
}

unsigned char
colorByte(float value)
{
  return static_cast<unsigned char>(std::clamp(value, 0.0F, 1.0F) * 255.0F);
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

  const int mvp = GetShaderLocation(shader, "u_worldViewProjectionMatrix");
  if (mvp >= 0) {
    shader.locs[SHADER_LOC_MATRIX_MVP] = mvp;
  }

  const int model = GetShaderLocation(shader, "u_worldMatrix");
  if (model >= 0) {
    shader.locs[SHADER_LOC_MATRIX_MODEL] = model;
  }
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
  for (Material& material : materials) {
    UnloadMaterial(material);
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

  for (const MaterialDescription& materialDescription : level.materials) {
    ShaderGenerationResult generated = generator.generate(materialDescription);
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
    Material material = LoadMaterialDefault();
    material.shader = shader;
    material.maps[MATERIAL_MAP_DIFFUSE].color =
      Color{ colorByte(materialDescription.baseColor[0]),
             colorByte(materialDescription.baseColor[1]),
             colorByte(materialDescription.baseColor[2]),
             255 };
    materials.push_back(material);
  }

  for (const MeshDescription& meshDescription : level.meshes) {
    meshes.push_back(uploadMesh(meshDescription));
  }

  return diagnostics;
}

void
Renderer::draw(entt::registry& registry) const
{
  const auto view = registry.view<const Renderable, const Transform>();
  for (const entt::entity entity : view) {
    const auto& renderable = view.get<const Renderable>(entity);
    const auto& transform = view.get<const Transform>(entity);

    if (renderable.mesh >= meshes.size() ||
        renderable.material >= materials.size()) {
      continue;
    }

    DrawMesh(meshes[renderable.mesh],
             materials[renderable.material],
             matrixFrom(transform.world));
  }
}

}
