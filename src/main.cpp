#include "parser/ast_nodes.hpp"
#include "parser/parser.hpp"
#include <logzy/logzy.hpp>
int main() {
  initalizeLlvmModule();

  std::string_view exampleCode =
      R"(fn testFn(a,b,c) {
      d = 5
      r = 0
      r = r + d
      {
	 d2 = d * 2
	 r = r + d2
      }
      r = r + d
      r
   })";

  Parser parser(exampleCode);

  auto res2 = parser.parseFunction();
  res2->codegen();

  printGeneratedCode();

  emitObjectFile("test.o");

  return 0;
}
