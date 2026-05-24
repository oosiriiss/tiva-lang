#include "ast_nodes.hpp"
#include "debug.hpp"
#include "lexer.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/PassInstrumentation.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <logzy/logzy.hpp>
#include <utility>

static std::unique_ptr<llvm::LLVMContext> llvmContext;
static std::unique_ptr<llvm::IRBuilder<llvm::NoFolder>> llvmBuilder;
static std::unique_ptr<llvm::Module> llvmModule;
static std::unique_ptr<llvm::FunctionPassManager> llvmFunctionPassManager;
static std::unique_ptr<llvm::ModuleAnalysisManager> llvmModuleAnalysisManager;
static std::unique_ptr<llvm::CGSCCAnalysisManager> llvmCallGraphAnalysisManager;
static std::unique_ptr<llvm::FunctionAnalysisManager>
    llvmFunctionAnalysisManager;
static std::unique_ptr<llvm::LoopAnalysisManager> llvmLoopAnalysisManager;

static std::unique_ptr<llvm::PassInstrumentationCallbacks>
    llmvPassInstrumentationCallbacks;
static std::unique_ptr<llvm::StandardInstrumentations> llvmPassInstrumentation;

static std::vector<std::unordered_map<std::string, llvm::AllocaInst *>>
    scopeValues;

static std::unordered_map<std::string, llvm::AllocaInst *> *currentScope;

static void printScopeValues() {
  for (auto &[k, v] : scopeValues.back()) {
    llvm::errs() << "(" << k << "," << v << ")\n";
  }
}

void pushScope() {
  // There should be at least one scope - global scope
  scopeValues.emplace_back(
      scopeValues.back()); // Copying last scope, so that all the variables
                           // defined there will be defined in current scope

  std::cout << "Scope values:";
  printScopeValues();

  currentScope = &scopeValues.back();
}

void popScope() {

  std::cout << "Scope values (before popping scope):";
  printScopeValues();

  scopeValues.pop_back();
  DEBUG_ASSERT(
      scopeValues.size() >= 1,
      "There should always be at least 1 scope on the stack (global scope)");
  currentScope = &scopeValues.back();
}

// Allocates variable on the stack in the entry block of the function
static llvm::AllocaInst *entryBlockAlloca(llvm::Function *function,
                                          llvm::StringRef variableName) {
  llvm::IRBuilder<> entryBuilder(&function->getEntryBlock(),
                                 function->getEntryBlock().begin());

  return entryBuilder.CreateAlloca(llvm::Type::getInt32Ty(*llvmContext),
                                   nullptr, variableName);
}

void initalizeLlvmModule() {
  llvmContext = std::make_unique<llvm::LLVMContext>();
  llvmModule = std::make_unique<llvm::Module>("Main Module", *llvmContext);
  llvmBuilder = std::make_unique<llvm::IRBuilder<llvm::NoFolder>>(*llvmContext);

  llvmModuleAnalysisManager = std::make_unique<llvm::ModuleAnalysisManager>();
  llvmCallGraphAnalysisManager = std::make_unique<llvm::CGSCCAnalysisManager>();
  llvmFunctionAnalysisManager =
      std::make_unique<llvm::FunctionAnalysisManager>();
  llvmLoopAnalysisManager = std::make_unique<llvm::LoopAnalysisManager>();

  llmvPassInstrumentationCallbacks =
      std::make_unique<llvm::PassInstrumentationCallbacks>();
  constexpr bool debugLogging = true;
  llvmPassInstrumentation = std::make_unique<llvm::StandardInstrumentations>(
      *llvmContext, debugLogging);
  llvmPassInstrumentation->registerCallbacks(*llmvPassInstrumentationCallbacks);

  // Transform passes
  llvmFunctionPassManager = std::make_unique<llvm::FunctionPassManager>();
  llvmFunctionPassManager->addPass(
      llvm::PromotePass()); // mem2reg: promote allocas to registers
  llvmFunctionPassManager->addPass(
      llvm::InstCombinePass()); // reduces instruction count
  llvmFunctionPassManager->addPass(
      llvm::ReassociatePass()); // rearranges expression trees so that they can
                                // be more efficiently grouped together i.e.
                                // before: "4 + (x + 5)" after: "x + (4+5) -> x
                                // + 9"
  llvmFunctionPassManager->addPass(
      llvm::GVNPass()); // checks if we are recalculating the same expression,
                        // if yes makes the result be reused
  llvmFunctionPassManager->addPass(
      llvm::SimplifyCFGPass()); // remove dead code, merge blocks that dont need
                                // to be separated, simplifies control flow
                                // graph

  // Analysis passes
  llvm::PassBuilder passBuilder;
  passBuilder.registerModuleAnalyses(*llvmModuleAnalysisManager);
  passBuilder.registerFunctionAnalyses(*llvmFunctionAnalysisManager);
  passBuilder.crossRegisterProxies(
      *llvmLoopAnalysisManager, *llvmFunctionAnalysisManager,
      *llvmCallGraphAnalysisManager, *llvmModuleAnalysisManager);

  // Global scope
  scopeValues.emplace_back(
      std::unordered_map<std::string, llvm::AllocaInst *>());
  currentScope = &scopeValues.back();
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

  logzy::info("Registering emit pass");
  llvm::legacy::PassManager pass;
  auto outputType = llvm::CodeGenFileType::ObjectFile;

  if (targetMachine->addPassesToEmitFile(pass, destination, nullptr,
                                         outputType)) {
    logzy::error("Couldnt register pass to emit file");
    return;
  }

  logzy::info("Running emit pass");
  pass.run(*llvmModule);
  logzy::info("Flushing file stream");
  destination.flush();
}

