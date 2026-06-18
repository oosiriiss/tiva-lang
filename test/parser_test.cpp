#include "parser/parser.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, bugprone-argument-comment, readability-magic-numbers, modernize-use-std-numbers)

// Helper Assertions
namespace {

template <typename T>
auto assertNode(AstNode* node) -> T* {
  EXPECT_NE(node, nullptr);
  auto* casted = dynamic_cast<T*>(node);
  EXPECT_NE(casted, nullptr);
  return casted;
}

auto assertIntegerNode(AstNode* node, std::uint64_t expectedValue) -> void {
  auto* intNode = assertNode<IntegerAstNode>(node);
  if (intNode != nullptr) {
    EXPECT_EQ(intNode->val, expectedValue);
  }
}

auto assertFloatNode(AstNode* node, double expectedValue) -> void {
  auto* floatNode = assertNode<FloatAstNode>(node);
  if (floatNode != nullptr) {
    EXPECT_DOUBLE_EQ(floatNode->val, expectedValue);
  }
}

auto assertBooleanNode(AstNode* node, bool expectedValue) -> void {
  auto* boolNode = assertNode<BooleanAstNode>(node);
  if (boolNode != nullptr) {
    EXPECT_EQ(boolNode->val, expectedValue);
  }
}

auto assertVariableNode(AstNode* node, std::string_view expectedName) -> void {
  auto* varNode = assertNode<VariableAstNode>(node);
  if (varNode != nullptr) {
    EXPECT_EQ(varNode->name, expectedName);
  }
}

auto assertBinaryExpr(AstNode* node, TokenType expectedOp) -> BinaryExprAstNode* {
  auto* binNode = assertNode<BinaryExprAstNode>(node);
  if (binNode != nullptr) {
    EXPECT_EQ(binNode->op, expectedOp);
  }
  return binNode;
}

} // namespace

// Helper Methods
TEST(ParserHelperTest, PeekTokenType) {
  Parser parser{"let x = 42"};
  EXPECT_TRUE(parser.peek(TokenType::Let));
  EXPECT_FALSE(parser.peek(TokenType::Identifier));
}

TEST(ParserHelperTest, PeekStringValue) {
  Parser parser{"let x = 42"};
  EXPECT_TRUE(parser.peek("let"));
  EXPECT_FALSE(parser.peek("x"));
}

TEST(ParserHelperTest, PeekCharValue) {
  Parser parser{"{ let x = 42 }"};
  EXPECT_TRUE(parser.peek('{'));
  EXPECT_FALSE(parser.peek('}'));
}

TEST(ParserHelperTest, MatchTokenType) {
  Parser parser{"let x = 42"};
  EXPECT_TRUE(parser.match(TokenType::Let));
  EXPECT_EQ(parser.lexemeType(), TokenType::Identifier);
  EXPECT_EQ(parser.lexeme(), "x");

  EXPECT_FALSE(parser.match(TokenType::Assign));
  EXPECT_EQ(parser.lexemeType(), TokenType::Identifier);
}

TEST(ParserHelperTest, MatchCharValue) {
  Parser parser{"{ let x }"};
  EXPECT_TRUE(parser.match('{'));
  EXPECT_EQ(parser.lexemeType(), TokenType::Let);

  EXPECT_FALSE(parser.match('}'));
  EXPECT_EQ(parser.lexemeType(), TokenType::Let);
}

TEST(ParserHelperTest, EnsureAndExpectSuccess) {
  Parser parser{"let x"};
  EXPECT_NO_THROW(parser.ensure(TokenType::Let));
  EXPECT_EQ(parser.lexemeType(), TokenType::Let);

  Token tok;
  EXPECT_NO_THROW(tok = parser.expect(TokenType::Let));
  EXPECT_EQ(tok.type, TokenType::Let);
  EXPECT_EQ(tok.value, "let");
  EXPECT_EQ(parser.lexemeType(), TokenType::Identifier);
}

