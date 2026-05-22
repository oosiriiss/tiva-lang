#include "ast_nodes.hpp"
#include "debug.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
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

void printGeneratedCode() { llvmModule->print(llvm::errs(), nullptr); }

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

auto CallAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for call to '{}' with args.count()={}", toCall,
               args.size());

  llvm::Function *functionToCall = llvmModule->getFunction(toCall);
  if (functionToCall == nullptr) {
    logzy::error("Trying to call undefined function '{}'", toCall);
    return nullptr;
  }

  if (functionToCall->arg_size() != args.size()) {
    logzy::error("Invalid number of arguments to the function '{}'", toCall);
    return nullptr;
  }

  std::vector<llvm::Value *> argValues;
  argValues.reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i) {

    auto *argValue = args[i]->codegen();
    if (argValue == nullptr) {
      logzy::debug("Couldnt generate code for arg[{}] of function {}", i,
                   toCall);
      return nullptr;
    }
    argValues.emplace_back(argValue);
  }

  return llvmBuilder->CreateCall(functionToCall, argValues, "calltmp");
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

auto FunctionPrototype::codegen() const -> llvm::Function * {
  // for now all args are ints
  std::vector<llvm::Type *> argTypes(args.size(),
                                     llvm::Type::getInt32Ty(*llvmContext));
  llvm::FunctionType *funcType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*llvmContext), argTypes, false);
  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, name, *llvmModule);

  size_t idx = 0;
  for (auto &arg : func->args()) {
    DEBUG_ASSERT(idx < args.size(),
                 std::format("idx={} args={}", idx, args.size()));
    arg.setName(args[idx++]);
  }

  return func;
}

auto Function::codegen() const -> llvm::Function * {
  // Checking if function was previously declared.
  llvm::Function *func = llvmModule->getFunction(prototype->name);
  if (func == nullptr) {
    func = prototype->codegen();
  }

  if (func == nullptr) {
    return nullptr;
  }

  if (!func->empty()) {
    logzy::error("Function '{}' redefined", prototype->name);
    return nullptr;
  }

  llvm::BasicBlock *block =
      llvm::BasicBlock::Create(*llvmContext, "entry", func);
  llvmBuilder->SetInsertPoint(block);

  scopeValues.clear();

  for (auto &arg : func->args()) {
    scopeValues[std::string(arg.getName())] = &arg;
  }

  auto *returnValue = body->codegen();
  if (returnValue == nullptr) {
    logzy::error("Function didn't return value");
    func->eraseFromParent();
    return nullptr;
  }

  llvmBuilder->CreateRet(returnValue);

  llvm::verifyFunction(*func);
  return func;
}
