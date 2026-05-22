#include "lexer.hpp"
#include "logzy/logzy.hpp"
#include "utility.hpp"
#include <array>
#include <string_view>
#include <unordered_map>
#include <utility>

using util::isDigit;
using util::isLetter;
using util::isWhitespace;

enum class State {
  Start,
};

Lexer::Lexer(std::string_view source) noexcept : source_{source} {}

auto Lexer::nextToken() -> Token {
  logzy::debug("Trying to parse next token");
  logzy::trace("Skipping whitespace");
  skipWhitespace();
  logzy::trace("source[0..16]='{}'", source_.substr(0, 16));

  if (auto tokenStringValue = readIdentifier()) {
    source_.remove_prefix(tokenStringValue->length());

    logzy::debug("Token is an identifier '{}'", *tokenStringValue);
    return Token{
        .value = std::move(tokenStringValue).value(),
        .type = TokenType::Identifier,
    };
  }

  if (auto symbolToken = readSymbol()) {
    auto c = source_.substr(0, 1);
    source_.remove_prefix(1);
    logzy::debug("Token is a symbol '{}'", *symbolToken);
    return Token{.value = c, .type = *symbolToken};
  }

  if (auto numberToken = readNumber()) {
    source_.remove_prefix(numberToken->length());
    logzy::debug("Token is a number '{}'", *numberToken);
    return Token{.value = std::move(numberToken).value(),
                 .type = TokenType::Number};
  }

  logzy::warn("Couldn't parse '{}' as a token.", source_.substr(0, 16));
  return Token{.value = source_.substr(0, 16), .type = TokenType::Unknown};
}

[[nodiscard]] static constexpr auto validIdentifierChar(char c) -> bool {
  return isLetter(c) || isDigit(c) || c == '_';
}

[[nodiscard]] static constexpr auto validIdentifierFirstChar(char c) -> bool {
  return !isDigit(c) && validIdentifierChar(c);
}

[[nodiscard]] auto Lexer::readIdentifier() noexcept
    -> std::optional<std::string_view> {

  if (source_.size() <= 0 || !validIdentifierFirstChar(source_[0])) {
    logzy::trace("'{}' is not a valid identifier beginning",
                 source_.substr(0, 1));
    return std::nullopt;
  }

  size_t identifierLength = 0;
  while (identifierLength < source_.size() &&
         validIdentifierChar(source_[identifierLength])) {
    ++identifierLength;
  }

  return std::optional<std::string_view>{std::in_place_t{}, source_.data(),
                                         identifierLength};
}

[[nodiscard]] auto Lexer::readNumber() noexcept
    -> std::optional<std::string_view> {

  size_t length = 0;

  // Allowing negative numbers
  if (length < source_.length() && source_[length] == '-') {
    logzy::trace("Negative number");
    ++length;
  }

  // whole part
  while (length < source_.length() && isDigit(source_[length])) {
    ++length;
  }

  // Decimal part of number
  if (length < source_.length() && source_[length] == '.') {
    ++length;
    while (length < source_.length() && isDigit(source_[length])) {
      ++length;
    }
  }

  if (length == 0) {
    logzy::trace("Not a number");
    return std::nullopt;
  }

  return std::optional<std::string_view>{std::in_place_t{}, source_.data(),
                                         length};
}

[[nodiscard]] auto Lexer::readSymbol() noexcept -> std::optional<TokenType> {

  static std::unordered_map<char, TokenType> tokens{
      {'+', TokenType::Plus},       {'-', TokenType::Minus},
      {'*', TokenType::Multiply},   {'/', TokenType::Divide},
      {'(', TokenType::ParenBegin}, {')', TokenType::ParenEnd},
  };

  if (source_.size() <= 0) {
    return std::nullopt;
  }

  char c = source_[0];

  auto iter = tokens.find(c);
  if (iter == tokens.end()) {
    logzy::trace("unknown symbol '{}'", c);
    return std::nullopt;
  }

  return iter->second;
}

void Lexer::skipWhitespace() noexcept {
  while (source_.size() > 0 && isWhitespace(source_[0])) {
    source_.remove_prefix(1);
  }
}

auto Lexer::isFinished() const noexcept -> bool { return source_.size() <= 0; }