TEST(ParserHelperTest, EnsureAndExpectThrowsOnMismatch) {
  Parser parser{"let x"};
  EXPECT_THROW(parser.ensure(TokenType::Identifier), std::logic_error);
  EXPECT_THROW(parser.expect(TokenType::Identifier), std::logic_error);
}

// Primary AST Nodes
TEST(ParserPrimaryTest, ParseNumberInteger) {
  Parser parser{"42"};
  auto node = parser.parseNumber();
  assertIntegerNode(node.get(), 42);
}

TEST(ParserPrimaryTest, ParseNumberFloat) {
  Parser parser{"3.14159"};
  auto node = parser.parseNumber();
  assertFloatNode(node.get(), 3.14159);
}

TEST(ParserPrimaryTest, ParseNumberThrowsOnInvalid) {
  Parser parser{"myVar"};
  EXPECT_THROW((void)parser.parseNumber(), std::logic_error);
}

TEST(ParserPrimaryTest, ParseBooleanTrue) {
  Parser parser{"true"};
  auto node = parser.parseBoolean();
  assertBooleanNode(node.get(), true);
}

TEST(ParserPrimaryTest, ParseBooleanFalse) {
  Parser parser{"false"};
  auto node = parser.parseBoolean();
  assertBooleanNode(node.get(), false);
}

TEST(ParserPrimaryTest, ParseBooleanThrowsOnInvalid) {
  Parser parser{"123"};
  EXPECT_THROW((void)parser.parseBoolean(), std::logic_error);
}

TEST(ParserPrimaryTest, ParseIdentifier) {
  Parser parser{"myVariable"};
  auto node = parser.parseIdentifier();
  assertVariableNode(node.get(), "myVariable");
}

TEST(ParserPrimaryTest, ParseIdentifierThrowsOnInvalid) {
  Parser parser{"123"};
  EXPECT_THROW((void)parser.parseIdentifier(), std::logic_error);
}

TEST(ParserPrimaryTest, ParseParenthesesValid) {
  Parser parser{"( 100 )"};
  auto node = parser.parseParentheses();
  assertIntegerNode(node.get(), 100);
}

TEST(ParserPrimaryTest, ParseParenthesesUnclosedThrows) {
  Parser parser{"( 100"};
  EXPECT_THROW((void)parser.parseParentheses(), std::logic_error);
}

// Expression Parsing
TEST(ParserExpressionTest, SingleIdentifier) {
  Parser parser{"x"};
  auto node = parser.parseExpression();
  assertVariableNode(node.get(), "x");
}

TEST(ParserExpressionTest, Assignment) {
  Parser parser{"x = y"};
  auto node = parser.parseExpression();
  auto* assign = assertNode<AssignmentAstNode>(node.get());
  assertVariableNode(assign->lhs.get(), "x");
  assertVariableNode(assign->rhs.get(), "y");
}

TEST(ParserExpressionTest, AssignmentRhsErrorReturnsNull) {
  Parser parser{"x = y as invalid"};
  auto node = parser.parseExpression();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserExpressionTest, PrecedenceAddMul) {
  Parser parser{"1 + 2 * 3"};
  auto node = parser.parseExpression();
  auto* root = assertBinaryExpr(node.get(), TokenType::Plus);
  assertIntegerNode(root->lhs.get(), 1);
  auto* rhs = assertBinaryExpr(root->rhs.get(), TokenType::Multiply);
  assertIntegerNode(rhs->lhs.get(), 2);
  assertIntegerNode(rhs->rhs.get(), 3);
}

TEST(ParserExpressionTest, PrecedenceMulAdd) {
  Parser parser{"1 * 2 + 3"};
  auto node = parser.parseExpression();
  auto* root = assertBinaryExpr(node.get(), TokenType::Plus);
  auto* lhs = assertBinaryExpr(root->lhs.get(), TokenType::Multiply);
  assertIntegerNode(lhs->lhs.get(), 1);
  assertIntegerNode(lhs->rhs.get(), 2);
  assertIntegerNode(root->rhs.get(), 3);
}

