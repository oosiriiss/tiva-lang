#include <cstdlib>
#include <logzy/logzy.hpp>

#include "driver/compiler.hpp"

auto main(int argc, char const* const* const argv) -> int {
  std::expected<driver::CompilerArguments, std::string> args =
      driver::parseArguments(argc, argv);

  if (!args) {
    const auto& errorMessage = args.error();

    // Empty error message means that during parsing terminal option like "help"
    // was encountered
    if (errorMessage.empty()) {
      return EXIT_SUCCESS;
    }

    logzy::critical("{}", errorMessage);
    return EXIT_FAILURE;
  }

  logzy::info("Compiling {} files", args->files.size());

  for (const auto& file : args->files) {
    logzy::info("Compiling file '{}'", file);

    if (!driver::compileFile(file)) {
      return EXIT_FAILURE;
    }

    logzy::info("Compiled successfully");
  }

  return EXIT_SUCCESS;
}
