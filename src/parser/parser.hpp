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

  constexpr void nextToken();
  void expectToken(TokenType type) const;

 private:
  Lexer lexer_;
  // Current Token produced by the lexer.
  Token currentToken_;
};

constexpr void Parser::nextToken() { currentToken_ = lexer_.nextToken(); }
