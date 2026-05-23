#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode =
      R"(fn testFn(a,b,c) {
      d = a 
      e = a + d
      a + b + c + e + d
   })";

  Parser parser(exampleCode);

  auto res2 = parser.parseFunction();
  res2->codegen();

  printGeneratedCode();

  emitObjectFile("test.o");

  return 0;
}
