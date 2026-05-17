#include "utility.hpp"
#include <array>
#include <gtest/gtest.h>

TEST(CharUtilsTest, IsLetter_TrueForLetters) {
  constexpr std::string_view letters =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (char c : letters) {
    EXPECT_TRUE(util::isLetter(c));
  }
}

TEST(CharUtilsTest, IsLetter_FalseForNonLetters) {
  constexpr std::string_view nonLetters =
      "0123456789 \n\t\r\v\f!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
  for (char c : nonLetters) {
    EXPECT_FALSE(util::isLetter(c));
  }
}

TEST(CharUtilsTest, IsDigit_TrueForDigits) {
  constexpr std::string_view digits = "0123456789";
  for (char c : digits) {
    EXPECT_TRUE(util::isDigit(c));
  }
}

TEST(CharUtilsTest, IsDigit_FalseForNonDigits) {
  constexpr std::string_view nonDigits =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ "
      "\n\t\r\v\f!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
  for (char c : nonDigits) {
    EXPECT_FALSE(util::isDigit(c));
  }
}

TEST(CharUtilsTest, IsWhitespace_TrueForWhitespace) {
  constexpr std::string_view whitespace = " \n\r\t\v\f";
  for (char c : whitespace) {
    EXPECT_TRUE(util::isWhitespace(c));
  }
}

TEST(CharUtilsTest, IsWhitespace_FalseForNonWhitespace) {
  constexpr std::string_view nonWhitespace =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()"
      "_+-=[]{}|;':\",./<>?`~";
  for (char c : nonWhitespace) {
    EXPECT_FALSE(util::isWhitespace(c));
  }
}
