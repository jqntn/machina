#include <machina/materialx_shader_generator.hpp>
#include <machina/renderer.hpp>
#include <machina/usd_level_loader.hpp>

#include <raymath.h>

#include <cmath>
#include <filesystem>
#include <print>
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

machina::Vec3
between(const machina::Vec3& start, const machina::Vec3& end)
{
  return { end.x - start.x, end.y - start.y, end.z - start.z };
}

machina::Vec3
cross(const machina::Vec3& left, const machina::Vec3& right)
{
  return { left.y * right.z - left.z * right.y,
           left.z * right.x - left.x * right.z,
           left.x * right.y - left.y * right.x };
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

machina::Vec3
normalized(const machina::Vec3& value)
{
  const float valueLength = length(value);
  if (valueLength <= 0.0F) {
    return {};
  }

  return { value.x / valueLength,
           value.y / valueLength,
           value.z / valueLength };
}

bool
same(const machina::Vec3& left, const machina::Vec3& right)
{
  return near(left.x, right.x) && near(left.y, right.y) &&
         near(left.z, right.z);
}

std::filesystem::path
sampleScenePath()
{
  return std::filesystem::current_path() / MACHINA_ASSETS_ROOT / "scenes" /
         "suzannes" / "Untitled.usda";
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

  bool foundAuthoredCustomNormal = false;
  for (const machina::MeshDescription& mesh : level.meshes) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
      return fail("expected populated mesh data");
    }

    for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
      const machina::MeshVertex& first = mesh.vertices[mesh.indices[index]];
      const machina::MeshVertex& second =
        mesh.vertices[mesh.indices[index + 1]];
      const machina::MeshVertex& third = mesh.vertices[mesh.indices[index + 2]];
      const machina::Vec3 faceNormal =
        normalized(cross(between(first.position, second.position),
                         between(first.position, third.position)));

      if (!same(first.normal, second.normal) ||
          !same(first.normal, third.normal)) {
        return fail("expected flat-shaded triangle normals");
      }

      if (length(first.normal) < 0.999F || length(first.normal) > 1.001F) {
        return fail("expected unit-length triangle normals");
      }

      if (dot(normalized(first.normal), faceNormal) < 0.9F) {
        foundAuthoredCustomNormal = true;
      }
    }
  }

  if (!foundAuthoredCustomNormal) {
    return fail("expected authored USD normals to be preserved");
  }

  for (const machina::EntityDescription& entity : level.entities) {
    if (entity.mesh >= level.meshes.size() ||
        entity.material >= level.materials.size()) {
      return fail("expected resolved mesh and material bindings");
    }
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

    if (shader.shader.vertexSource.find("flat vec3 normalWorld") ==
          std::string::npos ||
        shader.shader.fragmentSource.find("flat vec3 normalWorld") ==
          std::string::npos) {
      return fail("expected generated shaders to use flat normal varyings");
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
