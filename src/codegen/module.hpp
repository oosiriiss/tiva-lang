#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct CompilerState {

public:
  void printIr();
  void runOptimizations();
  void emitObjectFile(std::string_view fileName);

private:
  struct OptimizeState;

public:
  CompilerState();
  ~CompilerState();

  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<OptimizeState> optimizeState;
};