TEST(ParserExpressionTest, BinaryAssociativityLeftToRight) {
  Parser parser{"10 - 5 - 2"};
  auto node = parser.parseExpression();
  auto* root = assertBinaryExpr(node.get(), TokenType::Minus);
  auto* lhs = assertBinaryExpr(root->lhs.get(), TokenType::Minus);
  assertIntegerNode(lhs->lhs.get(), 10);
  assertIntegerNode(lhs->rhs.get(), 5);
  assertIntegerNode(root->rhs.get(), 2);
}

TEST(ParserExpressionTest, ComplexGrouping) {
  Parser parser{"(1 + 2) * 3"};
  auto node = parser.parseExpression();
  auto* root = assertBinaryExpr(node.get(), TokenType::Multiply);
  auto* lhs = assertBinaryExpr(root->lhs.get(), TokenType::Plus);
  assertIntegerNode(lhs->lhs.get(), 1);
  assertIntegerNode(lhs->rhs.get(), 2);
  assertIntegerNode(root->rhs.get(), 3);
}

// Postfix Expressions
TEST(ParserPostfixTest, CallNoArgs) {
  Parser parser{"foo()"};
  auto node = parser.parseExpression();
  auto* call = assertNode<CallAstNode>(node.get());
  assertVariableNode(call->toCall.get(), "foo");
  EXPECT_TRUE(call->args.empty());
}

TEST(ParserPostfixTest, CallWithMultipleArgs) {
  Parser parser{"add(x, 3.14, true)"};
  auto node = parser.parseExpression();
  auto* call = assertNode<CallAstNode>(node.get());
  assertVariableNode(call->toCall.get(), "add");
  ASSERT_EQ(call->args.size(), 3);
  assertVariableNode(call->args[0].get(), "x");
  assertFloatNode(call->args[1].get(), 3.14);
  assertBooleanNode(call->args[2].get(), true);
}

TEST(ParserPostfixTest, CallNested) {
  Parser parser{"foo(bar(1))"};
  auto node = parser.parseExpression();
  auto* callOuter = assertNode<CallAstNode>(node.get());
  assertVariableNode(callOuter->toCall.get(), "foo");
  ASSERT_EQ(callOuter->args.size(), 1);

  auto* callInner = assertNode<CallAstNode>(callOuter->args[0].get());
  assertVariableNode(callInner->toCall.get(), "bar");
  ASSERT_EQ(callInner->args.size(), 1);
  assertIntegerNode(callInner->args[0].get(), 1);
}

TEST(ParserPostfixTest, CallSyntaxErrorMissingComma) {
  Parser parser{"foo(1 2)"};
  auto node = parser.parseExpression();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserPostfixTest, CallSyntaxErrorInvalidArg) {
  Parser parser{"foo(x as invalid)"};
  auto node = parser.parseExpression();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserPostfixTest, CastValidInt) {
  Parser parser{"x as i32"};
  auto node = parser.parseExpression();
  auto* cast = assertNode<CastAstNode>(node.get());
  assertVariableNode(cast->operand.get(), "x");
  EXPECT_EQ(cast->targetType, TivaType::Int);
}

TEST(ParserPostfixTest, CastValidFloat) {
  Parser parser{"y as float"};
  auto node = parser.parseExpression();
  auto* cast = assertNode<CastAstNode>(node.get());
  assertVariableNode(cast->operand.get(), "y");
  EXPECT_EQ(cast->targetType, TivaType::Float);
}

TEST(ParserPostfixTest, CastValidBool) {
  Parser parser{"z as bool"};
  auto node = parser.parseExpression();
  auto* cast = assertNode<CastAstNode>(node.get());
  assertVariableNode(cast->operand.get(), "z");
  EXPECT_EQ(cast->targetType, TivaType::Boolean);
}

