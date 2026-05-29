#include "module.hpp"

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

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct CompilerState::OptimizeState {
  llvm::ModuleAnalysisManager moduleAnalysisManager;
  llvm::CGSCCAnalysisManager callGraphAnalysisManager;
  llvm::LoopAnalysisManager loopAnalysisManager;

  // TODO :: Currently functions are optimized right after definitions. They
  // could be optimized when the code generationm for the module is finished
  llvm::FunctionAnalysisManager functionAnalysisManager;
  llvm::FunctionPassManager functionPassManager;

  llvm::PassInstrumentationCallbacks passInstrumentationCallbacks;
  llvm::StandardInstrumentations passInstrumentation;

  OptimizeState(llvm::LLVMContext &context)
      : passInstrumentation(context, true) {
    passInstrumentation.registerCallbacks(passInstrumentationCallbacks);

    functionPassManager.addPass(
        llvm::PromotePass());  // mem2reg: promote allocas to registers
    functionPassManager.addPass(
        llvm::InstCombinePass());  // reduces instruction count
    functionPassManager.addPass(
        llvm::ReassociatePass());  // rearranges expression trees so that they
                                   // can be more efficiently grouped together
                                   // i.e. before: "4 + (x + 5)" after: "x +
                                   // (4+5) -> x
                                   // + 9"
    functionPassManager.addPass(
        llvm::GVNPass());  // checks if we are recalculating the same
                           // expression, if yes makes the result be reused
    functionPassManager.addPass(
        llvm::SimplifyCFGPass());  // remove dead code, merge blocks that dont
                                   // need to be separated, simplifies control
                                   // flow graph

    llvm::PassBuilder passBuilder;
    passBuilder.registerModuleAnalyses(moduleAnalysisManager);
    passBuilder.registerFunctionAnalyses(functionAnalysisManager);
    passBuilder.crossRegisterProxies(
        loopAnalysisManager, functionAnalysisManager, callGraphAnalysisManager,
        moduleAnalysisManager);
  }
};

CompilerState::CompilerState()
    : context{},
      builder{context},
      module{std::make_unique<llvm::Module>("Main module", context)},
      optimizeState{std::make_unique<OptimizeState>(context)} {}

CompilerState::~CompilerState() = default;

void CompilerState::printIr() const { module->print(llvm::errs(), nullptr); }

void CompilerState::runOptimizations() const {
  for (llvm::Function &func : *module) {
    if (func.isDeclaration()) {
      continue;
    }
    optimizeState->functionPassManager.run(
        func, optimizeState->functionAnalysisManager);
  }
}

void CompilerState::emitObjectFile(std::string_view fileName) const {
  auto targetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());

  logzy::info("Emmiting object code for target '{}' to file: '{}'",
              targetTriple.getTriple(), fileName);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  std::string err;
  const auto *target = llvm::TargetRegistry::lookupTarget(targetTriple, err);

  if (target == nullptr) {
    logzy::error("Couldn't lookup target. {}", err);
    return;
  }

  std::string_view cpu = "generic";
  std::string_view features;
  llvm::TargetOptions opts;

  auto *targetMachine = target->createTargetMachine(targetTriple, cpu, features,
                                                    opts, llvm::Reloc::PIC_);

  module->setDataLayout(targetMachine->createDataLayout());
  module->setTargetTriple(targetTriple);

  std::error_code error;

  llvm::raw_fd_ostream destination(fileName, error, llvm::sys::fs::OF_None);
  if (error) {
    logzy::error("Couldn't open file '{}' for writing. {}", fileName,
                 error.message());
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
  pass.run(*module);
  logzy::info("Flushing file stream");
  destination.flush();
}