auto NumberAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for number '{}'", val);
  return llvm::ConstantInt::get(*llvmContext, llvm::APInt(32, val, true));
}
auto VariableAstNode::codegen() const -> llvm::Value * {
  logzy::trace("generating code for variable '{}' (r-value)", name);
  auto val = currentScope->find(name);
  if (val == currentScope->end()) {
    logzy::warn("variable not found in this scope");
    return nullptr;
  }

  // Generates a value (R-value)
  return llvmBuilder->CreateLoad(val->second->getAllocatedType(), val->second,
                                 name.c_str());
}

auto VariableAstNode::codegenLValue() const -> llvm::Value * {
  logzy::trace("generating code for variable '{}' (l-value)", name);
  auto val = currentScope->find(name);
  if (val == currentScope->end()) {
    logzy::warn("variable not found in this scope");
    return nullptr;
  }

  // Generates a value (R-value)
  return val->second;
}

auto AssignmentAstNode::codegen() const -> llvm::Value * {
  logzy::trace("Generating code for assignemnt of var: '{}'", var->name);
  auto lhs = var->codegenLValue();
  auto rhsValue = this->rhs->codegen();

  if (rhsValue == nullptr) {
    logzy::error("Couldn't generate code for assginemnt rhs");
    return nullptr;
  }
  llvmBuilder->CreateStore(rhsValue, lhs);

  return rhsValue;
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

  // Special case for assginemnts

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

auto BlockAstNode::codegen() const -> llvm::Value * {
  logzy::trace("Generating code for block with {} expressions",
               expressions.size());
  llvm::Value *lastReturnValue = nullptr;

  pushScope();

  for (auto &expr : expressions) {
    lastReturnValue = expr->codegen();

    if (lastReturnValue == nullptr) {
      logzy::error("Failed to generate code for expression");
      return nullptr;
    }
  }

  // Block empty
  if (lastReturnValue == nullptr) {
    logzy::error("Empty block encountered");
    return nullptr;
  }

  popScope();

  return lastReturnValue;
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
  logzy::debug("Generating code for function '{}'", prototype->name);
  llvm::Function *func = llvmModule->getFunction(prototype->name);
  if (func == nullptr) {
    logzy::debug("Generating code for function '{}' prototype",
                 prototype->name);
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

  // currentScope->clear();

  pushScope(); // ??? double scopes for function and body
  for (auto &arg : func->args()) {
    auto allocated = entryBlockAlloca(func, arg.getName());
    llvmBuilder->CreateStore(&arg, allocated);
    (*currentScope)[std::string(arg.getName())] = allocated;
  }

  logzy::debug("Generating code for function '{}' body", prototype->name);
  auto *returnValue = body->codegen();
  popScope();

  if (returnValue == nullptr) {
    logzy::error("Function didn't return value");
    func->eraseFromParent();
    return nullptr;
  }

  llvmBuilder->CreateRet(returnValue);

  llvm::verifyFunction(*func);

  llvmFunctionPassManager->run(*func, *llvmFunctionAnalysisManager);
  return func;
}
