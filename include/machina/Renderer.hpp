#pragma once

#include "machina/LevelDescription.hpp"
#include "machina/MaterialXShaderGenerator.hpp"

#include <entt/entt.hpp>
#include <raylib.h>

#include <string>
#include <vector>

namespace machina {

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
  void draw(entt::registry& registry) const;

private:
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
};

}
