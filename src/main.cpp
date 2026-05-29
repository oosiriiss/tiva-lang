#include <logzy/logzy.hpp>

#include "driver/compiler.hpp"

auto main() -> int {
  if (!driver::compileFile("../example/first.ti")) {
    return -1;
  }

  return 0;
}
