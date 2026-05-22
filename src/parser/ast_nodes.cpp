#include "ast_nodes.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <logzy/logzy.hpp>
#include <utility>

static std::unique_ptr<llvm::LLVMContext> llvmContext;
static std::unique_ptr<llvm::IRBuilder<llvm::NoFolder>> llvmBuilder;
static std::unique_ptr<llvm::Module> llvmModule;
static std::unordered_map<std::string, llvm::Value *> scopeValues;

void initalizeLlvmModule() {
  llvmContext = std::make_unique<llvm::LLVMContext>();
  llvmModule = std::make_unique<llvm::Module>("Main Module", *llvmContext);
  llvmBuilder = std::make_unique<llvm::IRBuilder<llvm::NoFolder>>(*llvmContext);

  llvm::FunctionType *funcType =
      llvm::FunctionType::get(llvm::Type::getDoubleTy(*llvmContext), false);
  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "main", *llvmModule);

  llvm::BasicBlock *entryBlock =
      llvm::BasicBlock::Create(*llvmContext, "entry", func);
  llvmBuilder->SetInsertPoint(entryBlock);
}

void printGeneratedCode() {
  llvmBuilder->CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*llvmContext), 0));
  llvmModule->print(llvm::errs(), nullptr);
}

auto NumberAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for number '{}'", val);
  return llvm::ConstantFP::get(*llvmContext, llvm::APFloat(val));
}
auto VariableAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for variable '{}'", name);
  auto val = scopeValues.find(name);
  if (val == scopeValues.end()) {
    logzy::warn("variable not found in this scope");
    return nullptr;
  }
  return val->second;
}

auto BinaryExprAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for binary expression with operator'{}'", op);

  llvm::Value *left = lhs->codegen();
  llvm::Value *right = rhs->codegen();
  if (lhs == nullptr || rhs == nullptr) {
    logzy::warn("Binary expression has no left or right side");
    return nullptr;
  }

  switch (op) {
  case TokenType::Plus:
    return llvmBuilder->CreateFAdd(left, right, "addtmp");
  case TokenType::Minus:
    return llvmBuilder->CreateFSub(left, right, "subtmp");
  case TokenType::Divide:
    return llvmBuilder->CreateFDiv(left, right, "divtmp");
  case TokenType::Multiply:
    return llvmBuilder->CreateFMul(left, right, "multmp");
  default:
    logzy::error("invalid operator '{}'", op);
    break;
  }
  std::unreachable();
}
