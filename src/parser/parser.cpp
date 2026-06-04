#include "parser.hpp"

#include <llvm/IR/Intrinsics.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include "debug.hpp"
#include "lexer/token.hpp"
#include "logzy/logzy.hpp"
#include "semantic/types.hpp"

namespace {

  enum Precedence : std::int8_t {
    None = -1,
    Assignment = 2,
    Term = 10,
    Factor = 20,
  };

  using PrecedenceType = std::underlying_type_t<Precedence>;

  constexpr auto getPrecedence(TokenType type) noexcept -> std::int8_t {
    switch (type) {
      case TokenType::Assign:
        return Precedence::Assignment;
      case TokenType::Plus:
      case TokenType::Minus:
        return Precedence::Term;
      case TokenType::Multiply:
      case TokenType::Divide:
        return Precedence::Factor;
      default:
        return Precedence::None;
    }
  }
}  // namespace

[[nodiscard]] auto Parser::parsePrimary() -> std::unique_ptr<AstNode> {
  switch (currentToken_.type) {
    case TokenType::Identifier:
      return parseIdentifier();
    case TokenType::Number:
      return parseNumber();
    case TokenType::Boolean:
      return parseBoolean();
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

[[nodiscard]]
auto Parser::parseNumber() -> std::unique_ptr<AstNode> {
  expectToken(TokenType::Number);
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

auto Parser::parseBoolean() -> std::unique_ptr<BooleanAstNode> {
  expectToken(TokenType::Boolean);
  logzy::trace("Parsing boolean'{}'", currentToken_.value);

  std::unique_ptr<BooleanAstNode> result;

  if (currentToken_.value == "true") {
    result = std::make_unique<BooleanAstNode>(true);
  }

  if (currentToken_.value == "false") {
    result = std::make_unique<BooleanAstNode>(false);
  }

  nextToken();
  return result;
}

[[nodiscard]] auto Parser::parseIdentifier() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing identifier '{}'", currentToken_.value);
  std::string_view identifier = currentToken_.value;
  expectToken(TokenType::Identifier);
  nextToken();

  // Normal ident
  if (currentToken_.type != TokenType::ParenBegin) {
    logzy::trace("Was a normal identifier");
    return std::make_unique<VariableAstNode>(identifier);
  }

  expectToken(TokenType::ParenBegin);
  nextToken();
  // Call
  logzy::trace("Identifier '{}' is a call", identifier);
  std::vector<std::unique_ptr<AstNode>> args;

  while (currentToken_.type != TokenType::ParenEnd) {
    logzy::trace("Parsing argument");
    if (auto arg = parseExpression()) {
      args.emplace_back(std::move(arg));
    } else {
      return nullptr;
    }

    if (currentToken_.type != TokenType::Comma &&
        currentToken_.type != TokenType::ParenEnd) {
      logzy::error("Expected ',' or ')' in function '{}' arguments",
                   identifier);
      return nullptr;
    }

    if (currentToken_.type == TokenType::ParenEnd) {
      break;
    }
    expectToken(TokenType::Comma);
    nextToken();
  }
  expectToken(TokenType::ParenEnd);
  nextToken();
  return std::make_unique<CallAstNode>(identifier, std::move(args));
}

[[nodiscard]] auto Parser::parseParentheses() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing parentheses");
  expectToken(TokenType::ParenBegin);
  nextToken();
  auto expr = parseExpression();
  if (expr == nullptr) {
    return expr;
  }
  // assertion for simplicity for now
  expectToken(TokenType::ParenEnd);
  nextToken();

  return expr;
}

[[nodiscard]] auto Parser::parseFunctionPrototype()
    -> std::unique_ptr<FunctionPrototype> {
  expectToken(TokenType::Identifier);
  std::string_view functionName = currentToken_.value;
  nextToken();
  expectToken(TokenType::ParenBegin);

  std::vector<Parameter> params;
  nextToken();
  while (currentToken_.type != TokenType::ParenEnd) {
    std::string name{currentToken_.value};
    nextToken();

    expectToken(TokenType::Colon);
    nextToken();
    expectToken(TokenType::Identifier);
    TivaType declaredType = fromString(currentToken_.value);
    if (declaredType == TivaType::Unknown) {
      logzy::error("Expected parameter's type, but got: '{}'",
                   currentToken_.value);
      return nullptr;
    }
    nextToken();

    params.emplace_back(std::move(name), declaredType);

    if (currentToken_.type == TokenType::Comma) {
      nextToken();
      continue;
    }

    if (currentToken_.type != TokenType::ParenEnd) {
      logzy::error("Expected function's prototype closing parenthesis");
      return nullptr;
    }
  }

  expectToken(TokenType::ParenEnd);
  nextToken();

  TivaType returnType = TivaType::Unknown;

  // Optional return type
  if (currentToken_.type == TokenType::Arrow) {
    nextToken();
    expectToken(TokenType::Identifier);
    returnType = fromString(currentToken_.value);
    nextToken();
  }

  return std::make_unique<FunctionPrototype>(functionName, std::move(params),
                                             returnType);
}
[[nodiscard]] auto Parser::parseFunction() -> std::unique_ptr<Function> {
  expectToken(TokenType::Function);
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
    logzy::trace("Expression is an assignment");
    nextToken();

    std::unique_ptr<AstNode> rhs = parseExpression();
    if (rhs == nullptr) {
      logzy::error("No rhs");
      return nullptr;
    }

    return std::make_unique<AssignmentAstNode>(std::move(lhs), std::move(rhs));
  }
  auto rhs = parseBinaryExpressionRhs(0, std::move(lhs));
  logzy::trace("right side parsed");
  return rhs;
}

