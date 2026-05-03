#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace machina {

struct Diagnostic
{
  std::string message;
};

struct Vec2
{
  float x = 0.0F;
  float y = 0.0F;
};

struct Vec3
{
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct MeshVertex
{
  Vec3 position;
  Vec3 normal;
  Vec2 texcoord;
};

struct MeshDescription
{
  std::string path;
  std::string name;
  std::vector<MeshVertex> vertices;
  std::vector<std::uint16_t> indices;
};

struct MaterialInput
{
  std::string name;
  std::string type;
  std::string value;
};

struct MaterialDescription
{
  std::string path;
  std::string name;
  std::string nodeCategory;
  std::string nodeType;
  std::vector<MaterialInput> inputs;
  std::array<float, 3> baseColor = { 1.0F, 1.0F, 1.0F };
};

struct EntityDescription
{
  std::string path;
  std::string name;
  std::size_t mesh = 0;
  std::size_t material = 0;
  std::array<float, 16> transform = {
    1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
    0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
  };
};

struct LevelDescription
{
  std::vector<MeshDescription> meshes;
  std::vector<MaterialDescription> materials;
  std::vector<EntityDescription> entities;
  std::vector<Diagnostic> diagnostics;

  [[nodiscard]] bool ok() const { return diagnostics.empty(); }
};

}
