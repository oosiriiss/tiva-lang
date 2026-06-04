#include "types.hpp"

#include <llvm/ADT/APInt.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include <stdexcept>

#include "semantic/types.hpp"

[[nodiscard]] auto toLlvm(llvm::LLVMContext& ctx, TivaType type)
    -> llvm::Type* {
  switch (type) {
    case TivaType::Int:
      return llvm::Type::getIntNTy(ctx, typing::INT_BITS);
    case TivaType::Float:
      return llvm::Type::getDoubleTy(ctx);
    case TivaType::Boolean:
      return llvm::Type::getInt1Ty(ctx);
      break;
    case TivaType::Unknown:
      [[fallthrough]];
    default:
      throw std::invalid_argument(
          std::format("Cannot convert tiva's type '{}' to llvm's type", type));
  }
}
