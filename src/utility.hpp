#pragma once
#include <array>

namespace util {

[[nodiscard]] constexpr auto isLetter(char c) noexcept -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
[[nodiscard]] constexpr auto isDigit(char c) noexcept -> bool {
  return (c >= '0' && c <= '9');
}
[[nodiscard]] constexpr auto isWhitespace(char c) noexcept -> bool {
  // Whitespace characters in default C locale from cppreference
  constexpr std::array whitespaceCharacters{' ', '\f', '\n', '\r', '\t', '\v'};

  for (char ws : whitespaceCharacters) {
    if (c == ws) {
      return true;
    }
  }

  return false;
}

} // namespace util
