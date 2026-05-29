#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

struct CompilerState {
 public:
  CompilerState();
  CompilerState(const CompilerState&) = delete;
  CompilerState(CompilerState&&) = delete;
  auto operator=(const CompilerState&) -> CompilerState& = delete;
  auto operator=(CompilerState&&) -> CompilerState& = delete;
  ~CompilerState();

  void printIr() const;
  void runOptimizations() const;
  void emitObjectFile(std::string_view fileName) const;

 private:
  struct OptimizeState;

 public:
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<OptimizeState> optimizeState;
};
