#pragma once

#include <filesystem>

namespace driver {
  auto compileFile(const std::filesystem::path& sourcePath) -> bool;

  [[nodiscard]] auto rewriteToOutputFile(
      const std::filesystem::path& sourceFilePath) -> std::string;
}  // namespace driver
