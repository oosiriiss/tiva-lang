#include "lexer/lexer.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string_view>
#include <vector>

namespace {
  struct ExpectedToken {
    TokenType type;
    std::string_view value;
  };
}  // namespace

TEST(LexerTest, ParseKeyword_ValidKeywords) {
  struct TestCase {
    std::string_view input;
    TokenType expectedType;
  };

  const std::vector<TestCase> cases = {
      {.input = "fn", .expectedType = TokenType::Function},
      {.input = "if", .expectedType = TokenType::If},
      {.input = "else", .expectedType = TokenType::Else},
      {.input = "let", .expectedType = TokenType::Let},
      {.input = "true", .expectedType = TokenType::Boolean},
      {.input = "false", .expectedType = TokenType::Boolean},
      {.input = "as", .expectedType = TokenType::As}};

  for (const auto& cse : cases) {
    auto tokenOpt = Lexer::parseKeyword(cse.input);
    ASSERT_TRUE(tokenOpt.has_value())
        << "Failed to parse keyword: " << cse.input;
    EXPECT_EQ(tokenOpt->type, cse.expectedType);
    EXPECT_EQ(tokenOpt->value, cse.input);
  }
}

TEST(LexerTest, ParseKeyword_InvalidKeywords) {
  const std::vector<std::string_view> cases = {
      "fna", "iff", "else_", "let1", "true_val", "falsey", "ask", "random",
      "FN",  "If",  "LET",   "TRUE", "FALSE",    "As",     ""};

  for (const auto& input : cases) {
    auto tokenOpt = Lexer::parseKeyword(input);
    EXPECT_FALSE(tokenOpt.has_value())
        << "Incorrectly parsed keyword: " << input;
  }
}

TEST(LexerTest, ReadSymbol_ValidSymbols) {
  struct TestCase {
    std::string_view input;
    TokenType expectedType;
    std::string_view expectedValue;
  };

  const std::vector<TestCase> cases = {
      {.input = "+", .expectedType = TokenType::Plus, .expectedValue = "+"},
      {.input = "-", .expectedType = TokenType::Minus, .expectedValue = "-"},
      {.input = "->", .expectedType = TokenType::Arrow, .expectedValue = "->"},
      {.input = "*", .expectedType = TokenType::Multiply, .expectedValue = "*"},
      {.input = "/", .expectedType = TokenType::Divide, .expectedValue = "/"},
      {.input = "(",
       .expectedType = TokenType::ParenBegin,
       .expectedValue = "("},
      {.input = ")", .expectedType = TokenType::ParenEnd, .expectedValue = ")"},
      {.input = ",", .expectedType = TokenType::Comma, .expectedValue = ","},
      {.input = "{",
       .expectedType = TokenType::CurlyBegin,
       .expectedValue = "{"},
      {.input = "}", .expectedType = TokenType::CurlyEnd, .expectedValue = "}"},
      {.input = "=", .expectedType = TokenType::Assign, .expectedValue = "="},
      {.input = ":", .expectedType = TokenType::Colon, .expectedValue = ":"}};

  for (const auto& cse : cases) {
    Lexer lexer(cse.input);
    auto tokenOpt = lexer.readSymbol();
    ASSERT_TRUE(tokenOpt.has_value()) << "Failed to read symbol: " << cse.input;
    EXPECT_EQ(tokenOpt->type, cse.expectedType);
    EXPECT_EQ(tokenOpt->value, cse.expectedValue);
  }
}

TEST(LexerTest, ReadSymbol_PrefixMatchingAndRemainingSource) {
  // Arrow prefix vs Minus
  {
    Lexer lexer("->xyz");
    auto tokenOpt = lexer.readSymbol();
    ASSERT_TRUE(tokenOpt.has_value());
    EXPECT_EQ(tokenOpt->type, TokenType::Arrow);
    EXPECT_EQ(tokenOpt->value, "->");
  }
  {
    Lexer lexer("-xyz");
    auto tokenOpt = lexer.readSymbol();
    ASSERT_TRUE(tokenOpt.has_value());
    EXPECT_EQ(tokenOpt->type, TokenType::Minus);
    EXPECT_EQ(tokenOpt->value, "-");
  }
}

