#pragma once
#include <format>
#include <optional>
#include <string_view>

enum class TokenType {
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
  Unknown,
  __SizeGuard,
};

struct Token {
  std::string_view value = "UNINITIALZIED";
  TokenType type = TokenType::Unknown;
};

struct Lexer {
public:
  Lexer(std::string_view source) noexcept;

  [[nodiscard]] auto nextToken() -> Token;
  [[nodiscard]] auto readIdentifier() noexcept
      -> std::optional<std::string_view>;
  [[nodiscard]] auto parseKeyword(std::string_view identifierString) noexcept
      -> std::optional<Token>;
  [[nodiscard]] auto readNumber() noexcept -> std::optional<std::string_view>;
  [[nodiscard]] auto readSymbol() noexcept -> std::optional<TokenType>;
  void skipWhitespace() noexcept;

  [[nodiscard]] auto isFinished() const noexcept -> bool;

private:
  std::string_view source_;
};

template <>
struct std::formatter<TokenType> : std::formatter<std::string_view> {

  auto format(TokenType s, std::format_context &ctx) const {
    std::string_view name;
    switch (s) {
    case TokenType::Identifier:
      name = "Identifier";
      break;
    case TokenType::Number:
      name = "Number";
      break;
    case TokenType::Plus:
      name = "Plus";
      break;
    case TokenType::Minus:
      name = "Minus";
      break;
    case TokenType::Divide:
      name = "Divide";
      break;
    case TokenType::Multiply:
      name = "Multiply";
      break;
    case TokenType::ParenBegin:
      name = "ParenBegin";
      break;
    case TokenType::ParenEnd:
      name = "ParenEnd";
      break;
    case TokenType::Function:
      name = "Function";
      break;
    case TokenType::Comma:
      name = "Comma";
      break;
    case TokenType::Unknown:
      name = "Unknown";
      break;
    case TokenType::__SizeGuard:
      name = "__SizeGuard";
      break;
    default:
      name = "InvalidToken";
      break;
    }
    return std::formatter<std::string_view>::format(name, ctx);
  }
};

template <> struct std::formatter<Token> {
  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw std::format_error(
          "Brak obslugi niestandardowych specyfikatorow dla Token");
    }
    return it;
  }

  auto format(const Token &token, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "Token(type: {}, value: \"{}\")",
                          token.type, token.value);
  }
};
