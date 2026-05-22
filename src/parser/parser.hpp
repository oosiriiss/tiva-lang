#pragma once
#include "ast_nodes.hpp"
#include "lexer.hpp"
#include <memory>

class Parser {
public:
  constexpr Parser(std::string_view source) noexcept : lexer_{source} {
    nextToken();
  };

  [[nodiscard]] auto parsePrimary() -> std::unique_ptr<AstNode>;

  [[nodiscard]] auto parseNumber() -> std::unique_ptr<NumberAstNode>;
  [[nodiscard]] auto parseIdentifier() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseParentheses() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseExpression() -> std::unique_ptr<AstNode>;
  [[nodiscard]] auto parseFunctionPrototype()
      -> std::unique_ptr<FunctionPrototype>;
  [[nodiscard]] auto parseFunction() -> std::unique_ptr<Function>;
  [[nodiscard]] auto parseBinaryExpressionRhs(int expressionPrecedence,
                                              std::unique_ptr<AstNode> lhs)
      -> std::unique_ptr<AstNode>;

  constexpr void nextToken() { currentToken_ = lexer_.nextToken(); }

private:
  Lexer lexer_;
  // Current Token produced by the lexer.
  Token currentToken_;
};
