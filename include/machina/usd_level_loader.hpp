#pragma once

#include <machina/level_description.hpp>

#include <filesystem>

namespace machina {

class UsdLevelLoader
{
public:
  [[nodiscard]] LevelDescription load(const std::filesystem::path& path) const;
};

}
