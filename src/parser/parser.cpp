#include "parser.hpp"

#include <llvm/IR/Intrinsics.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include "debug.hpp"
#include "lexer/token.hpp"
#include "logzy/logzy.hpp"
#include "parser/ast_nodes.hpp"
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
        [[fallthrough]];
      case TokenType::Minus:
        return Precedence::Term;
      case TokenType::Multiply:
        [[fallthrough]];
      case TokenType::Divide:
        return Precedence::Factor;
      default:
        return Precedence::None;
    }
  }

  // Checks if a number lexeme is a floating point number.
  constexpr auto isFloatNumber(std::string_view lexeme) -> bool {
    return lexeme.contains('.');
  }

}  // namespace

[[nodiscard]] auto Parser::parsePrimary() -> std::unique_ptr<AstNode> {
  switch (lexemeType()) {
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
  logzy::trace("Parsing number '{}'", lexeme());
  ensure(TokenType::Number);

  std::unique_ptr<AstNode> result;

  if (isFloatNumber(lexeme())) {
    result = std::make_unique<FloatAstNode>(lexeme());
  } else {
    result = std::make_unique<IntegerAstNode>(lexeme());
  }

  nextToken();
  return result;
}

auto Parser::parseBoolean() -> std::unique_ptr<BooleanAstNode> {
  logzy::trace("Parsing boolean'{}'", lexeme());
  ensure(TokenType::Boolean);

  std::unique_ptr<BooleanAstNode> result;

  if (lexeme() == "true") {
    result = std::make_unique<BooleanAstNode>(true);
  }

  if (lexeme() == "false") {
    result = std::make_unique<BooleanAstNode>(false);
  }

  nextToken();
  return result;
}

[[nodiscard]] auto Parser::parseIdentifier() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing identifier '{}'", lexeme());
  ensure(TokenType::Identifier);
  return std::make_unique<VariableAstNode>(nextToken().value);
}

[[nodiscard]] auto Parser::parseParentheses() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing parentheses");
  expect(TokenType::ParenBegin);
  auto expr = parseExpression();
  if (expr == nullptr) {
    return expr;
  }
  expect(TokenType::ParenEnd);
  return expr;
}

[[nodiscard]] auto Parser::parseFunctionPrototype()
    -> std::unique_ptr<FunctionPrototype> {
  logzy::trace("Parsing function prototype");
  std::string_view functionName = expect(TokenType::Identifier).value;
  logzy::trace("Function is: '{}'", functionName);
  expect(TokenType::ParenBegin);

  std::vector<Parameter> params;
  while (true) {
    logzy::trace("Parsing parameter");
    std::string_view name = expect(TokenType::Identifier).value;
    logzy::trace("Parameter name: '{}'", name);
    expect(TokenType::Colon);
    std::string_view typeString = expect(TokenType::Identifier).value;
      logzy::trace("Parameter's type string is: '{}'",typeString);

    TivaType declaredType = fromString(typeString);
    if (declaredType == TivaType::Unknown) {
      logzy::error("Expected parameter's type, but got: '{}'", lexeme());
      return nullptr;
    }
    logzy::trace("Parameter type: '{}'", declaredType);

    params.emplace_back(std::string(name), declaredType);

    if (match(',')) {
      continue;
    }

    if (match(')')) {
      break;
    }

    logzy::error("Expected function's prototype closing parenthesis");
    return nullptr;
  }

  TivaType returnType = TivaType::Unknown;

  // Optional return type
  if (match(TokenType::Arrow)) {
    auto returnTypeString = expect(TokenType::Identifier).value;
    returnType = fromString(returnTypeString);
  }

  return std::make_unique<FunctionPrototype>(functionName, std::move(params),
                                             returnType);
}
[[nodiscard]] auto Parser::parseFunction() -> std::unique_ptr<Function> {
  expect(TokenType::Function);

  auto prototype = parseFunctionPrototype();
  if (prototype == nullptr) {
    return nullptr;
  }

  std::unique_ptr<AstNode> body = nullptr;

  if (peek('(')) {
    body = parseExpression();
    if (body == nullptr) {
      return nullptr;
    }
  } else if (peek('{')) {
    body = parseBlock();
    if (body == nullptr) {
      return nullptr;
    }
  }

  return std::make_unique<Function>(std::move(prototype), std::move(body));
}

[[nodiscard]] auto Parser::parsePostfixExpression()
    -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing postfix expression");

  std::unique_ptr<AstNode> expression = parsePrimary();
  if (expression == nullptr) {
    logzy::error("No postfix expression lhs");
    return nullptr;
  }

  while (true) {
    if (peek('(')) {
      expression = parseCall(std::move(expression));
    }
    if (peek(TokenType::As)) {
      expression = parseCast(std::move(expression));
    } else {
      return expression;
    }
  }
}

