#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode =
      "externFn(x,y,z) fn testFn(a,b,c) (a + b + c + externFn(a,b,c))";

  Parser parser(exampleCode);

  auto res1 = parser.parseFunctionPrototype();
  auto res2 = parser.parseFunction();
  res1->codegen();
  res2->codegen();

  printGeneratedCode();

  return 0;
}
