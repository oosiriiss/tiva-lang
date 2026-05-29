#include "types.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include <stdexcept>

#include "semantic/types.hpp"

[[nodiscard]] auto toLlvm(llvm::LLVMContext& ctx, TivaType type)
    -> llvm::Type* {
  switch (type) {
    case TivaType::Int:
      return llvm::Type::getInt32Ty(ctx);
    case TivaType::Float:
      return llvm::Type::getDoubleTy(ctx);
    case TivaType::Unknown:
      throw std::invalid_argument(
          std::format("Cannot convert tiva's type '{}' to llvm's type", type));
  }
}
