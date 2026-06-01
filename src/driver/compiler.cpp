#include "driver/compiler.hpp"

#include <logzy/logzy.hpp>

#include "codegen/codegen_visitor.hpp"
#include "codegen/module.hpp"
#include "core/translation_unit.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_visitor.hpp"

namespace driver {

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

    auto res1 = parser.parseFunction();
    semanticAnalysisVisitor.visit(res1.get());
    codegenVisitor.visit(res1.get());

    auto res2 = parser.parseFunction();
    semanticAnalysisVisitor.visit(res2.get());
    codegenVisitor.visit(res2.get());

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
