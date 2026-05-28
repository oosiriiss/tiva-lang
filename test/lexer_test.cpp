#include "lexer.hpp"

#include <gtest/gtest.h>

TEST(LexerTest, IsFinished) {
  EXPECT_TRUE(Lexer("").isFinished());
  EXPECT_FALSE(Lexer("a").isFinished());
  EXPECT_FALSE(Lexer("   ").isFinished());
}

TEST(LexerTest, SkipWhitespace_ConsumesAllWhitespace) {
  std::vector<std::string_view> cases = {" ", "\t", "\n", "\r",
                                         " \t \n \r \v \f"};

  for (auto input : cases) {
    Lexer lexer(input);
    lexer.skipWhitespace();
    EXPECT_TRUE(lexer.isFinished());
  }
}

TEST(LexerTest, SkipWhitespace_StopsAtFirstNonWhitespace) {
  Lexer lexer("  \t\n  abc \t");
  lexer.skipWhitespace();
  EXPECT_FALSE(lexer.isFinished());

  auto id = lexer.readIdentifier();
  EXPECT_TRUE(id.has_value());
  EXPECT_EQ(id.value(), "abc");
}

TEST(LexerTest, ReadIdentifier_ValidIdentifiers) {
  std::vector<std::pair<std::string_view, std::string_view>> cases = {
      {"identifier", "identifier"},
      {"camelCase", "camelCase"},
      {"PascalCase", "PascalCase"},
      {"snake_case", "snake_case"},
      {"x = 10", "x"},
      {"var123 = 5", "var123"},
      {"_privateVar", "_privateVar"}};

  for (const auto &[input, expected] : cases) {
    Lexer lexer(input);
    auto result = lexer.readIdentifier();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), expected);
  }
}

TEST(LexerTest, ReadIdentifier_InvalidIdentifiers) {
  std::vector<std::string_view> cases = {"123var", "  var", "!var", "-var", ""};

  for (auto input : cases) {
    Lexer lexer(input);
    EXPECT_FALSE(lexer.readIdentifier().has_value());
  }
}

TEST(LexerTest, ReadSymbol_ValidSymbols) {
  std::vector<std::pair<std::string_view, TokenType>> cases = {
      {"+", TokenType::Plus},     {"-", TokenType::Minus},
      {"*", TokenType::Multiply}, {"/", TokenType::Divide},
      {"+ x", TokenType::Plus},   {"-10", TokenType::Minus}};

  for (const auto &[input, expected] : cases) {
    Lexer lexer(input);
    auto result = lexer.readSymbol();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), expected);
  }
}

TEST(LexerTest, ReadSymbol_InvalidSymbols) {
  std::vector<std::string_view> cases = {"a", "123", "  ", "_var", ""};

  for (auto input : cases) {
    Lexer lexer(input);
    EXPECT_FALSE(lexer.readSymbol().has_value());
  }
}

TEST(LexerTest, ReadNumber_ValidIntegers) {
  std::vector<std::string_view> cases = {"0", "42069", "-67"};

  for (auto input : cases) {
    Lexer lexer(input);
    EXPECT_TRUE(lexer.readNumber().has_value());
  }
}

TEST(LexerTest, ReadNumber_ValidDecimalNumbers) {
  std::vector<std::string_view> cases = {"0.0", "-6.7", ".68", "10."};

  for (auto input : cases) {
    Lexer lexer(input);
    EXPECT_TRUE(lexer.readNumber().has_value());
  }
}

TEST(LexerTest, ReadNumber_InvalidNumbers) {
  std::vector<std::string_view> cases = {"0.0.", "-0-", "0-", "0-6.7", ".6.7"};

  for (auto input : cases) {
    Lexer lexer(input);
    EXPECT_TRUE(lexer.readNumber().has_value());
  }
}

TEST(LexerTest, NextToken_ParsesSymbolSequence) {
  Lexer lexer("+-*/");

  auto t1 = lexer.nextToken();
  EXPECT_EQ(t1.type, TokenType::Plus);
  EXPECT_EQ(t1.value, "+");

  auto t2 = lexer.nextToken();
  EXPECT_EQ(t2.type, TokenType::Minus);
  EXPECT_EQ(t2.value, "-");

  auto t3 = lexer.nextToken();
  EXPECT_EQ(t3.type, TokenType::Multiply);
  EXPECT_EQ(t3.value, "*");

  auto t4 = lexer.nextToken();
  EXPECT_EQ(t4.type, TokenType::Divide);
  EXPECT_EQ(t4.value, "/");
}

