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
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
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
}

void printGeneratedCode() { llvmModule->print(llvm::errs(), nullptr); }

void emitObjectFile(std::string_view fileName) {

  auto targetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

  logzy::info("Emmiting object code for target '{}' to file: '{}'",
              targetTriple.getTriple(), fileName);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  std::string err;
  auto target = llvm::TargetRegistry::lookupTarget(targetTriple, err);

  if (target == nullptr) {
    logzy::error("Couldn't lookup target. {}", err);
    return;
  }

  std::string_view cpu = "generic";
  std::string_view features = "";
  llvm::TargetOptions opts;

  auto targetMachine = target->createTargetMachine(targetTriple, cpu, features,
                                                   opts, llvm::Reloc::PIC_);

  llvmModule->setDataLayout(targetMachine->createDataLayout());
  llvmModule->setTargetTriple(targetTriple);

  std::error_code ec;

  llvm::raw_fd_ostream destination(fileName, ec, llvm::sys::fs::OF_None);
  if (ec) {
    logzy::error("Couldn't open file '{}' for writing. {}", fileName,
                 ec.message());
    return;
  }

  llvm::legacy::PassManager pass;

  auto outputType = llvm::CodeGenFileType::ObjectFile;

  if (targetMachine->addPassesToEmitFile(pass, destination, nullptr,
                                         outputType)) {
    logzy::error("Couldnt register pass to emit file");
    return;
  }

  pass.run(*llvmModule);
  destination.flush();
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
    return llvmBuilder->CreateAdd(left, right, "addtmp");
  case TokenType::Minus:
    return llvmBuilder->CreateSub(left, right, "subtmp");
  case TokenType::Divide:
    return llvmBuilder->CreateSDiv(left, right, "divtmp");
  case TokenType::Multiply:
    return llvmBuilder->CreateMul(left, right, "multmp");
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
