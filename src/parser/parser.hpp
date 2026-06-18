#pragma once
#include <memory>

#include "ast_nodes.hpp"
#include "lexer/lexer.hpp"

class Parser {
 public:
  constexpr explicit Parser(std::string_view source) noexcept
      : lexer_{source} {
    nextToken();
  };

  [[nodiscard]] auto parsePrimary() -> std::unique_ptr<AstNode>;

  [[nodiscard]] auto parseNumber() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseBoolean() -> std::unique_ptr<BooleanAstNode>;
  [[nodiscard]] auto parseIdentifier() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseParentheses() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseFunctionPrototype()
      -> std::unique_ptr<FunctionPrototype>;
  [[nodiscard]] auto parseFunction() -> std::unique_ptr<Function>;

  [[nodiscard]] auto parsePostfixExpression() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseExpression() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseBinaryExpressionRhs(int expressionPrecedence,
                                              std::unique_ptr<AstNode> lhs)
      -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseBlock(
      std::optional<std::string_view> blockName = std::nullopt)
      -> std::unique_ptr<BlockAstNode>;

  [[nodiscard]] auto parseIfElse() -> std::unique_ptr<IfElseAstNode>;
  [[nodiscard]] auto parseLet() -> std::unique_ptr<LetAstNode>;

  [[nodiscard]] auto parseGlobalDeclaration() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseTranslationUnit()
      -> std::unique_ptr<TranslationUnitAstNode>;
  [[nodiscard]] auto parseCall(std::unique_ptr<AstNode> calledExpression)
      -> std::unique_ptr<CallAstNode>;
  [[nodiscard]] auto parseCast(std::unique_ptr<AstNode> castExpression)
      -> std::unique_ptr<CastAstNode>;

  // Returns the current token and processes the next one.
  constexpr auto nextToken() -> Token;

  // Returns the value of current token
  constexpr auto lexeme() const noexcept -> std::string_view;

  // Returns the type of current token
  constexpr auto lexemeType() const noexcept -> TokenType;

  // Asserts that the current token is of the argument type
  constexpr auto expect(TokenType type) -> Token;

  // Asserts that the current token is of the argument type, If it is, skips the
  // token, if not nothing happens
  constexpr void ensure(TokenType type) const;

  // Checks if the current token is of the given type
  [[nodiscard]] constexpr auto peek(TokenType type) const noexcept -> bool;

  // Checks if the current token's value matches the argument value
  [[nodiscard]] constexpr auto peek(std::string_view value) const noexcept
      -> bool;

  // Checks if the current token's value matches the argument value
  [[nodiscard]] constexpr auto peek(char value) const noexcept -> bool;

  // Checks if the current token is of the given type, if yes it consumes and
  // skips it, else nothing happens
  template <class T>
  [[nodiscard]] constexpr auto match(T val) -> bool
    requires requires {
      { peek(val) } -> std::same_as<bool>;
    };
  ;

 private:
  Lexer lexer_;
  // Current Token produced by the lexer.
  Token currentToken_;
};

constexpr auto Parser::nextToken() -> Token {
  Token tok = currentToken_;
  currentToken_ = lexer_.nextToken();
  return tok;
}
constexpr auto Parser::lexeme() const noexcept -> std::string_view {
  return currentToken_.value;
}

constexpr auto Parser::lexemeType() const noexcept -> TokenType {
  return currentToken_.type;
}

constexpr auto Parser::expect(TokenType type) -> Token {
  ensure(type);
  return nextToken();
}

constexpr void Parser::ensure(TokenType type) const {
  if (currentToken_.type != type) {
    throw std::logic_error(std::format("Expected token '{}' but received '{}'",
                                       type, currentToken_));
  }
}

template <class T>
[[nodiscard]] constexpr auto Parser::match(T val) -> bool
  requires requires {
    { peek(val) } -> std::same_as<bool>;
  }
{
  bool doesMatch = peek(val);
  if (doesMatch) {
    nextToken();
  }
  return doesMatch;
}

constexpr auto Parser::peek(TokenType type) const noexcept -> bool {
  return currentToken_.type == type;
}

constexpr auto Parser::peek(std::string_view value) const noexcept -> bool {
  return currentToken_.value == value;
}

constexpr auto Parser::peek(char value) const noexcept -> bool {
  return currentToken_.value.size() == 1 &&
         currentToken_.value.starts_with(value);
}