TEST(LexerTest, ReadSymbol_InvalidSymbols) {
  const std::vector<std::string_view> cases = {
      "a", "1", " ", "_", "@", "#", "$", "%", "^", "&",
      "?", "<", ">", "[", "]", ";", "~", "`", ""};

  for (const auto& input : cases) {
    Lexer lexer(input);
    auto tokenOpt = lexer.readSymbol();
    EXPECT_FALSE(tokenOpt.has_value())
        << "Incorrectly read symbol for input: " << input;
  }
}

TEST(LexerTest, ReadNumber_ValidNumbers) {
  struct TestCase {
    std::string_view input;
    std::string_view expectedValue;
  };

  const std::vector<TestCase> cases = {
      {.input = "0", .expectedValue = "0"},
      {.input = "42", .expectedValue = "42"},
      {.input = "1234567890", .expectedValue = "1234567890"},
      {.input = "-50", .expectedValue = "-50"},
      {.input = "3.14", .expectedValue = "3.14"},
      {.input = ".5", .expectedValue = ".5"},
      {.input = "0.0", .expectedValue = "0.0"},
      {.input = "10.", .expectedValue = "10."},
      {.input = "-0.123", .expectedValue = "-0.123"},
      {.input = "-.99", .expectedValue = "-.99"}};

  for (const auto& cse : cases) {
    Lexer lexer(cse.input);
    auto numOpt = lexer.readNumber();
    ASSERT_TRUE(numOpt.has_value()) << "Failed to read number: " << cse.input;
    EXPECT_EQ(*numOpt, cse.expectedValue);
  }
}

TEST(LexerTest, ReadNumber_PartialAndInvalidNumbers) {
  // readNumber stops at non-digits or extra decimal points
  {
    Lexer lexer("123a");
    auto numOpt = lexer.readNumber();
    ASSERT_TRUE(numOpt.has_value());
    EXPECT_EQ(*numOpt, "123");
  }
  {
    Lexer lexer("3.14.15");
    auto numOpt = lexer.readNumber();
    ASSERT_TRUE(numOpt.has_value());
    EXPECT_EQ(*numOpt, "3.14");  // reads up to first dot and its decimal digits
  }
  {
    Lexer lexer("-0-");
    auto numOpt = lexer.readNumber();
    ASSERT_TRUE(numOpt.has_value());
    EXPECT_EQ(*numOpt, "-0");
  }
  {
    Lexer lexer("abc");
    auto numOpt = lexer.readNumber();
    EXPECT_FALSE(numOpt.has_value());
  }
  {
    Lexer lexer("");
    auto numOpt = lexer.readNumber();
    EXPECT_FALSE(numOpt.has_value());
  }
}

TEST(LexerTest, ReadIdentifier_ValidIdentifiers) {
  struct TestCase {
    std::string_view input;
    std::string_view expectedValue;
  };

  const std::vector<TestCase> cases = {
      {.input = "name", .expectedValue = "name"},
      {.input = "_private", .expectedValue = "_private"},
      {.input = "camelCase", .expectedValue = "camelCase"},
      {.input = "snake_case", .expectedValue = "snake_case"},
      {.input = "PascalCase", .expectedValue = "PascalCase"},
      {.input = "var123", .expectedValue = "var123"},
      {.input = "_123", .expectedValue = "_123"}};

  for (const auto& cse : cases) {
    Lexer lexer(cse.input);
    auto idOpt = lexer.readIdentifier();
    ASSERT_TRUE(idOpt.has_value())
        << "Failed to read identifier: " << cse.input;
    EXPECT_EQ(*idOpt, cse.expectedValue);
  }
}