TEST(ParserPostfixTest, CastInvalidType) {
  Parser parser{"x as invalid_type"};
  auto node = parser.parseExpression();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserPostfixTest, CastChained) {
  Parser parser{"x as float as i32"};
  auto node = parser.parseExpression();
  auto* outerCast = assertNode<CastAstNode>(node.get());
  EXPECT_EQ(outerCast->targetType, TivaType::Int);

  auto* innerCast = assertNode<CastAstNode>(outerCast->operand.get());
  EXPECT_EQ(innerCast->targetType, TivaType::Float);
  assertVariableNode(innerCast->operand.get(), "x");
}

TEST(ParserPostfixTest, ChainedCallAndCast) {
  Parser parser{"foo() as float"};
  auto node = parser.parseExpression();
  auto* cast = assertNode<CastAstNode>(node.get());
  EXPECT_EQ(cast->targetType, TivaType::Float);

  auto* call = assertNode<CallAstNode>(cast->operand.get());
  assertVariableNode(call->toCall.get(), "foo");
  EXPECT_TRUE(call->args.empty());
}

TEST(ParserPostfixTest, DoubleCallLeavesSecondParen) {
  Parser parser{"foo()()"};
  auto node = parser.parseExpression();
  
  auto* call = assertNode<CallAstNode>(node.get());
  assertVariableNode(call->toCall.get(), "foo");
  EXPECT_TRUE(call->args.empty());

  EXPECT_TRUE(parser.peek('('));
}

// Control Flow
TEST(ParserControlFlowTest, SimpleIfWithoutBraces) {
  Parser parser{"if cond a"};
  auto node = parser.parseIfElse();
  EXPECT_NE(node, nullptr);
  assertVariableNode(node->condition.get(), "cond");
  assertVariableNode(node->body.get(), "a");
  EXPECT_EQ(node->elseBody, nullptr);
}

// Let Variables
TEST(ParserControlFlowTest, SimpleIfElseWithoutBraces) {
  Parser parser{"if cond a else b"};
  auto node = parser.parseIfElse();
  EXPECT_NE(node, nullptr);
  assertVariableNode(node->condition.get(), "cond");
  assertVariableNode(node->body.get(), "a");
  assertVariableNode(node->elseBody.get(), "b");
}

TEST(ParserControlFlowTest, IfElseWithBlocks) {
  Parser parser{"if cond { 1 } else { 2 }"};
  auto node = parser.parseIfElse();
  EXPECT_NE(node, nullptr);
  assertVariableNode(node->condition.get(), "cond");

  auto* bodyBlock = assertNode<BlockAstNode>(node->body.get());
  ASSERT_EQ(bodyBlock->expressions.size(), 1);
  assertIntegerNode(bodyBlock->expressions[0].get(), 1);

  auto* elseBlock = assertNode<BlockAstNode>(node->elseBody.get());
  ASSERT_EQ(elseBlock->expressions.size(), 1);
  assertIntegerNode(elseBlock->expressions[0].get(), 2);
}

TEST(ParserControlFlowTest, ElseIfChain) {
  Parser parser{"if a { 1 } else if b { 2 } else { 3 }"};
  auto node = parser.parseIfElse();
  EXPECT_NE(node, nullptr);
  assertVariableNode(node->condition.get(), "a");

  auto* body1 = assertNode<BlockAstNode>(node->body.get());
  assertIntegerNode(body1->expressions[0].get(), 1);

  auto* nestedIf = assertNode<IfElseAstNode>(node->elseBody.get());
  assertVariableNode(nestedIf->condition.get(), "b");

  auto* body2 = assertNode<BlockAstNode>(nestedIf->body.get());
  assertIntegerNode(body2->expressions[0].get(), 2);

  auto* body3 = assertNode<BlockAstNode>(nestedIf->elseBody.get());
  assertIntegerNode(body3->expressions[0].get(), 3);
}

// Let Variables
TEST(ParserLetTest, SimpleLet) {
  Parser parser{"let x = 42"};
  auto node = parser.parseLet();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->varName, "x");
  EXPECT_EQ(node->declaredType, TivaType::Unknown);
  assertIntegerNode(node->rhs.get(), 42);
}

