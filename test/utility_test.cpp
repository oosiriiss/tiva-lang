#include "utility/utility.hpp"
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <stdexcept>

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

TEST(ToNumberTest, ParsesPositiveIntegers) {
  EXPECT_EQ(util::toNumber<int>("12345"), 12345);
  EXPECT_EQ(util::toNumber<int>("0"), 0);
  EXPECT_EQ(util::toNumber<long>("987654321"), 987654321L);
  EXPECT_EQ(util::toNumber<int8_t>("127"), 127);
}

TEST(ToNumberTest, ParsesNegativeIntegers) {
  EXPECT_EQ(util::toNumber<int>("-12345"), -12345);
  EXPECT_EQ(util::toNumber<int>("-0"), 0);
  EXPECT_EQ(util::toNumber<int64_t>("-9876543210"), -9876543210LL);
  EXPECT_EQ(util::toNumber<int8_t>("-128"), -128);
}

TEST(ToNumberTest, ParsesUnsignedIntegers) {
  EXPECT_EQ(util::toNumber<unsigned int>("4294967295"), 4294967295U);
  EXPECT_EQ(util::toNumber<uint8_t>("255"), 255);
  EXPECT_EQ(util::toNumber<uint64_t>("18446744073709551615"),
            18446744073709551615ULL);
}

TEST(ToNumberTest, IntegerHandlesLimits) {
  EXPECT_EQ(util::toNumber<int32_t>("2147483647"), 2147483647);
  EXPECT_EQ(util::toNumber<int32_t>("-2147483648"), -2147483648);
  EXPECT_EQ(util::toNumber<int64_t>("9223372036854775807"),
            9223372036854775807LL);
  EXPECT_EQ(util::toNumber<int64_t>("-9223372036854775808"),
            -9223372036854775807LL - 1);
}

TEST(ToNumberTest, IntegerEdgeCasePartialParsing) {
  EXPECT_EQ(util::toNumber<int>("123abc"), 123);
  EXPECT_EQ(util::toNumber<int>("42.7"), 42);
  EXPECT_EQ(util::toNumber<int>("-99%"), -99);
  EXPECT_EQ(util::toNumber<unsigned int>("404Not Found"), 404U);
}

TEST(ToNumberTest, IntegerEdgeCaseOverflowUnderflow) {
  EXPECT_THROW((void)util::toNumber<int8_t>("999"), std::out_of_range);
  EXPECT_THROW((void)util::toNumber<int32_t>("9999999999999999999"),
               std::out_of_range);
  EXPECT_THROW((void)util::toNumber<uint8_t>("-1"), std::invalid_argument);
}

TEST(ToNumberTest, ParsesFloat) {
  EXPECT_FLOAT_EQ(util::toNumber<float>("3.14"), 3.14f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("0.0"), 0.0f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("42"), 42.0f);
  EXPECT_FLOAT_EQ(util::toNumber<float>(".5"), 0.5f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("42."), 42.0f);
}

TEST(ToNumberTest, ParsesDouble) {
  EXPECT_DOUBLE_EQ(util::toNumber<double>("3.14159265358979323846"),
                   3.14159265358979323846);
  EXPECT_DOUBLE_EQ(util::toNumber<double>("0.0"), 0.0);
}

TEST(ToNumberTest, ParsesNegativeFloatingPoints) {
  EXPECT_FLOAT_EQ(util::toNumber<float>("-2.718"), -2.718f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("-0.0"), -0.0f);
  EXPECT_DOUBLE_EQ(util::toNumber<double>("-99.99"), -99.99);
}

TEST(ToNumberTest, ParsesScientificNotation) {
  EXPECT_FLOAT_EQ(util::toNumber<float>("1.23e4"), 12300.0f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("5.6E-3"), 0.0056f);
  EXPECT_DOUBLE_EQ(util::toNumber<double>("-1e6"), -1000000.0);
  EXPECT_DOUBLE_EQ(util::toNumber<double>("4.2e+2"), 420.0);
}

TEST(ToNumberTest, FloatingPointEdgeCasePartialParsing) {
  EXPECT_FLOAT_EQ(util::toNumber<float>("3.14abc"), 3.14f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("42.5.6"), 42.5f);
  EXPECT_FLOAT_EQ(util::toNumber<float>("1.23e"), 1.23f);
  EXPECT_DOUBLE_EQ(util::toNumber<double>("9.99%"), 9.99);
}

TEST(ToNumberTest, ParsesInfinityAndNaN) {
  EXPECT_TRUE(std::isnan(util::toNumber<float>("nan")));
  EXPECT_TRUE(std::isnan(util::toNumber<float>("-nan")));
  EXPECT_TRUE(std::isinf(util::toNumber<float>("inf")));
  EXPECT_TRUE(std::isinf(util::toNumber<float>("-inf")));
  EXPECT_LT(util::toNumber<float>("-inf"), 0.0f);
}

TEST(ToNumberTest, EdgeCaseInvalidCharacters) {

  EXPECT_THROW((void)util::toNumber<int>("abc"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<int>("abc123"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<int>("-"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<int>("+"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<int>("+-123"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<int>("  42"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("abc"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("e"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("E5"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("."), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("-."), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("+"), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>("  3.14"), std::invalid_argument);
}

TEST(ToNumberTest, EdgeCaseEmptyString) {
  EXPECT_THROW((void)util::toNumber<int>(""), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<unsigned int>(""), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<float>(""), std::invalid_argument);
  EXPECT_THROW((void)util::toNumber<double>(""), std::invalid_argument);
  ;
}
