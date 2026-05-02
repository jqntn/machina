#include "machina/MaterialXShaderGenerator.hpp"
#include "machina/UsdLevelLoader.hpp"

#include <filesystem>
#include <print>
#include <string_view>

namespace {

int
fail(std::string_view message)
{
  std::println("{}", message);
  return 1;
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

  for (const machina::MeshDescription& mesh : level.meshes) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
      return fail("expected populated mesh data");
    }
  }

  for (const machina::EntityDescription& entity : level.entities) {
    if (entity.mesh >= level.meshes.size() ||
        entity.material >= level.materials.size()) {
      return fail("expected resolved mesh and material bindings");
    }
  }

  machina::MaterialXShaderGenerator generator(std::filesystem::current_path() /
                                              MACHINA_MATERIALX_LIBRARY_ROOT);
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
  }

  return 0;
}