TEST(LexerTest, ReadIdentifier_InvalidAndPartialIdentifiers) {
  // Cannot start with a digit
  {
    Lexer lexer("123var");
    auto idOpt = lexer.readIdentifier();
    EXPECT_FALSE(idOpt.has_value());
  }
  // Stop at invalid identifier char
  {
    Lexer lexer("var-name");
    auto idOpt = lexer.readIdentifier();
    ASSERT_TRUE(idOpt.has_value());
    EXPECT_EQ(*idOpt, "var");
  }
  {
    Lexer lexer("x+y");
    auto idOpt = lexer.readIdentifier();
    ASSERT_TRUE(idOpt.has_value());
    EXPECT_EQ(*idOpt, "x");
  }
  {
    Lexer lexer("  leading_space");
    auto idOpt = lexer.readIdentifier();
    EXPECT_FALSE(idOpt.has_value());
  }
  {
    Lexer lexer("");
    auto idOpt = lexer.readIdentifier();
    EXPECT_FALSE(idOpt.has_value());
  }
}

TEST(LexerTest, State_IsFinished) {
  EXPECT_TRUE(Lexer("").isFinished());
  EXPECT_FALSE(Lexer("a").isFinished());
  EXPECT_FALSE(Lexer("   ").isFinished());
}

TEST(LexerTest, SkipWhitespace_ConsumesAllWhitespace) {
  const std::vector<std::string_view> cases = {" ", "\t", "\n", "\r",
                                               " \t \n \r \v \f"};

  for (const auto& input : cases) {
    Lexer lexer(input);
    lexer.skipWhitespace();
    EXPECT_TRUE(lexer.isFinished())
        << "Failed to consume all whitespace for: " << input;
  }
}

TEST(LexerTest, SkipWhitespace_StopsAtFirstNonWhitespace) {
  Lexer lexer("  \t\n  abc \t");
  lexer.skipWhitespace();
  EXPECT_FALSE(lexer.isFinished());

  auto idOpt = lexer.readIdentifier();
  ASSERT_TRUE(idOpt.has_value());
  EXPECT_EQ(*idOpt, "abc");
}

TEST(LexerTest, NextToken_EOF_OnEmpty) {
  Lexer lexer("");
  auto token = lexer.nextToken();
  EXPECT_EQ(token.type, TokenType::Eof);
  EXPECT_EQ(token.value, "");
  EXPECT_TRUE(lexer.isFinished());
}

TEST(LexerTest, NextToken_ParsesKeywordsAndIdentifiers) {
  Lexer lexer("let myVar = fn if else true false as unknown_id");

  const std::vector<ExpectedToken> expected = {
      {.type = TokenType::Let, .value = "let"},
      {.type = TokenType::Identifier, .value = "myVar"},
      {.type = TokenType::Assign, .value = "="},
      {.type = TokenType::Function, .value = "fn"},
      {.type = TokenType::If, .value = "if"},
      {.type = TokenType::Else, .value = "else"},
      {.type = TokenType::Boolean, .value = "true"},
      {.type = TokenType::Boolean, .value = "false"},
      {.type = TokenType::As, .value = "as"},
      {.type = TokenType::Identifier, .value = "unknown_id"}};

  for (size_t i = 0; i < expected.size(); ++i) {
    auto token = lexer.nextToken();
    EXPECT_EQ(token.type, expected[i].type) << "Mismatch type at index " << i;
    EXPECT_EQ(token.value, expected[i].value)
        << "Mismatch value at index " << i;
  }

  auto finalToken = lexer.nextToken();
  EXPECT_EQ(finalToken.type, TokenType::Eof);
}

TEST(LexerTest, NextToken_ParsesSymbols) {
  Lexer lexer("+ - -> * / ( ) , { } = :");

  const std::vector<ExpectedToken> expected = {
      {.type = TokenType::Plus, .value = "+"},
      {.type = TokenType::Minus, .value = "-"},
      {.type = TokenType::Arrow, .value = "->"},
      {.type = TokenType::Multiply, .value = "*"},
      {.type = TokenType::Divide, .value = "/"},
      {.type = TokenType::ParenBegin, .value = "("},
      {.type = TokenType::ParenEnd, .value = ")"},
      {.type = TokenType::Comma, .value = ","},
      {.type = TokenType::CurlyBegin, .value = "{"},
      {.type = TokenType::CurlyEnd, .value = "}"},
      {.type = TokenType::Assign, .value = "="},
      {.type = TokenType::Colon, .value = ":"}};

  for (size_t i = 0; i < expected.size(); ++i) {
    auto token = lexer.nextToken();
    EXPECT_EQ(token.type, expected[i].type) << "Mismatch type at index " << i;
    EXPECT_EQ(token.value, expected[i].value)
        << "Mismatch value at index " << i;
  }

  auto finalToken = lexer.nextToken();
  EXPECT_EQ(finalToken.type, TokenType::Eof);
}

