#pragma once

#include "machina/LevelDescription.hpp"

#include <filesystem>

namespace machina {

class UsdLevelLoader
{
public:
  [[nodiscard]] LevelDescription load(const std::filesystem::path& path) const;
};

}
