#include "parser.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

#include "debug.hpp"
#include "lexer.hpp"
#include "logzy/logzy.hpp"
#include "utility.hpp"

static std::unordered_map<TokenType, int> precedences{
    {TokenType::Assign, 2},    {TokenType::Plus, 10},   {TokenType::Minus, 10},
    {TokenType::Multiply, 20}, {TokenType::Divide, 20},
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
    case TokenType::CurlyBegin:
      return parseBlock();
    case TokenType::If:
      return parseIfElse();
    case TokenType::Let:
      return parseLet();
    default:
      logzy::critical("Unsupported token {}", currentToken_);
      DEBUG_ASSERT(false);
      break;
  }
}

#define ASSERT_TOKEN_TYPE(tokenType)                            \
  DEBUG_ASSERT(currentToken_.type == tokenType,                 \
               std::format("Expected token type {} and got {}", \
                           std::to_underlying(tokenType), currentToken_));
#define ASSERT_NUMBER_TOKEN ASSERT_TOKEN_TYPE(TokenType::Number);
#define ASSERT_IDENTIFIER_TOKEN ASSERT_TOKEN_TYPE(TokenType::Identifier);
#define ASSERT_TOKEN_VALUE(tokenValue)                            \
  do {                                                            \
    DEBUG_ASSERT(currentToken_.value.length() > 0);               \
    DEBUG_ASSERT(currentToken_.value == tokenValue,               \
                 std::format("currentToken_={}, tokenValue='{}'", \
                             currentToken_, tokenValue));         \
  } while (0);

[[nodiscard]]
auto Parser::parseNumber() -> std::unique_ptr<AstNode> {
  ASSERT_NUMBER_TOKEN;
  logzy::trace("Parsing number '{}'", currentToken_.value);

  std::unique_ptr<AstNode> result;

  if (currentToken_.value.contains('.')) {
    result = std::make_unique<FloatAstNode>(currentToken_.value);
  } else {
    result = std::make_unique<IntegerAstNode>(currentToken_.value);
  }

  nextToken();
  return result;
}

