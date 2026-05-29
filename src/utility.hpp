#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace util {

  [[nodiscard]] constexpr auto isLetter(char character) noexcept -> bool {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
  }
  [[nodiscard]] constexpr auto isDigit(char character) noexcept -> bool {
    return (character >= '0' && character <= '9');
  }
  [[nodiscard]] constexpr auto isWhitespace(char character) noexcept -> bool {
    // Whitespace characters in default C locale from cppreference
    constexpr std::array whitespaceCharacters{' ',  '\f', '\n',
                                              '\r', '\t', '\v'};

    return std::ranges::any_of(whitespaceCharacters,
                               [character](char whitespaceChar) -> bool {
                                 return whitespaceChar == character;
                               });
  }

  template <std::integral T>
  [[nodiscard]] constexpr auto toNumber(std::string_view numberString) -> T {
    T output;

    constexpr int base = 10;
    auto [ptr, ec] = std::from_chars(
        numberString.data(), std::to_address(numberString.end()), output, base);

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
    auto [ptr, ec] =
        std::from_chars(numberString.data(),
                        std::to_address(numberString.end()), output, format);

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

  template <class Derived, class Base>
  auto uniqueDynamicCast(std::unique_ptr<Base> &ptr)
      -> std::unique_ptr<Derived> {
    if (auto *derivedRaw = dynamic_cast<Derived *>(ptr.get())) {
      // Releasing ownership so ptr doesn't delete the data
      ptr.release();
      // Ownership transferred to new instance
      return std::unique_ptr<Derived>(derivedRaw);
    }

    // ptr unchanged
    return nullptr;
  }

}  // namespace util
