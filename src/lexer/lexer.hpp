#pragma once
#include <format>
#include <optional>
#include <string_view>

#include "token.hpp"

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