[[nodiscard]] auto Parser::parseExpression() -> std::unique_ptr<AstNode> {
  logzy::trace("Parsing expression's left side");
  auto lhs = parsePostfixExpression();
  if (lhs == nullptr) {
    logzy::error("Couldn't parse expression. No left hand side");
    return lhs;
  }

  logzy::trace("Parsing expression's right side");
  if (match(TokenType::Assign)) {
    logzy::trace("Expression is an assignment");

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
    PrecedenceType tokenPrecedence = getPrecedence(lexemeType());

    // Checking if it is a binary operator
    if (tokenPrecedence < expressionPrecedence) {
      return lhs;
    }

    TokenType binOp = nextToken().type;

    auto rhs = parsePostfixExpression();
    if (rhs == nullptr) {
      return nullptr;
    }

    PrecedenceType nextPrecedence = getPrecedence(lexemeType());
    if (tokenPrecedence < nextPrecedence) {
      logzy::trace(
          "next operator has higher precedence curr: {}={} vs next: {}={}",
          binOp, tokenPrecedence, lexemeType(), nextPrecedence);

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
  expect(TokenType::CurlyBegin);

  std::vector<std::unique_ptr<AstNode>> expressions;
  while (!match('}')) {
    if (auto expr = parseExpression()) {
      expressions.emplace_back(std::move(expr));
    } else {
      logzy::error("Couldn't parse expression in block");
      return nullptr;
    }
  }

  logzy::trace("Block has {} expressions", expressions.size());

  static std::uint32_t BlockCounter{0};
  return std::make_unique<BlockAstNode>(
      (blockName.has_value())
          ? *blockName
          : std::string("block_") + std::to_string(BlockCounter),
      std::move(expressions));
}

[[nodiscard]] auto Parser::parseIfElse() -> std::unique_ptr<IfElseAstNode> {
  logzy::trace("Parsing if else");
  expect(TokenType::If);

  std::unique_ptr<AstNode> condition = parseExpression();
  if (condition == nullptr) {
    logzy::error("Couldn't parse if condition expression");
    return nullptr;
  }
  std::unique_ptr<AstNode> expression = parseExpression();
  if (expression == nullptr) {
    logzy::error("Couldn't parse if body expression");
    return nullptr;
  }
  std::unique_ptr<AstNode> elseBody = nullptr;
  if (match(TokenType::Else)) {
    elseBody = parseExpression();
    if (elseBody == nullptr) {
      logzy::error("Couldn't parse else body expression");
      return nullptr;
    }
  }

  return std::make_unique<IfElseAstNode>(
      std::move(condition), std::move(expression), std::move(elseBody));
}

[[nodiscard]] auto Parser::parseLet() -> std::unique_ptr<LetAstNode> {
  expect(TokenType::Let);
  std::string_view varName = expect(TokenType::Identifier).value;

  TivaType declaredType = TivaType::Unknown;

  // Type declaration
  if (match(':')) {
    auto variableTypeString = expect(TokenType::Identifier).value;
    // TODO :: Detect invalid types
    declaredType = fromString(variableTypeString);
    if (declaredType == TivaType::Unknown) {
      logzy::error("Invalid type variable '{}' has an unknown type '{}'",
                   varName, lexeme());
      return nullptr;
    }
  }

  expect(TokenType::Assign);

  auto rhs = parseExpression();
  if (rhs == nullptr) {
    logzy::error("couldn't parse let expression rhs");
    return nullptr;
  }

  return std::make_unique<LetAstNode>(varName, std::move(rhs), declaredType);
}

[[nodiscard]] auto Parser::parseGlobalDeclaration()
    -> std::unique_ptr<AstNode> {
  if (peek(TokenType::Function)) {
    return parseFunction();
  }

  logzy::error("Couldn't parse global declaration starting with token: {}",
               lexemeType());
  return nullptr;
}

[[nodiscard]] auto Parser::parseTranslationUnit()
    -> std::unique_ptr<TranslationUnitAstNode> {
  std::vector<std::unique_ptr<AstNode>> globalDeclarations;

  while (!peek(TokenType::Eof)) {
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

[[nodiscard]] auto Parser::parseCall(std::unique_ptr<AstNode> calledExpression)
    -> std::unique_ptr<CallAstNode> {
  logzy::trace("Parsing call");
  expect(TokenType::ParenBegin);

  std::vector<std::unique_ptr<AstNode>> args;

  while (!match(')')) {
    logzy::trace("Parsing argument");
    if (auto arg = parseExpression()) {
      args.emplace_back(std::move(arg));
    } else {
      return nullptr;
    }

    if (match(',')) {
      continue;
    }

    if (match(')')) {
      break;
    }

    logzy::error("Missing ',' or ')' in call arguments.");
    return nullptr;
  }

  return std::make_unique<CallAstNode>(std::move(calledExpression),
                                       std::move(args));
}

[[nodiscard]] auto Parser::parseCast(std::unique_ptr<AstNode> castExpression)
    -> std::unique_ptr<CastAstNode> {
  logzy::trace("Parsing cast");
  expect(TokenType::As);
  auto typeString = expect(TokenType::Identifier).value;

  auto type = fromString(typeString);
  if (type == TivaType::Unknown) {
    logzy::error("Invalid type during cast: '{}'", lexeme());
    return nullptr;
  }

  auto castNode =
      std::make_unique<CastAstNode>(std::move(castExpression), type);
  return castNode;
}
