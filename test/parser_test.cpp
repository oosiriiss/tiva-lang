#include "parser/parser.hpp"
#include <gtest/gtest.h>

std::unique_ptr<Parser> createParser(std::string_view source) {
  auto parser = std::make_unique<Parser>(source);
  parser->nextToken();
  return parser;
}

TEST(ParserTest, ParsesNumberCorrectly) {
  auto parser = createParser("42.5");
  auto node = parser->parseNumber();

  ASSERT_NE(node, nullptr) << "Parser should return a valid node";

  EXPECT_DOUBLE_EQ(node->val, 42.5);
}

TEST(ParserTest, ParsesVariableCorrectly) {
  auto parser = createParser("my_variable");
  auto baseNode = parser->parseIdentifier();

  ASSERT_NE(baseNode, nullptr);

  auto varNode = dynamic_cast<VariableAstNode *>(baseNode.get());
  ASSERT_NE(varNode, nullptr) << "Node should be of type VariableAstNode";

  EXPECT_EQ(varNode->name, "my_variable");
}

TEST(ParserTest, ParsesSimpleBinaryExpression) {
  auto parser = createParser("a + 5");
  auto baseNode = parser->parseBinaryExpression();

  ASSERT_NE(baseNode, nullptr);

  auto binaryNode = dynamic_cast<BinaryExprAstNode *>(baseNode.get());
  ASSERT_NE(binaryNode, nullptr) << "Root node should be BinaryExprAstNode";

  auto lhs = dynamic_cast<VariableAstNode *>(binaryNode->lhs.get());
  ASSERT_NE(lhs, nullptr);
  EXPECT_EQ(lhs->name, "a");

  auto rhs = dynamic_cast<NumberAstNode *>(binaryNode->rhs.get());
  ASSERT_NE(rhs, nullptr);
  EXPECT_DOUBLE_EQ(rhs->val, 5.0);
}

TEST(ParserTest, ParsesParenthesesCorrectly) {
  auto parser = createParser("( 10 )");
  auto baseNode = parser->parseParentheses();

  ASSERT_NE(baseNode, nullptr);

  auto numNode = dynamic_cast<NumberAstNode *>(baseNode.get());
  ASSERT_NE(numNode, nullptr)
      << "Parenthesized expression should return the enclosed node";
  EXPECT_DOUBLE_EQ(numNode->val, 10.0);
}

TEST(ParserTest, HandlesOperatorPrecedence) {
  auto parser = createParser("1 + 2 * 3");
  auto rootNode = parser->parseBinaryExpression();

  ASSERT_NE(rootNode, nullptr);

  auto plusNode = dynamic_cast<BinaryExprAstNode *>(rootNode.get());
  ASSERT_NE(plusNode, nullptr);

  auto lhsNum = dynamic_cast<NumberAstNode *>(plusNode->lhs.get());
  ASSERT_NE(lhsNum, nullptr);
  EXPECT_DOUBLE_EQ(lhsNum->val, 1.0);

  auto mulNode = dynamic_cast<BinaryExprAstNode *>(plusNode->rhs.get());
  ASSERT_NE(mulNode, nullptr);

  auto mulLhs = dynamic_cast<NumberAstNode *>(mulNode->lhs.get());
  auto mulRhs = dynamic_cast<NumberAstNode *>(mulNode->rhs.get());

  ASSERT_NE(mulLhs, nullptr);
  ASSERT_NE(mulRhs, nullptr);

  EXPECT_DOUBLE_EQ(mulLhs->val, 2.0);
  EXPECT_DOUBLE_EQ(mulRhs->val, 3.0);
}