[[nodiscard]] auto Parser::parseIdentifier() -> std::unique_ptr<AstNode> {
  ASSERT_IDENTIFIER_TOKEN

  logzy::trace("Parsing identifier '{}'", currentToken_.value);
  std::string_view identifier = currentToken_.value;
  nextToken();  // Skipping identifier

  // Normal ident
  if (currentToken_.type != TokenType::ParenBegin) {
    logzy::trace("Was a normal identifier");
    return std::make_unique<VariableAstNode>(identifier);
  }

  if (currentToken_.type == TokenType::ParenBegin) {  // Call
    std::vector<std::unique_ptr<AstNode>> args;

    while (true) {
      if (auto arg = parseExpression()) {
        args.emplace_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (currentToken_.type == TokenType::ParenEnd) {
        break;
      }

      if (currentToken_.type != TokenType::Comma) {
        logzy::error("Expected ',' or ')' in function '{}' arguments",
                     identifier);
        return nullptr;
      }

      // Skipping comma
      nextToken();
    }
    return std::make_unique<CallAstNode>(identifier, std::move(args));
  }

  logzy::error("Invalid token '{}' after identifier '{}'", currentToken_,
               identifier);
  return nullptr;
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

[[nodiscard]] auto Parser::parseFunctionPrototype()
    -> std::unique_ptr<FunctionPrototype> {
  ASSERT_IDENTIFIER_TOKEN;
  std::string_view functionName = currentToken_.value;
  nextToken();

  ASSERT_TOKEN_VALUE("(");

  std::vector<std::string> argNames;
  nextToken();
  while (currentToken_.type == TokenType::Identifier) {
    argNames.emplace_back(currentToken_.value);
    nextToken();

    if (currentToken_.type == TokenType::Comma) {
      nextToken();
    } else if (currentToken_.type == TokenType::ParenEnd) {
      break;
    } else {
      logzy::error("Expected ',' or ')', but got {}", currentToken_);
      return nullptr;
    }
  }

  ASSERT_TOKEN_VALUE(")");
  nextToken();

  return std::make_unique<FunctionPrototype>(functionName, std::move(argNames));
}
[[nodiscard]] auto Parser::parseFunction() -> std::unique_ptr<Function> {
  // Skipping fn
  nextToken();

  auto prototype = parseFunctionPrototype();
  if (prototype == nullptr) {
    return nullptr;
  }

  std::unique_ptr<AstNode> body = nullptr;

  if (currentToken_.type == TokenType::ParenBegin) {
    body = parseExpression();
    if (body == nullptr) {
      return nullptr;
    }
  } else if (currentToken_.type == TokenType::CurlyBegin) {
    body = parseBlock();
    if (body == nullptr) {
      return nullptr;
    }
  }

  return std::make_unique<Function>(std::move(prototype), std::move(body));
}

[[nodiscard]] auto Parser::parseExpression() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing expression's left side");
  auto lhs = parsePrimary();
  if (lhs == nullptr) {
    return lhs;
  }

  logzy::trace("Parsing expression's right side");
  if (currentToken_.type == TokenType::Assign) {
    nextToken();

    auto variableNode = util::unique_dynamic_cast<VariableAstNode>(lhs);
    if (variableNode == nullptr) {
      logzy::error("Expected assignemnt lhs to be a lvalue");
      return nullptr;
    }

    std::unique_ptr<AstNode> rhs = parseExpression();
    if (rhs == nullptr) {
      logzy::error("No rhs");
      return nullptr;
    }

    return std::make_unique<AssignmentAstNode>(std::move(variableNode),
                                               std::move(rhs));
  } else {
    return parseBinaryExpressionRhs(0, std::move(lhs));
  }
}

[[nodiscard]] auto Parser::parseBinaryExpressionRhs(
    int expressionPrecedence, std::unique_ptr<AstNode> lhs)
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

[[nodiscard]] auto Parser::parseBlock(std::optional<std::string_view> blockName)
    -> std::unique_ptr<BlockAstNode> {
  logzy::debug("Parsing block");
  ASSERT_TOKEN_TYPE(TokenType::CurlyBegin);
  nextToken();

  std::vector<std::unique_ptr<AstNode>> expressions;
  while (currentToken_.type != TokenType::CurlyEnd) {
    if (auto expr = parseExpression()) {
      expressions.emplace_back(std::move(expr));
    } else {
      logzy::error("Couldn't parse expression in block");
      return nullptr;
    }
  }

  logzy::trace("Block has {} expressions", expressions.size());

  // Skippingthe last '}'
  nextToken();

  static std::uint32_t blockCounter{0};

  return std::make_unique<BlockAstNode>(
      (blockName.has_value()) ? *blockName
                              : std::format("block_{}", blockCounter),
      std::move(expressions));
}

[[nodiscard]] auto Parser::parseIfElse() -> std::unique_ptr<IfElseAstNode> {
  logzy::trace("Parsing if else");
  nextToken();  // Consuming if

  auto condition = parseExpression();
  if (condition == nullptr) {
    logzy::error("Couldn't parse if condition expression");
    return nullptr;
  }

  auto ifBlock = parseExpression();
  if (ifBlock == nullptr) {
    logzy::error("Couldn't parse if's ifBlock expression");
    return nullptr;
  }

  ASSERT_TOKEN_TYPE(TokenType::Else);
  nextToken();

  auto elseBlock = parseExpression();
  if (elseBlock == nullptr) {
    logzy::error("Couldn't parse if's elseBlock expression");
    return nullptr;
  }

  return std::make_unique<IfElseAstNode>(
      std::move(condition), std::move(ifBlock), std::move(elseBlock));
}

[[nodiscard]] auto Parser::parseLet() -> std::unique_ptr<LetAstNode> {
  ASSERT_TOKEN_TYPE(TokenType::Let);
  nextToken();

  ASSERT_TOKEN_TYPE(TokenType::Identifier);
  std::string_view varName = currentToken_.value;
  nextToken();

  ASSERT_TOKEN_TYPE(TokenType::Assign);
  nextToken();

  auto rhs = parseExpression();
  if (rhs == nullptr) {
    logzy::error("couldn't parse let expression rhs");
    return nullptr;
  }

  return std::make_unique<LetAstNode>(varName, std::move(rhs));
}
