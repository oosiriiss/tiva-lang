#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode = "fn testFn(a,b,c) (a + b + c)";

  Parser parser(exampleCode);

  auto res = parser.parseFunction();
  auto value = res->codegen();

  printGeneratedCode();

  return 0;
}
