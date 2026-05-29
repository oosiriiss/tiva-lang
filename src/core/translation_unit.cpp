#include "translation_unit.hpp"

#include <fstream>

[[nodiscard]] auto TranslationUnit::fromFile(const std::filesystem::path& path)
    -> std::expected<TranslationUnit, std::string> {
  std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    return std::unexpected("Failed to open file: " + path.string());
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::string sourceCode;

  if (size > 0) {
    sourceCode.resize(size);

    if (!file.read(sourceCode.data(), size)) {
      return std::unexpected("Failed to read contents of file: " +
                             path.string());
    }
  }

  return std::expected<TranslationUnit, std::string>{
      std::in_place_t{}, std::move(path.string()), std::move(sourceCode)};
}
