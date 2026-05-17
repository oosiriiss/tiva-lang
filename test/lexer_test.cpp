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
