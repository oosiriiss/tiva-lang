#pragma once

#include <expected>
#include <filesystem>
#include <vector>

namespace driver {

  struct CompilerArguments {
    std::vector<std::string> files;
  };

  auto parseArguments(int argc, char const* const* argv)
      -> std::expected<CompilerArguments, std::string>;

  auto compileFile(const std::filesystem::path& sourcePath) -> bool;

  [[nodiscard]] auto rewriteToOutputFile(
      const std::filesystem::path& sourceFilePath) -> std::string;
}  // namespace driver
