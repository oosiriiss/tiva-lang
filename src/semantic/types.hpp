#pragma once

#include <format>
#include <string_view>

namespace typing {
  constexpr auto INT_BITS = 32;
}

enum class TivaType : std::uint8_t { Int, Float, Unknown };

template <>
struct std::formatter<TivaType> : std::formatter<std::string_view> {
  static auto format(TivaType type, std::format_context &ctx) {
    std::string_view name;

#define TYPE_FORMAT(enumName) \
  case TivaType::enumName:    \
    name = #enumName;         \
    break

    switch (type) {
      TYPE_FORMAT(Int);
      TYPE_FORMAT(Float);

      case TivaType::Unknown:
        throw std::invalid_argument(std::format(
            "No formatter for type TivaType.int={}", static_cast<int>(type)));
    }

#undef TYPE_FORMAT
    return std::format_to(ctx.out(), "{}", name);
  }
};
