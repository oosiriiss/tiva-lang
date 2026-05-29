#pragma once

#include <expected>
#include <filesystem>
#include <string>

struct TranslationUnit {
 public:
  [[nodiscard]] static auto fromFile(const std::filesystem::path& path)
      -> std::expected<TranslationUnit, std::string>;

 public:
  std::string sourcePath;
  std::string sourceContent;
};
