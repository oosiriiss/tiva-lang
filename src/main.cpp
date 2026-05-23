#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode =
      "fn testFn(a,b,c) (a + b + c)";

  Parser parser(exampleCode);

  auto res2 = parser.parseFunction();
  res2->codegen();

  printGeneratedCode();

  emitObjectFile("test.o");

  return 0;
}
