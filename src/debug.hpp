#pragma once

#pragma once

#define ENABLE_DEBUG_UTILS

#if defined(ENABLE_DEBUG_UTILS)

#include <iostream>
#include <source_location>
#include <stacktrace>

namespace debugutils {
constexpr bool DEBUG = true; // NOLINT
} // namespace debugutils

#define DEBUG_ONLY(...) __VA_ARGS__

[[noreturn]] inline void handleAssertFail(
    std::string_view expr, std::string_view message,
    const std::source_location &sourceLoc = std::source_location::current(),
    const std::stacktrace &stacktrace = std::stacktrace::current()) {

  std::cerr << "\nAssertion failed\n";
  std::cerr << "Expression: " << expr << '\n';
  if (!message.empty()) {
    std::cerr << "Message: " << message << '\n';
  }
  std::cerr << "File: " << sourceLoc.file_name() << '\n';
  std::cerr << "Line: " << sourceLoc.line() << '\n';
  std::cerr << "Stacktrace:\n" << std::to_string(stacktrace) << '\n';
  std::abort();
}

#define GET_FIRST(First, ...) First // NOLINT

#define DEBUG_ASSERT(expr, ...)                                                \
  (static_cast<bool>(expr)                                                     \
       ? void(0)                                                               \
       : handleAssertFail(#expr, GET_FIRST(__VA_ARGS__ __VA_OPT__(, ) "")));

#else

namespace debugutils {
constexpr bool DEBUG = false; // NOLINT
} // namespace debugutils

#define DEBUG_ONLY(...)
#define DEBUG_ASSERT(...)

#endif
