#include <gtest/gtest.h>
#include "lexer.hpp"

TEST(TokenizeTest, Example) {
  auto tokenized = tokenize("x");

  EXPECT_EQ(tokenized, "x Tokenized");
}
