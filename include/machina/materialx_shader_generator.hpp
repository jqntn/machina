#pragma once

#include <filesystem>
#include <machina/level_description.hpp>
#include <string>
#include <vector>

namespace machina {

struct GeneratedShader
{
  std::string vertexSource;
  std::string fragmentSource;
};

struct ShaderGenerationResult
{
  GeneratedShader shader;
  std::vector<Diagnostic> diagnostics;

  [[nodiscard]] bool ok() const { return diagnostics.empty(); }
};

class MaterialXShaderGenerator
{
public:
  explicit MaterialXShaderGenerator(std::filesystem::path materialXRoot);

  [[nodiscard]] ShaderGenerationResult generate(
    const MaterialDescription& material) const;

private:
  std::filesystem::path materialXRoot;
};

}
