#include "driver/compiler.hpp"

#include <cppli/cppli.hpp>
#include <logzy/logzy.hpp>

#include "codegen/codegen_visitor.hpp"
#include "codegen/module.hpp"
#include "core/translation_unit.hpp"
#include "cppli/help.hpp"
#include "cppli/option.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_visitor.hpp"

namespace {

  enum class OptionKey : std::uint8_t { OutputFile, Help };

  constexpr auto getOptions() -> cppli::OptionContainer<OptionKey> {
    cppli::OptionContainer<OptionKey> opts;

    using cppli::Option;

    opts.addOption(OptionKey::Help,
                   Option{.firstName = "-h",
                          .secondName = "--help",
                          .description = "Displays help message",
                          .needsValue = true});

    return opts;
  }

}  // namespace

namespace driver {

  auto parseArguments(int argc, char const* const* argv)
      -> std::expected<CompilerArguments, std::string> {
    auto options = getOptions();

    cppli::ParseResult<OptionKey> result =
        cppli::parseArguments(argc, argv, options);

    // Positionals[0] is program name, removing it.
    result.positionals.erase(result.positionals.begin());

    std::expected<CompilerArguments, std::string> out;

    if (result.options.contains(OptionKey::Help)) {
      auto helpMessage = cppli::createHelp(options, "tiva-compiler");
      std::println("{}", helpMessage);
      return std::unexpected(std::string(""));
    }

    if (result.positionals.empty()) {
      return std::unexpected(std::string(
          "No input files provided. Nothing to compile. Please provide them as "
          "positional arguments tiva-compiler file1 file2 file3"));
    }

    for (const auto& file : result.positionals) {
      out->files.emplace_back(file);
    }

    return out;
  }

  auto compileFile(const std::filesystem::path& sourcePath) -> bool {
    CompilerState compilerState;
    SemanticAnalysisVisitor semanticAnalysisVisitor;
    CodeGenVisitor codegenVisitor(&compilerState);

    auto code = TranslationUnit::fromFile(sourcePath);
    if (!code) {
      logzy::critical("Couldn't load the source file at '{}'", sourcePath);
      return false;
    }

    Parser parser(code->sourceContent);

    auto unit = parser.parseTranslationUnit();
    semanticAnalysisVisitor.visit(unit.get());
    codegenVisitor.visit(unit.get());

    logzy::info("IR before optimizations");
    compilerState.printIr();
    compilerState.runOptimizations();
    logzy::info("IR after optimizations");
    compilerState.printIr();

    std::string outputFile = rewriteToOutputFile(sourcePath);

    compilerState.emitObjectFile(outputFile);

    return true;
  }

  [[nodiscard]] auto rewriteToOutputFile(
      const std::filesystem::path& sourceFilePath) -> std::string {
    std::filesystem::path fileName = sourceFilePath.filename();
    fileName.replace_extension("o");

    return fileName.string();
  }
}  // namespace driver
