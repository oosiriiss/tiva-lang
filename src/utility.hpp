#pragma once
#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <stdexcept>
#include <string_view>

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

template <std::integral T>
[[nodiscard]] constexpr auto toNumber(std::string_view numberString) -> T {

  T output;

  constexpr int base = 10;
  auto [ptr, ec] = std::from_chars(numberString.data(),
                                   numberString.data() + numberString.length(),
                                   output, base);

  if (ec == std::errc()) {
    return output;
  }

  if (ec == std::errc::result_out_of_range) {
    throw std::out_of_range(
        std::format("Number '{}' larger than specified type", numberString));
  }

  throw std::invalid_argument(
      std::format("Number '{}' is not a number", numberString));
}

template <std::floating_point T>
[[nodiscard]] constexpr auto toNumber(std::string_view numberString) -> T {

  T output;

  auto format = std::chars_format::general;
  auto [ptr, ec] = std::from_chars(numberString.data(),
                                   numberString.data() + numberString.length(),
                                   output, format);

  if (ec == std::errc()) {
    return output;
  }

  if (ec == std::errc::result_out_of_range) {
    throw std::out_of_range(
        std::format("Number '{}' larger than specified type", numberString));
  }

  throw std::invalid_argument(
      std::format("Number '{}' is not a number", numberString));
}

} // namespace util
