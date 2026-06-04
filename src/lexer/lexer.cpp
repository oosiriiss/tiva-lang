#include "lexer.hpp"

#include <logzy/logzy.hpp>
#include <utility>

#include "lexer/token.hpp"
#include "utility.hpp"

using util::isDigit;
using util::isLetter;
using util::isWhitespace;

constexpr size_t SOURCE_FORESEE_LENGTH = 16;

Lexer::Lexer(std::string_view source) noexcept
    : source_{source} {}

auto Lexer::nextToken() -> Token {
  logzy::debug("Trying to parse next token");
  logzy::trace("Skipping whitespace");
  skipWhitespace();
  logzy::trace("source[0..16]='{}'", source_.substr(0, SOURCE_FORESEE_LENGTH));

  if (source_.length() <= 0) {
    return Token{.value = "", .type = TokenType::Eof};
  }

  if (auto tokenStringValue = readIdentifier()) {
    source_.remove_prefix(tokenStringValue->length());

    if (auto keywordToken = parseKeyword(*tokenStringValue)) {
      logzy::debug("Token is a keyword '{}'={}", *tokenStringValue,
                   keywordToken->value);
      return *keywordToken;
    }

    logzy::debug("Token is an identifier '{}'", *tokenStringValue);
    return Token{
        .value = std::move(tokenStringValue).value(),
        .type = TokenType::Identifier,
    };
  }

  if (auto symbolToken = readSymbol()) {
    source_.remove_prefix(symbolToken->value.size());
    logzy::debug("Token is a symbol '{}'", *symbolToken);
    return *symbolToken;
  }

  if (auto numberToken = readNumber()) {
    source_.remove_prefix(numberToken->length());
    logzy::debug("Token is a number '{}'", *numberToken);
    return Token{.value = std::move(numberToken).value(),
                 .type = TokenType::Number};
  }

  logzy::warn("Couldn't parse '{}' as a token.",
              source_.substr(0, SOURCE_FORESEE_LENGTH));
  return Token{.value = source_.substr(0, SOURCE_FORESEE_LENGTH),
               .type = TokenType::Unknown};
}

[[nodiscard]] auto Lexer::parseKeyword(
    std::string_view identifierString) noexcept -> std::optional<Token> {
  TokenType type = TokenType::Unknown;

  if (identifierString == "fn") {
    type = TokenType::Function;
  } else if (identifierString == "if") {
    type = TokenType::If;
  } else if (identifierString == "else") {
    type = TokenType::Else;
  } else if (identifierString == "let") {
    type = TokenType::Let;
  } else if (identifierString == "true" || identifierString == "false") {
    type = TokenType::Boolean;
  } else {
    logzy::trace("unknown keyword '{}'", identifierString);
    return std::nullopt;
  }
  return std::optional<Token>{std::in_place_t{}, identifierString, type};
}

namespace {
  [[nodiscard]] constexpr auto validIdentifierChar(char character) -> bool {
    return isLetter(character) || isDigit(character) || character == '_';
  }

  [[nodiscard]] constexpr auto validIdentifierFirstChar(char character)
      -> bool {
    return !isDigit(character) && validIdentifierChar(character);
  }
}  // namespace

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

[[nodiscard]] auto Lexer::readSymbol() noexcept -> std::optional<Token> {
  if (source_.size() <= 0) {
    return std::nullopt;
  }

  char character = source_[0];

  size_t symbolChars = 1;

  auto match = [&](char character) -> bool {
    if (source_.size() <= symbolChars) {
      return false;
    }
    bool isMatch = (source_[symbolChars] == character);

    if (isMatch) {
      ++symbolChars;
    }
    return isMatch;
  };

  auto type = TokenType::Unknown;

  switch (character) {
    case '+':
      type = TokenType::Plus;
      break;
    case '-':
      if (match('>')) {
        type = TokenType::Arrow;
        break;
      }
      type = TokenType::Minus;
      break;
    case '*':
      type = TokenType::Multiply;
      break;
    case '/':
      type = TokenType::Divide;
      break;
    case '(':
      type = TokenType::ParenBegin;
      break;
    case ')':
      type = TokenType::ParenEnd;
      break;
    case ',':
      type = TokenType::Comma;
      break;
    case '{':
      type = TokenType::CurlyBegin;
      break;
    case '}':
      type = TokenType::CurlyEnd;
      break;
    case '=':
      type = TokenType::Assign;
      break;
    case ':':
      type = TokenType::Colon;
      break;
    default:
      logzy::trace("unknown symbol '{}'", character);
      return std::nullopt;
  }

  return std::optional<Token>{
      std::in_place_t{}, std::string_view{source_.data(), symbolChars}, type

  };
}

void Lexer::skipWhitespace() noexcept {
  while (source_.size() > 0 && isWhitespace(source_[0])) {
    source_.remove_prefix(1);
  }
}

auto Lexer::isFinished() const noexcept -> bool { return source_.size() <= 0; }