TEST(LexerTest, NextToken_AdvancesStateUntilFinished) {
  Lexer lexer("Hello my name is   ");

  int maxIterations = 100;
  int currentIteration = 0;

  while (!lexer.isFinished() && currentIteration < maxIterations) {
    [[maybe_unused]] auto token = lexer.nextToken();
    currentIteration++;
  }

  EXPECT_TRUE(lexer.isFinished());
  EXPECT_LT(currentIteration, maxIterations);
}

// ---------------------------------------------------------
// NOWE TESTY - DO DODANIA NA KONIEC PLIKU
// ---------------------------------------------------------

TEST(LexerTest, ReadSymbol_Parentheses) {
  std::vector<std::pair<std::string_view, TokenType>> cases = {
      {"(", TokenType::ParenBegin},
      {")", TokenType::ParenEnd},
      {"(x", TokenType::ParenBegin},
      {") +", TokenType::ParenEnd}};

  for (const auto &[input, expected] : cases) {
    Lexer lexer(input);
    auto result = lexer.readSymbol();
    ASSERT_TRUE(result.has_value())
        << "Failed to parse symbol for input: " << input;
    EXPECT_EQ(result.value(), expected);
  }
}

TEST(LexerTest, NextToken_ParsesParentheses) {
  Lexer lexer("() ( )");

  auto t1 = lexer.nextToken();
  EXPECT_EQ(t1.type, TokenType::ParenBegin);
  EXPECT_EQ(t1.value, "(");

  auto t2 = lexer.nextToken();
  EXPECT_EQ(t2.type, TokenType::ParenEnd);
  EXPECT_EQ(t2.value, ")");

  auto t3 = lexer.nextToken();
  EXPECT_EQ(t3.type, TokenType::ParenBegin);
  EXPECT_EQ(t3.value, "(");

  auto t4 = lexer.nextToken();
  EXPECT_EQ(t4.type, TokenType::ParenEnd);
  EXPECT_EQ(t4.value, ")");
}

TEST(LexerTest, NextToken_ParsesMixedExpression) {
  Lexer lexer("( foo + 42 ) * 3.14 / bar");

  struct ExpectedToken {
    TokenType type;
    std::string_view value;
  };

  std::vector<ExpectedToken> expected = {
      {TokenType::ParenBegin, "("},  {TokenType::Identifier, "foo"},
      {TokenType::Plus, "+"},        {TokenType::Number, "42"},
      {TokenType::ParenEnd, ")"},    {TokenType::Multiply, "*"},
      {TokenType::Number, "3.14"},   {TokenType::Divide, "/"},
      {TokenType::Identifier, "bar"}};

  for (size_t i = 0; i < expected.size(); ++i) {
    auto token = lexer.nextToken();
    EXPECT_EQ(token.type, expected[i].type)
        << "Mismatch at index " << i << " (value: " << expected[i].value << ")";
    EXPECT_EQ(token.value, expected[i].value) << "Mismatch at index " << i;
  }

  EXPECT_TRUE(lexer.isFinished());
}

TEST(LexerTest, NextToken_ExpressionWithoutSpaces) {
  Lexer lexer("a+12.5*b");

  struct ExpectedToken {
    TokenType type;
    std::string_view value;
  };

  std::vector<ExpectedToken> expected = {
      {.type = TokenType::Identifier, .value = "a"},
      {.type = TokenType::Plus, .value = "+"},
      {.type = TokenType::Number, .value = "12.5"},
      {.type = TokenType::Multiply, .value = "*"},
      {.type = TokenType::Identifier, .value = "b"}};

  for (size_t i = 0; i < expected.size(); ++i) {
    auto token = lexer.nextToken();
    EXPECT_EQ(token.type, expected[i].type) << "Mismatch at index " << i;
    EXPECT_EQ(token.value, expected[i].value) << "Mismatch at index " << i;
  }

  EXPECT_TRUE(lexer.isFinished());
}
