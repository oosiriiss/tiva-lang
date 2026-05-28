#include <logzy/logzy.hpp>

#include "codegen/codegen_visitor.hpp"
#include "codegen/module.hpp"
#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_visitor.hpp"

auto main() -> int {
  CompilerState compilerState;
  SemanticAnalysisVisitor semanticAnalysisVisitor;
  CodeGenVisitor codegenVisitor(&compilerState);

  std::string_view exampleCode =
      R"(fn testFn(a,b,c) {
      a + b + 6.7
   })";

  Parser parser(exampleCode);
  auto res2 = parser.parseFunction();

  semanticAnalysisVisitor.visit(res2.get());
  codegenVisitor.visit(res2.get());

  logzy::info("IR before optimizations");
  compilerState.printIr();
  compilerState.runOptimizations();
  logzy::info("IR after optimizations");
  compilerState.printIr();
  compilerState.emitObjectFile("test.o");

  return 0;
}