[[nodiscard]] auto Parser::parseBinaryExpressionRhs(
    int expressionPrecedence, std::unique_ptr<AstNode> lhs)
    -> std::unique_ptr<AstNode> {
  while (true) {
    PrecedenceType tokenPrecedence = getPrecedence(currentToken_.type);

    // Checking if it is a binary operator
    if (tokenPrecedence < expressionPrecedence) {
      return lhs;
    }

    TokenType binOp = currentToken_.type;
    nextToken();

    auto rhs = parsePrimary();
    if (rhs == nullptr) {
      return nullptr;
    }

    PrecedenceType nextPrecedence = getPrecedence(currentToken_.type);
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
  expectToken(TokenType::CurlyBegin);
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

  static std::uint32_t BlockCounter{0};
  return std::make_unique<BlockAstNode>(
      (blockName.has_value())
          ? *blockName
          : std::string("block_") + std::to_string(BlockCounter),
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

  expectToken(TokenType::Else);
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
  expectToken(TokenType::Let);
  nextToken();

  expectToken(TokenType::Identifier);
  std::string_view varName = currentToken_.value;
  nextToken();

  TivaType declaredType = TivaType::Unknown;

  // Type declaration
  if (currentToken_.type == TokenType::Colon) {
    nextToken();
    expectToken(TokenType::Identifier);  // Declared type
    // TODO :: Detect invalid types
    declaredType = fromString(currentToken_.value);
    if (declaredType == TivaType::Unknown) {
      logzy::error("Invalid type variable '{}' has an unknown type '{}'",
                   varName, currentToken_.value);
      return nullptr;
    }
    nextToken();
  }

  expectToken(TokenType::Assign);
  nextToken();

  auto rhs = parseExpression();
  if (rhs == nullptr) {
    logzy::error("couldn't parse let expression rhs");
    return nullptr;
  }

  return std::make_unique<LetAstNode>(varName, std::move(rhs), declaredType);
}

[[nodiscard]] auto Parser::parseGlobalDeclaration()
    -> std::unique_ptr<AstNode> {
  if (currentToken_.type == TokenType::Function) {
    return parseFunction();
  }

  logzy::error("Couldn't parse global declaration starting with token: {}",
               currentToken_.type);
  return nullptr;
}

[[nodiscard]] auto Parser::parseTranslationUnit()
    -> std::unique_ptr<TranslationUnitAstNode> {
  std::vector<std::unique_ptr<AstNode>> globalDeclarations;

  while (currentToken_.type != TokenType::Eof) {
    auto declaration = parseGlobalDeclaration();
    if (declaration == nullptr) {
      logzy::error("Couldn't parse global declaration");
      break;
    }
    globalDeclarations.emplace_back(std::move(declaration));
  }

  return std::make_unique<TranslationUnitAstNode>(
      std::move(globalDeclarations));
}

void Parser::expectToken(TokenType type) const {
  if (currentToken_.type != type) {
    throw std::logic_error(std::format("Expected token '{}' but received '{}'",
                                       type, currentToken_));
  }
}
