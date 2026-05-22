#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode = "1+2+3+4+5+6";

  Parser parser(exampleCode);

  auto res = parser.parseExpression();
  auto value = res->codegen();

  printGeneratedCode();

  return 0;
}
