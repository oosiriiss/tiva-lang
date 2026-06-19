#pragma once
#include <format>
#include <string_view>

enum class TokenType : char {
  Identifier,
  Number,
  Boolean,
  Plus,
  Minus,
  Divide,
  Multiply,
  ParenBegin,
  ParenEnd,
  Function,
  Comma,
  CurlyBegin,
  CurlyEnd,
  Assign,
  If,
  Else,
  Let,
  Colon,
  Arrow,
  Eof,
  As,
  Equality,
  Unknown,
};

struct Token {
  std::string_view value = "UNINITIALZIED";
  TokenType type = TokenType::Unknown;
};

constexpr auto toString(TokenType type) noexcept -> std::string_view {
  switch (type) {
    case TokenType::Identifier:
      return "Identifier";
    case TokenType::Number:
      return "Number";
    case TokenType::Boolean:
      return "Boolean";
    case TokenType::Plus:
      return "Plus";
    case TokenType::Minus:
      return "Minus";
    case TokenType::Divide:
      return "Divide";
    case TokenType::Multiply:
      return "Multiply";
    case TokenType::ParenBegin:
      return "ParenBegin";
    case TokenType::ParenEnd:
      return "ParenEnd";
    case TokenType::Function:
      return "Function";
    case TokenType::Comma:
      return "Comma";
    case TokenType::CurlyBegin:
      return "CurlyBegin";
    case TokenType::CurlyEnd:
      return "CurlyEnd";
    case TokenType::Assign:
      return "Assign";
    case TokenType::If:
      return "If";
    case TokenType::Else:
      return "Else";
    case TokenType::Let:
      return "Let";
    case TokenType::Colon:
      return "Colon";
    case TokenType::Arrow:
      return "Arrow";
    case TokenType::Eof:
      return "Eof";
    case TokenType::As:
      return "As";
    case TokenType::Equality:
      return "Equality";
    case TokenType::Unknown:
      return "Unknown";
    default:
      return "SYMBOL_WITHOUT_STRING_REPRESENTATION";
  }
}

template <>
struct std::formatter<TokenType> : std::formatter<std::string_view> {
  auto format(TokenType token, std::format_context &ctx) const {
    return std::formatter<std::string_view>::format(toString(token), ctx);
  }
};

template <>
struct std::formatter<Token> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(const Token &token, std::format_context &ctx) {
    return std::format_to(ctx.out(), "Token(type: {}, value: \"{}\")",
                          token.type, token.value);
  }
};
