#pragma once

#include <array>
#include <entt/entity/fwd.hpp>
#include <machina/level_description.hpp>
#include <machina/materialx_shader_generator.hpp>
#include <raylib.h>
#include <vector>

namespace machina {

[[nodiscard]] Matrix
raylibMatrixFromTransform(const std::array<float, 16>& transform);

struct UploadedMaterialUniform
{
  int location = -1;
  int uniformType = 0;
  std::array<float, 4> floatValues = {};
  std::array<int, 4> intValues = {};
};

struct UploadedMaterial
{
  Material material = {};
  std::vector<UploadedMaterialUniform> parameterUniforms;
  int viewPositionLocation = -1;
  int envRadianceLocation = -1;
  int envIrradianceLocation = -1;
  int envLightIntensityLocation = -1;
  int envRadianceMipsLocation = -1;
  int envRadianceSamplesLocation = -1;
  int activeLightCountLocation = -1;
  int lightTypeLocation = -1;
  int lightDirectionLocation = -1;
  int lightColorLocation = -1;
  int lightIntensityLocation = -1;
};

class Renderer
{
public:
  Renderer() = default;
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;
  ~Renderer();

  [[nodiscard]] std::vector<Diagnostic> load(
    const LevelDescription& level,
    const MaterialXShaderGenerator& generator);
  void draw(entt::registry& registry, const Camera& camera) const;

private:
  std::vector<Shader> shaders;
  std::vector<Mesh> meshes;
  std::vector<UploadedMaterial> materials;
};

}