TEST(ParserLetTest, LetWithTypeAnnotationInt) {
  Parser parser{"let x: i32 = 42"};
  auto node = parser.parseLet();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->varName, "x");
  EXPECT_EQ(node->declaredType, TivaType::Int);
  assertIntegerNode(node->rhs.get(), 42);
}

TEST(ParserLetTest, LetWithTypeAnnotationFloat) {
  Parser parser{"let y: float = 3.14"};
  auto node = parser.parseLet();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->varName, "y");
  EXPECT_EQ(node->declaredType, TivaType::Float);
  assertFloatNode(node->rhs.get(), 3.14);
}

TEST(ParserLetTest, LetWithTypeAnnotationBool) {
  Parser parser{"let z: bool = true"};
  auto node = parser.parseLet();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->varName, "z");
  EXPECT_EQ(node->declaredType, TivaType::Boolean);
  assertBooleanNode(node->rhs.get(), true);
}

TEST(ParserLetTest, LetInvalidTypeAnnotationReturnsNull) {
  Parser parser{"let x: invalid_type = 42"};
  auto node = parser.parseLet();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserLetTest, LetMissingAssignThrows) {
  Parser parser{"let x 42"};
  EXPECT_THROW((void)parser.parseLet(), std::logic_error);
}

TEST(ParserLetTest, LetRhsExpressionErrorReturnsNull) {
  Parser parser{"let x = y as invalid_type"};
  auto node = parser.parseLet();
  EXPECT_EQ(node, nullptr);
}

// Blocks
TEST(ParserBlockTest, EmptyBlock) {
  Parser parser{"{}"};
  auto node = parser.parseBlock("empty");
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->name, "empty");
  EXPECT_TRUE(node->expressions.empty());
}

TEST(ParserBlockTest, BlockWithAutoNameGeneration) {
  Parser parser{"{}"};
  auto node = parser.parseBlock();
  EXPECT_NE(node, nullptr);
  EXPECT_FALSE(node->name.empty());
  EXPECT_TRUE(node->name.starts_with("block_"));
}

TEST(ParserBlockTest, BlockWithMultipleExpressions) {
  Parser parser{"{ let a = 10\n a + 2 }"};
  auto node = parser.parseBlock("my_block");
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->name, "my_block");
  ASSERT_EQ(node->expressions.size(), 2);

  auto* letNode = assertNode<LetAstNode>(node->expressions[0].get());
  EXPECT_EQ(letNode->varName, "a");

  auto* binExpr = assertBinaryExpr(node->expressions[1].get(), TokenType::Plus);
  assertVariableNode(binExpr->lhs.get(), "a");
  assertIntegerNode(binExpr->rhs.get(), 2);
}

TEST(ParserBlockTest, BlockWithNestedBlocks) {
  Parser parser{"{ { let x = 5 } x }"};
  auto node = parser.parseBlock();
  EXPECT_NE(node, nullptr);
  ASSERT_EQ(node->expressions.size(), 2);

  auto* innerBlock = assertNode<BlockAstNode>(node->expressions[0].get());
  ASSERT_EQ(innerBlock->expressions.size(), 1);

  assertVariableNode(node->expressions[1].get(), "x");
}

TEST(ParserBlockTest, BlockWithExpressionSyntaxErrorReturnsNull) {
  Parser parser{"{ let x = 1\n y as invalid_type }"};
  auto node = parser.parseBlock();
  EXPECT_EQ(node, nullptr);
}

// Functions & Prototypes
TEST(ParserFunctionTest, PrototypeSimpleOneParam) {
  Parser parser{"myFunc(x: i32)"};
  auto node = parser.parseFunctionPrototype();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->name, "myFunc");
  ASSERT_EQ(node->params.size(), 1);
  EXPECT_EQ(node->params[0].name, "x");
  EXPECT_EQ(node->params[0].declaredType, TivaType::Int);
  EXPECT_EQ(node->returnType, TivaType::Unknown);
}

