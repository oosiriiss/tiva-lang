#pragma once
#include <format>
#include <optional>
#include <string_view>

enum class TokenType : char {
  Identifier,
  Number,
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
  Unknown,
};

struct Token {
  std::string_view value = "UNINITIALZIED";
  TokenType type = TokenType::Unknown;
};

struct Lexer {
 public:
  explicit Lexer(std::string_view source) noexcept;

  [[nodiscard]] auto nextToken() -> Token;
  [[nodiscard]] auto readIdentifier() noexcept
      -> std::optional<std::string_view>;
  [[nodiscard]] static auto parseKeyword(
      std::string_view identifierString) noexcept -> std::optional<Token>;
  [[nodiscard]] auto readNumber() noexcept -> std::optional<std::string_view>;
  [[nodiscard]] auto readSymbol() noexcept -> std::optional<TokenType>;
  [[nodiscard]] auto parseBlock() noexcept -> std::optional<Token>;
  void skipWhitespace() noexcept;

  [[nodiscard]] auto isFinished() const noexcept -> bool;

 private:
  std::string_view source_;
};

// 1. A reusable, compile-time stringifier
constexpr auto toString(TokenType type) noexcept -> std::string_view {
  switch (type) {
    case TokenType::Identifier:
      return "Identifier";
    case TokenType::Number:
      return "Number";
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
    auto iter = ctx.begin();
    if (iter != ctx.end() && *iter != '}') {
      throw std::format_error(
          "Brak obslugi niestandardowych specyfikatorow dla Token");
    }
    return iter;
  }

  static auto format(const Token &token, std::format_context &ctx) {
    return std::format_to(ctx.out(), "Token(type: {}, value: \"{}\")",
                          token.type, token.value);
  }
};
