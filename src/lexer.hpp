#pragma once
#include <optional>
#include <string_view>

enum class TokenType {
  Identifier,
  Number,
  Plus,
  Minus,
  Divide,
  Multiply,
  Unknown,
  __SizeGuard,
};

struct Token {
  std::string_view value;
  TokenType type;
};

struct Lexer {
public:
  Lexer(std::string_view source) noexcept;

  [[nodiscard]] auto nextToken() -> Token;
  [[nodiscard]] auto readIdentifier() noexcept
      -> std::optional<std::string_view>;
  [[nodiscard]] auto readNumber() noexcept -> std::optional<std::string_view>;
  [[nodiscard]] auto readSymbol() noexcept -> std::optional<TokenType>;
  void skipWhitespace() noexcept;

  [[nodiscard]] auto isFinished() const noexcept -> bool;

private:
  std::string_view source_;
};
