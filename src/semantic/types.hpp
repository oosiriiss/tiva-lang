#pragma once

#include <format>
#include <string_view>
#include <vector>

namespace llvm {
  struct Type;
  struct LLVMContext;
}  // namespace llvm

namespace typing {
  constexpr auto INT_BITS = 32;
  constexpr auto BOOL_BITS = 1;
}  // namespace typing

enum class TivaType : std::uint8_t {
  Int,
  Float,
  Boolean,
  Unknown,
};


[[nodiscard]] auto toLlvm(llvm::LLVMContext& ctx, TivaType type) -> llvm::Type*;

struct Parameter {
  std::string name;
  TivaType declaredType;
};

struct FunctionSignature {
  std::string name;
  std::vector<Parameter> parameters;
  TivaType returnType;
};

[[nodiscard]] constexpr auto fromString(std::string_view type) noexcept
    -> TivaType {
  if (type == "i32") {
    return TivaType::Int;
  }
  if (type == "float") {
    return TivaType::Float;
  }

  if (type == "bool") {
    return TivaType::Boolean;
  }

  return TivaType::Unknown;
}

template <>
struct std::formatter<TivaType> : std::formatter<std::string_view> {
  static auto format(TivaType type, std::format_context& ctx) {
    std::string_view name;

#define TYPE_FORMAT(enumName) \
  case TivaType::enumName:    \
    name = #enumName;         \
    break

    switch (type) {
      TYPE_FORMAT(Int);
      TYPE_FORMAT(Float);
      TYPE_FORMAT(Boolean);

      case TivaType::Unknown:
        throw std::invalid_argument(std::format(
            "No formatter for type TivaType.int={}", static_cast<int>(type)));
    }

#undef TYPE_FORMAT
    return std::format_to(ctx.out(), "{}", name);
  }
};