TEST(LexerTest, NextToken_HandlesUnknownInput) {
  // Unknown characters like '@'
  Lexer lexer("x @");

  // x
  auto token1 = lexer.nextToken();
  EXPECT_EQ(token1.type, TokenType::Identifier);
  EXPECT_EQ(token1.value, "x");

  // @ -> Unknown token (it should return source up to SOURCE_FORESEE_LENGTH)
  auto token2 = lexer.nextToken();
  EXPECT_EQ(token2.type, TokenType::Unknown);
  EXPECT_TRUE(token2.value.starts_with("@"));

  // Calling nextToken again continues to try parsing at the unknown token '@'
  // because the source is not advanced on unknown inputs.
  auto token3 = lexer.nextToken();
  EXPECT_EQ(token3.type, TokenType::Unknown);
  EXPECT_TRUE(token3.value.starts_with("@"));
}

TEST(LexerTest, NextToken_ParsesComplexProgram) {
  Lexer lexer(
      "fn add(a: Number, b: Number) -> Number {\n"
      "    let sum = a + b\n"
      "    if sum {\n"
      "        true\n"
      "    } else {\n"
      "        false\n"
      "    }\n"
      "}");

  const std::vector<ExpectedToken> expected = {
      {.type = TokenType::Function, .value = "fn"},
      {.type = TokenType::Identifier, .value = "add"},
      {.type = TokenType::ParenBegin, .value = "("},
      {.type = TokenType::Identifier, .value = "a"},
      {.type = TokenType::Colon, .value = ":"},
      {.type = TokenType::Identifier, .value = "Number"},
      {.type = TokenType::Comma, .value = ","},
      {.type = TokenType::Identifier, .value = "b"},
      {.type = TokenType::Colon, .value = ":"},
      {.type = TokenType::Identifier, .value = "Number"},
      {.type = TokenType::ParenEnd, .value = ")"},
      {.type = TokenType::Arrow, .value = "->"},
      {.type = TokenType::Identifier, .value = "Number"},
      {.type = TokenType::CurlyBegin, .value = "{"},

      {.type = TokenType::Let, .value = "let"},
      {.type = TokenType::Identifier, .value = "sum"},
      {.type = TokenType::Assign, .value = "="},
      {.type = TokenType::Identifier, .value = "a"},
      {.type = TokenType::Plus, .value = "+"},
      {.type = TokenType::Identifier, .value = "b"},

      {.type = TokenType::If, .value = "if"},
      {.type = TokenType::Identifier, .value = "sum"},
      {.type = TokenType::CurlyBegin, .value = "{"},

      {.type = TokenType::Boolean, .value = "true"},

      {.type = TokenType::CurlyEnd, .value = "}"},
      {.type = TokenType::Else, .value = "else"},
      {.type = TokenType::CurlyBegin, .value = "{"},

      {.type = TokenType::Boolean, .value = "false"},

      {.type = TokenType::CurlyEnd, .value = "}"},
      {.type = TokenType::CurlyEnd, .value = "}"}};

  for (size_t i = 0; i < expected.size(); ++i) {
    auto token = lexer.nextToken();
    EXPECT_EQ(token.type, expected[i].type)
        << "Mismatch type at index " << i << ", got " << toString(token.type);
    EXPECT_EQ(token.value, expected[i].value)
        << "Mismatch value at index " << i;
  }

  EXPECT_EQ(lexer.nextToken().type, TokenType::Eof);
}
