#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode =
      R"(fn testFn(a,b,c) {
      (c+b)
      a + b + c + 5 + 10
   })";

  Parser parser(exampleCode);

  auto res2 = parser.parseFunction();
  res2->codegen();

  printGeneratedCode();

  emitObjectFile("test.o");

  return 0;
}
