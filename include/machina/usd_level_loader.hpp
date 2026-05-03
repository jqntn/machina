#pragma once

#include <filesystem>
#include <machina/level_description.hpp>

namespace machina {

class UsdLevelLoader
{
public:
  [[nodiscard]] LevelDescription load(const std::filesystem::path& path) const;
};

}
