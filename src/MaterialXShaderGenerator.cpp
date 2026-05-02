#include "machina/MaterialXShaderGenerator.hpp"

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace machina {
namespace {

std::string
sanitizeIdentifier(const std::string& value)
{
  std::string result;
  result.reserve(value.size());

  for (const unsigned char character : value) {
    if (std::isalnum(character) != 0 || character == '_') {
      result.push_back(static_cast<char>(character));
    } else {
      result.push_back('_');
    }
  }

  if (result.empty()) {
    return "material";
  }

  if (std::isdigit(static_cast<unsigned char>(result.front())) != 0) {
    result.insert(result.begin(), '_');
  }

  return result;
}

std::string
xmlEscaped(const std::string& value)
{
  std::string result;
  result.reserve(value.size());

  for (const char character : value) {
    switch (character) {
      case '&':
        result += "&amp;";
        break;
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '"':
        result += "&quot;";
        break;
      case '\'':
        result += "&apos;";
        break;
      default:
        result.push_back(character);
        break;
    }
  }

  return result;
}

std::string
materialDocumentXml(const MaterialDescription& material)
{
  const std::string materialName = sanitizeIdentifier(material.name);
  const std::string shaderName = materialName + "_shader";

  std::ostringstream xml;
  xml << "<?xml version=\"1.0\"?>\n";
  xml << "<materialx version=\"1.39\" colorspace=\"lin_rec709\">\n";
  xml << "  <surfacematerial name=\"" << xmlEscaped(materialName)
      << "\" type=\"material\">\n";
  xml << "    <input name=\"surfaceshader\" type=\"surfaceshader\" nodename=\""
      << xmlEscaped(shaderName) << "\" />\n";
  xml << "  </surfacematerial>\n";
  xml << "  <" << xmlEscaped(material.nodeCategory) << " name=\""
      << xmlEscaped(shaderName) << "\" type=\"" << xmlEscaped(material.nodeType)
      << "\">\n";

  for (const auto& input : material.inputs) {
    if (input.value.empty()) {
      continue;
    }

    xml << "    <input name=\"" << xmlEscaped(input.name) << "\" type=\""
        << xmlEscaped(input.type) << "\" value=\"" << xmlEscaped(input.value)
        << "\" />\n";
  }

  xml << "  </" << xmlEscaped(material.nodeCategory) << ">\n";
  xml << "</materialx>\n";

  return xml.str();
}

}

MaterialXShaderGenerator::MaterialXShaderGenerator(
  std::filesystem::path materialXRoot)
  : materialXRoot(std::move(materialXRoot))
{
}

ShaderGenerationResult
MaterialXShaderGenerator::generate(const MaterialDescription& material) const
{
  ShaderGenerationResult result;

  try {
    MaterialX::DocumentPtr libraries = MaterialX::createDocument();
    const MaterialX::FileSearchPath searchPath(materialXRoot.string());
    MaterialX::loadLibraries(
      MaterialX::FilePathVec{ MaterialX::FilePath("libraries") },
      searchPath,
      libraries);

    MaterialX::DocumentPtr document = MaterialX::createDocument();
    MaterialX::readFromXmlString(
      document, materialDocumentXml(material), searchPath);
    document->importLibrary(libraries);

    std::string validation;
    if (!document->validate(&validation)) {
      result.diagnostics.push_back({ "MaterialX validation failed for " +
                                     material.path + ": " + validation });
      return result;
    }

    std::vector<MaterialX::TypedElementPtr> renderables =
      MaterialX::findRenderableElements(document);
    if (renderables.empty()) {
      result.diagnostics.push_back(
        { "MaterialX generation found no renderable element for " +
          material.path });
      return result;
    }

    MaterialX::ShaderGeneratorPtr generator =
      MaterialX::GlslShaderGenerator::create();
    MaterialX::GenContext context(generator);
    context.registerSourceCodeSearchPath(searchPath);
    MaterialX::ShaderPtr shader = generator->generate(
      sanitizeIdentifier(material.name), renderables.front(), context);

    result.shader.vertexSource =
      shader->getSourceCode(MaterialX::Stage::VERTEX);
    result.shader.fragmentSource =
      shader->getSourceCode(MaterialX::Stage::PIXEL);

    if (result.shader.vertexSource.empty() ||
        result.shader.fragmentSource.empty()) {
      result.diagnostics.push_back(
        { "MaterialX generated empty GLSL for " + material.path });
    }
  } catch (const std::exception& exception) {
    result.diagnostics.push_back({ "MaterialX generation failed for " +
                                   material.path + ": " + exception.what() });
  }

  return result;
}

}