TEST(ParserFunctionTest, PrototypeWithParamsAndReturn) {
  Parser parser{"add(x: i32, y: float) -> bool"};
  auto node = parser.parseFunctionPrototype();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->name, "add");
  EXPECT_EQ(node->returnType, TivaType::Boolean);
  ASSERT_EQ(node->params.size(), 2);

  EXPECT_EQ(node->params[0].name, "x");
  EXPECT_EQ(node->params[0].declaredType, TivaType::Int);

  EXPECT_EQ(node->params[1].name, "y");
  EXPECT_EQ(node->params[1].declaredType, TivaType::Float);
}

TEST(ParserFunctionTest, PrototypeInvalidParamTypeReturnsNull) {
  Parser parser{"foo(x: invalid)"};
  auto node = parser.parseFunctionPrototype();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserFunctionTest, PrototypeMissingCommaReturnsNull) {
  Parser parser{"foo(x: i32 y: float)"};
  auto node = parser.parseFunctionPrototype();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserFunctionTest, PrototypeMissingClosingParenReturnsNull) {
  Parser parser{"foo(x: i32"};
  auto node = parser.parseFunctionPrototype();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserFunctionTest, FunctionExpressionBody) {
  Parser parser{"fn add(x: i32, y: i32) -> i32 (x + y)"};
  auto node = parser.parseFunction();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->prototype->name, "add");
  EXPECT_EQ(node->prototype->returnType, TivaType::Int);

  auto* binExpr = assertBinaryExpr(node->body.get(), TokenType::Plus);
  assertVariableNode(binExpr->lhs.get(), "x");
  assertVariableNode(binExpr->rhs.get(), "y");
}

TEST(ParserFunctionTest, FunctionBlockBody) {
  Parser parser{"fn process(x: i32) { let a = 1 }"};
  auto node = parser.parseFunction();
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->prototype->name, "process");
  EXPECT_EQ(node->prototype->returnType, TivaType::Unknown);

  auto* block = assertNode<BlockAstNode>(node->body.get());
  ASSERT_EQ(block->expressions.size(), 1);
}

TEST(ParserFunctionTest, FunctionPrototypeErrorReturnsNull) {
  Parser parser{"fn foo(x: invalid) (1)"};
  auto node = parser.parseFunction();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserFunctionTest, FunctionBodyErrorReturnsNull) {
  Parser parser{"fn foo(x: i32) (x as invalid)"};
  auto node = parser.parseFunction();
  EXPECT_EQ(node, nullptr);
}

TEST(ParserFunctionTest, FunctionMissingKeywordThrows) {
  Parser parser{"foo() { 1 }"};
  EXPECT_THROW((void)parser.parseFunction(), std::logic_error);
}

// Translation Units
TEST(ParserTranslationUnitTest, EmptyTranslationUnit) {
  Parser parser{""};
  auto node = parser.parseTranslationUnit();
  EXPECT_NE(node, nullptr);
  EXPECT_TRUE(node->declarations.empty());
}

TEST(ParserTranslationUnitTest, MultipleFunctions) {
  std::string_view source =
      "fn first(x: i32) -> i32 (1)\n"
      "fn second(a: i32) { let x = a }";
  Parser parser{source};
  auto node = parser.parseTranslationUnit();
  EXPECT_NE(node, nullptr);
  ASSERT_EQ(node->declarations.size(), 2);

  auto* func1 = assertNode<Function>(node->declarations[0].get());
  EXPECT_EQ(func1->prototype->name, "first");

  auto* func2 = assertNode<Function>(node->declarations[1].get());
  EXPECT_EQ(func2->prototype->name, "second");
}

