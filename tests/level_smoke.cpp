#include <cmath>
#include <filesystem>
#include <fstream>
#include <machina/level_description.hpp>
#include <machina/materialx_shader_generator.hpp>
#include <machina/renderer.hpp>
#include <machina/usd_level_loader.hpp>
#include <print>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <string_view>
#include <vector>

namespace {

int
fail(std::string_view message)
{
  std::println("{}", message);
  return 1;
}

bool
near(float left, float right)
{
  return std::fabs(left - right) <= 0.0001F;
}

float
dot(const machina::Vec3& left, const machina::Vec3& right)
{
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

float
length(const machina::Vec3& value)
{
  return std::sqrt(dot(value, value));
}

bool
same(const machina::Vec3& left, const machina::Vec3& right)
{
  return near(left.x, right.x) && near(left.y, right.y) &&
         near(left.z, right.z);
}

bool
allTriangleCornerNormalsMatch(const machina::MeshDescription& mesh,
                              float minimumDot)
{
  for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
    const machina::MeshVertex& first = mesh.vertices[mesh.indices[index]];
    const machina::MeshVertex& second = mesh.vertices[mesh.indices[index + 1]];
    const machina::MeshVertex& third = mesh.vertices[mesh.indices[index + 2]];

    if (dot(first.normal, second.normal) < minimumDot ||
        dot(first.normal, third.normal) < minimumDot) {
      return false;
    }
  }

  return true;
}

bool
hasSplitTriangleNormals(const machina::MeshDescription& mesh, float maximumDot)
{
  for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
    const machina::MeshVertex& first = mesh.vertices[mesh.indices[index]];
    const machina::MeshVertex& second = mesh.vertices[mesh.indices[index + 1]];
    const machina::MeshVertex& third = mesh.vertices[mesh.indices[index + 2]];

    if (dot(first.normal, second.normal) < maximumDot ||
        dot(first.normal, third.normal) < maximumDot) {
      return true;
    }
  }

  return false;
}

std::filesystem::path
sampleScenePath()
{
  return std::filesystem::current_path() / MACHINA_ASSETS_ROOT / "scenes" /
         "suzannes.usda";
}

std::filesystem::path
normalInterpolationScenePath()
{
  const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                     "machina_normal_interpolation.usda";
  std::ofstream scene(path);
  scene << R"usda(#usda 1.0
(
    defaultPrim = "root"
    metersPerUnit = 1
    upAxis = "Y"
)

def Xform "root"
{
    def Scope "_materials"
    {
        def Material "Material_001"
        {
            token outputs:mtlx:surface.connect = </root/_materials/Material_001/Shader.outputs:surface>

            def Shader "Shader"
            {
                uniform token info:id = "ND_open_pbr_surface_surfaceshader"
                color3f inputs:base_color = (1, 1, 1)
                token outputs:surface
            }
        }
    }

    def Xform "FlatObject"
    {
        def Mesh "FlatMesh" (
            prepend apiSchemas = ["MaterialBindingAPI"]
        )
        {
            int[] faceVertexCounts = [3]
            int[] faceVertexIndices = [0, 1, 2]
            rel material:binding = </root/_materials/Material_001>
            normal3f[] normals = [(1, 0, 0), (1, 0, 0), (1, 0, 0)] (
                interpolation = "faceVarying"
            )
            point3f[] points = [(0, 0, 0), (0, 0, 1), (1, 0, 0)]
            uniform token subdivisionScheme = "none"
        }
    }

    def Xform "SmoothObject"
    {
        def Mesh "SmoothMesh" (
            prepend apiSchemas = ["MaterialBindingAPI"]
        )
        {
            int[] faceVertexCounts = [3]
            int[] faceVertexIndices = [0, 1, 2]
            rel material:binding = </root/_materials/Material_001>
            normal3f[] normals = [(0, 1, 0), (0, 0, 1), (1, 0, 0)] (
                interpolation = "faceVarying"
            )
            point3f[] points = [(2, 0, 0), (2, 0, 1), (3, 0, 0)]
            uniform token subdivisionScheme = "none"
        }
    }

    def Xform "PrimvarObject"
    {
        def Mesh "PrimvarNormalMesh" (
            prepend apiSchemas = ["MaterialBindingAPI"]
        )
        {
            int[] faceVertexCounts = [3]
            int[] faceVertexIndices = [0, 1, 2]
            rel material:binding = </root/_materials/Material_001>
            normal3f[] normals = [(0, 1, 0), (0, 1, 0), (0, 1, 0)] (
                interpolation = "faceVarying"
            )
            normal3f[] primvars:normals = [(1, 0, 0), (0, 1, 0), (0, 0, 1)] (
                interpolation = "faceVarying"
            )
            int[] primvars:normals:indices = [2, 1, 0]
            point3f[] points = [(4, 0, 0), (4, 0, 1), (5, 0, 0)]
            uniform token subdivisionScheme = "none"
        }
    }
}
)usda";
  return path;
}

}

