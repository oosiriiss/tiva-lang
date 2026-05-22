#include "parser.hpp"
#include "debug.hpp"
#include "logzy/logzy.hpp"
#include <cassert>
#include <unordered_map>
#include <utility>

static std::unordered_map<TokenType, int> precedences{
    {TokenType::Plus, 10},
    {TokenType::Minus, 10},
    {TokenType::Multiply, 20},
    {TokenType::Divide, 20},
};

static constexpr auto getPrecedence(TokenType type) -> int {
  auto iter = precedences.find(type);
  if (iter == precedences.end()) {
    return -1;
  }
  return iter->second;
}

[[nodiscard]] auto Parser::parsePrimary() -> std::unique_ptr<AstNode> {
  switch (currentToken_.type) {
  case TokenType::Identifier:
    return parseIdentifier();
  case TokenType::Number:
    return parseNumber();
  case TokenType::ParenBegin:
    return parseParentheses();
  default:
    logzy::critical("Unsupported token {}", currentToken_);
    DEBUG_ASSERT(false);
    break;
  }
}

#define ASSERT_TOKEN_TYPE(tokenType)                                           \
  DEBUG_ASSERT(currentToken_.type == tokenType,                                \
               std::format("Expected token type {} and got {}",                \
                           std::to_underlying(tokenType), currentToken_));
#define ASSERT_NUMBER_TOKEN ASSERT_TOKEN_TYPE(TokenType::Number);
#define ASSERT_IDENTIFIER_TOKEN ASSERT_TOKEN_TYPE(TokenType::Identifier);
#define ASSERT_TOKEN_VALUE(tokenValue)                                         \
  do {                                                                         \
    DEBUG_ASSERT(currentToken_.value.length() > 0);                            \
    DEBUG_ASSERT(currentToken_.value == tokenValue,                            \
                 std::format("currentToken_={}, tokenValue='{}'",              \
                             currentToken_, tokenValue));                      \
  } while (0);

[[nodiscard]]
auto Parser::parseNumber() -> std::unique_ptr<NumberAstNode> {
  ASSERT_NUMBER_TOKEN;
  logzy::trace("Parsing number '{}'", currentToken_.value);
  auto num = std::make_unique<NumberAstNode>(currentToken_.value);
  nextToken();
  return num;
}

[[nodiscard]] auto Parser::parseIdentifier() -> std::unique_ptr<AstNode> {
  ASSERT_IDENTIFIER_TOKEN

  logzy::trace("Parsing identifier '{}'", currentToken_.value);
  auto variable = std::make_unique<VariableAstNode>(currentToken_.value);
  nextToken();

  return variable;
}

[[nodiscard]] auto Parser::parseParentheses() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing parentheses");
  ASSERT_TOKEN_VALUE("(");
  nextToken();
  auto expr = parseExpression();
  if (expr == nullptr) {
    return expr;
  }
  // assertion for simplicity for now
  ASSERT_TOKEN_VALUE(")");
  nextToken();

  return expr;
}

[[nodiscard]] auto Parser::parseExpression() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing expression's left side");
  auto lhs = parsePrimary();
  if (lhs == nullptr) {
    return lhs;
  }
  logzy::trace("Parsing expression's right side");
  return parseBinaryExpressionRhs(0, std::move(lhs));
}

[[nodiscard]] auto
Parser::parseBinaryExpressionRhs(int expressionPrecedence,
                                 std::unique_ptr<AstNode> lhs)
    -> std::unique_ptr<AstNode> {

  while (true) {
    int tokenPrecedence = getPrecedence(currentToken_.type);

    // Checking if it is a binary operator (non-binary get precedence -1)
    if (tokenPrecedence < expressionPrecedence) {
      return lhs;
    }

    TokenType binOp = currentToken_.type;
    nextToken();

    auto rhs = parsePrimary();
    if (rhs == nullptr) {
      return nullptr;
    }

    int nextPrecedence = getPrecedence(currentToken_.type);
    if (tokenPrecedence < nextPrecedence) {

      logzy::trace(
          "next operator has higher precedence curr: {}={} vs next: {}={}",
          binOp, tokenPrecedence, currentToken_.type, nextPrecedence);

      rhs = parseBinaryExpressionRhs(tokenPrecedence + 1, std::move(rhs));
      if (rhs == nullptr) {
        return nullptr;
      }
    }

    lhs = std::make_unique<BinaryExprAstNode>(binOp, std::move(lhs),
                                              std::move(rhs));
  }
}