TEST(ParserTranslationUnitTest, NonFunctionAtTopLevelErrorBreaks) {
  std::string_view source =
      "fn first(x: i32) (1)\n"
      "let x = 5\n"
      "fn second(y: i32) (2)";
  Parser parser{source};
  auto node = parser.parseTranslationUnit();
  EXPECT_NE(node, nullptr);
  ASSERT_EQ(node->declarations.size(), 1);
  auto* func = assertNode<Function>(node->declarations[0].get());
  EXPECT_EQ(func->prototype->name, "first");
}

// Direct Parser Methods
TEST(ParserDirectMethodTest, ParsePrimaryDelegation) {
  {
    Parser parser{"myVar"};
    auto node = parser.parsePrimary();
    assertVariableNode(node.get(), "myVar");
  }
  {
    Parser parser{"123"};
    auto node = parser.parsePrimary();
    assertIntegerNode(node.get(), 123);
  }
  {
    Parser parser{"true"};
    auto node = parser.parsePrimary();
    assertBooleanNode(node.get(), true);
  }
  {
    Parser parser{"(42)"};
    auto node = parser.parsePrimary();
    assertIntegerNode(node.get(), 42);
  }
  {
    Parser parser{"{ let x = 5 }"};
    auto node = parser.parsePrimary();
    auto* block = assertNode<BlockAstNode>(node.get());
    EXPECT_EQ(block->expressions.size(), 1);
  }
  {
    Parser parser{"if cond a else b"};
    auto node = parser.parsePrimary();
    auto* ifElse = assertNode<IfElseAstNode>(node.get());
    assertVariableNode(ifElse->condition.get(), "cond");
  }
  {
    Parser parser{"let x = 42"};
    auto node = parser.parsePrimary();
    auto* letNode = assertNode<LetAstNode>(node.get());
    EXPECT_EQ(letNode->varName, "x");
  }
}

TEST(ParserDirectMethodTest, ParseGlobalDeclaration) {
  {
    Parser parser{"fn foo(x: i32) (1)"};
    auto node = parser.parseGlobalDeclaration();
    auto* func = assertNode<Function>(node.get());
    EXPECT_EQ(func->prototype->name, "foo");
  }
  {
    Parser parser{"let x = 5"};
    auto node = parser.parseGlobalDeclaration();
    EXPECT_EQ(node, nullptr);
  }
}

TEST(ParserDirectMethodTest, ParseBinaryExpressionRhsDirect) {
  {
    Parser parser{"1 + 2"};
    auto lhs = parser.parsePrimary();
    auto node = parser.parseBinaryExpressionRhs(15, std::move(lhs));
    assertIntegerNode(node.get(), 1);
    EXPECT_TRUE(parser.peek(TokenType::Plus));
  }
  {
    Parser parser{"1 + 2"};
    auto lhs = parser.parsePrimary();
    auto node = parser.parseBinaryExpressionRhs(5, std::move(lhs));
    auto* binExpr = assertBinaryExpr(node.get(), TokenType::Plus);
    assertIntegerNode(binExpr->lhs.get(), 1);
    assertIntegerNode(binExpr->rhs.get(), 2);
  }
}

TEST(ParserDirectMethodTest, ParseCallDirect) {
  Parser parser{"(1, 2)"};
  auto callee = std::make_unique<VariableAstNode>("myFunc");
  auto node = parser.parseCall(std::move(callee));
  auto* call = assertNode<CallAstNode>(node.get());
  assertVariableNode(call->toCall.get(), "myFunc");
  ASSERT_EQ(call->args.size(), 2);
  assertIntegerNode(call->args[0].get(), 1);
  assertIntegerNode(call->args[1].get(), 2);
}

TEST(ParserDirectMethodTest, ParseCastDirect) {
  Parser parser{"as float"};
  auto operand = std::make_unique<VariableAstNode>("x");
  auto node = parser.parseCast(std::move(operand));
  auto* cast = assertNode<CastAstNode>(node.get());
  assertVariableNode(cast->operand.get(), "x");
  EXPECT_EQ(cast->targetType, TivaType::Float);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, bugprone-argument-comment, readability-magic-numbers, modernize-use-std-numbers)