int
main()
{
  const machina::LevelDescription level =
    machina::UsdLevelLoader().load(sampleScenePath());

  if (!level.diagnostics.empty()) {
    for (const machina::Diagnostic& diagnostic : level.diagnostics) {
      std::println("{}", diagnostic.message);
    }

    return 1;
  }

  if (level.entities.size() != 2) {
    return fail("expected two renderable entities");
  }

  if (level.materials.size() != 2) {
    return fail("expected two MaterialX materials");
  }

  if (level.meshes.size() != 2) {
    return fail("expected two mesh resources");
  }

  for (const machina::MeshDescription& mesh : level.meshes) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
      return fail("expected populated mesh data");
    }

    for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
      const machina::MeshVertex& first = mesh.vertices[mesh.indices[index]];
      const machina::MeshVertex& second =
        mesh.vertices[mesh.indices[index + 1]];
      const machina::MeshVertex& third = mesh.vertices[mesh.indices[index + 2]];

      if (length(first.normal) < 0.999F || length(first.normal) > 1.001F ||
          length(second.normal) < 0.999F || length(second.normal) > 1.001F ||
          length(third.normal) < 0.999F || length(third.normal) > 1.001F) {
        return fail("expected unit-length triangle normals");
      }
    }
  }

  const machina::MeshDescription* sampleFlatMesh = nullptr;
  const machina::MeshDescription* sampleSmoothMesh = nullptr;
  for (const machina::MeshDescription& mesh : level.meshes) {
    if (mesh.name == "Suzanne_001") {
      sampleFlatMesh = &mesh;
    } else if (mesh.name == "Suzanne") {
      sampleSmoothMesh = &mesh;
    }
  }

  if (sampleFlatMesh == nullptr || sampleSmoothMesh == nullptr) {
    return fail("expected sample flat and smooth meshes");
  }

  if (!allTriangleCornerNormalsMatch(*sampleFlatMesh, 0.9999F)) {
    return fail("expected blue sample mesh normals to stay flat per triangle");
  }

  if (!hasSplitTriangleNormals(*sampleSmoothMesh, 0.99F)) {
    return fail("expected red sample mesh normals to stay smooth per triangle");
  }

  for (const machina::EntityDescription& entity : level.entities) {
    if (entity.mesh >= level.meshes.size() ||
        entity.material >= level.materials.size()) {
      return fail("expected resolved mesh and material bindings");
    }
  }

  const machina::LevelDescription normalLevel =
    machina::UsdLevelLoader().load(normalInterpolationScenePath());

  if (!normalLevel.diagnostics.empty()) {
    for (const machina::Diagnostic& diagnostic : normalLevel.diagnostics) {
      std::println("{}", diagnostic.message);
    }

    return 1;
  }

  const machina::MeshDescription* flatMesh = nullptr;
  const machina::MeshDescription* smoothMesh = nullptr;
  const machina::MeshDescription* primvarNormalMesh = nullptr;
  for (const machina::MeshDescription& mesh : normalLevel.meshes) {
    if (mesh.name == "FlatMesh") {
      flatMesh = &mesh;
    } else if (mesh.name == "SmoothMesh") {
      smoothMesh = &mesh;
    } else if (mesh.name == "PrimvarNormalMesh") {
      primvarNormalMesh = &mesh;
    }
  }

  if (flatMesh == nullptr || smoothMesh == nullptr ||
      primvarNormalMesh == nullptr) {
    return fail("expected normal interpolation test meshes");
  }

  if (flatMesh->indices.size() != 3 || smoothMesh->indices.size() != 3 ||
      primvarNormalMesh->indices.size() != 3) {
    return fail("expected single-triangle normal test meshes");
  }

  const machina::MeshVertex& flatFirst =
    flatMesh->vertices[flatMesh->indices[0]];
  const machina::MeshVertex& flatSecond =
    flatMesh->vertices[flatMesh->indices[1]];
  const machina::MeshVertex& flatThird =
    flatMesh->vertices[flatMesh->indices[2]];
  if (!same(flatFirst.normal, flatSecond.normal) ||
      !same(flatFirst.normal, flatThird.normal)) {
    return fail("expected duplicated face-varying normals to stay flat");
  }

  if (!same(flatFirst.normal, { 1.0F, 0.0F, 0.0F }) ||
      !same(flatSecond.normal, { 1.0F, 0.0F, 0.0F }) ||
      !same(flatThird.normal, { 1.0F, 0.0F, 0.0F })) {
    return fail("expected flat face-varying normals to be preserved");
  }

  const machina::MeshVertex& smoothFirst =
    smoothMesh->vertices[smoothMesh->indices[0]];
  const machina::MeshVertex& smoothSecond =
    smoothMesh->vertices[smoothMesh->indices[1]];
  const machina::MeshVertex& smoothThird =
    smoothMesh->vertices[smoothMesh->indices[2]];
  if (same(smoothFirst.normal, smoothSecond.normal) ||
      same(smoothFirst.normal, smoothThird.normal)) {
    return fail("expected distinct face-varying normals to stay smooth");
  }

  if (!same(smoothFirst.normal, { 0.0F, 1.0F, 0.0F }) ||
      !same(smoothSecond.normal, { 0.0F, 0.0F, 1.0F }) ||
      !same(smoothThird.normal, { 1.0F, 0.0F, 0.0F })) {
    return fail("expected smooth face-varying normals to be preserved");
  }

  const machina::MeshVertex& primvarFirst =
    primvarNormalMesh->vertices[primvarNormalMesh->indices[0]];
  const machina::MeshVertex& primvarSecond =
    primvarNormalMesh->vertices[primvarNormalMesh->indices[1]];
  const machina::MeshVertex& primvarThird =
    primvarNormalMesh->vertices[primvarNormalMesh->indices[2]];
  if (!same(primvarFirst.normal, { 0.0F, 0.0F, 1.0F }) ||
      !same(primvarSecond.normal, { 0.0F, 1.0F, 0.0F }) ||
      !same(primvarThird.normal, { 1.0F, 0.0F, 0.0F })) {
    return fail("expected indexed primvars:normals to override mesh normals");
  }

  const Matrix translated = machina::raylibMatrixFromTransform({ 1.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 1.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 0.0F,
                                                                 1.0F,
                                                                 0.0F,
                                                                 2.0F,
                                                                 3.0F,
                                                                 4.0F,
                                                                 1.0F });
  const Vector3 transformed =
    Vector3Transform(Vector3{ 1.0F, 1.0F, 1.0F }, translated);
  if (!near(transformed.x, 3.0F) || !near(transformed.y, 4.0F) ||
      !near(transformed.z, 5.0F)) {
    return fail("expected row-major transforms to convert to raylib matrices");
  }

  machina::MaterialXShaderGenerator generator(std::filesystem::current_path() /
                                              MACHINA_MATERIALX_LIBRARY_ROOT);
  std::vector<machina::GeneratedShader> generatedShaders;
  for (const machina::MaterialDescription& material : level.materials) {
    const machina::ShaderGenerationResult shader = generator.generate(material);
    if (!shader.ok()) {
      for (const machina::Diagnostic& diagnostic : shader.diagnostics) {
        std::println("{}", diagnostic.message);
      }

      return 1;
    }

    if (shader.shader.vertexSource.empty() ||
        shader.shader.fragmentSource.empty()) {
      return fail("expected generated GLSL sources");
    }

    if (shader.shader.vertexSource.find("u_worldViewProjectionMatrix") ==
        std::string::npos) {
      return fail("expected generated vertex shader to expose raylib MVP");
    }

    if (shader.shader.vertexSource.find(
          "layout(location = 0) in vec3 i_position;") == std::string::npos ||
        shader.shader.vertexSource.find(
          "layout(location = 2) in vec3 i_normal;") == std::string::npos ||
        shader.shader.vertexSource.find(
          "layout(location = 4) in vec3 i_tangent;") == std::string::npos) {
      return fail("expected generated vertex shader to bind raylib VAO slots");
    }

    if (shader.shader.vertexSource.find("flat vec3 normalWorld") !=
          std::string::npos ||
        shader.shader.fragmentSource.find("flat vec3 normalWorld") !=
          std::string::npos) {
      return fail("expected generated shaders to interpolate normals");
    }

    if (shader.shader.vertexSource.find(
          "u_viewProjectionMatrix * hPositionWorld") != std::string::npos) {
      return fail("expected generated vertex shader to use raylib MVP");
    }

    if (shader.shader.fragmentSource.find("u_viewPosition") ==
        std::string::npos) {
      return fail("expected generated fragment shader to expose view position");
    }

    if (shader.shader.fragmentSource.find("u_envIrradiance") ==
        std::string::npos) {
      return fail(
        "expected generated fragment shader to expose environment lighting");
    }

    if (shader.shader.fragmentSource.find("u_lightData[") ==
        std::string::npos) {
      return fail("expected generated fragment shader to expose light data");
    }

    if (shader.shader.fragmentSource.find("material_shader_base_color") ==
        std::string::npos) {
      return fail(
        "expected generated shader to use canonical material uniforms");
    }

    if (shader.shader.fragmentSource.find(material.name + "_shader") !=
        std::string::npos) {
      return fail("expected generated shader to avoid material-specific names");
    }

    generatedShaders.push_back(shader.shader);
  }

  if (generatedShaders.size() == 2 &&
      generatedShaders[0].vertexSource != generatedShaders[1].vertexSource) {
    return fail(
      "expected matching material graphs to share vertex shader source");
  }

  return 0;
}
