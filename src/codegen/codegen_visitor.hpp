#pragma once
#include "codegen/module.hpp"
#include "debug.hpp"
#include "parser/ast_nodes.hpp"

namespace llvm {
struct AllocaInst;
class Function;
} // namespace llvm

class CodeGenVisitor : public AstNodeVisitor {
public:
  constexpr CodeGenVisitor(CompilerState *state) noexcept : state{state} {}

  void visit(IntegerAstNode *) override;
  void visit(FloatAstNode *) override;
  void visit(VariableAstNode *) override;
  void visit(AssignmentAstNode *) override;
  void visit(CallAstNode *) override;
  void visit(BinaryExprAstNode *) override;
  void visit(BlockAstNode *) override;
  void visit(IfElseAstNode *) override;
  void visit(LetAstNode *) override;
  // void visit(FunctionPrototype *) override; // No need for it to be a node
  void visit(Function *) override;

private:
  using Scope = std::unordered_map<std::string, llvm::AllocaInst *>;

  /**
   * Calls node's accept method and returns ReturnValue
   *
   * Checks if ReturnValue was set, as of now everything is an expression and
   * should set ReturnValue member variable.
   */
  [[nodiscard]] auto generate(AstNode *node) -> llvm::Value *;
  [[nodiscard]] auto generate(std::unique_ptr<AstNode> &node) -> llvm::Value *;

  // TODO :: Current way of sharing the AllocaInst
  // May lead to errors i.e. when two scopes define a variable x
  // {
  // 	let x = 0;
  // 	{
  // 	   let x = 1; // Normally i would like it to shadow the
  // variable, but it
  //       x = 5;     // may modify the outer x if not managed properly
  // 	}
  // }

  void printScope(std::string_view message);

  auto allocateInEntryBlock(llvm::Function *func, std::string_view variableName)
      -> llvm::AllocaInst *;

  constexpr auto currentScope() -> Scope &;
  constexpr void beginScope();
  constexpr void endScope();

private:
  std::vector<Scope> scopeValues{Scope{}}; // Global scope
  CompilerState *state{nullptr};

  // Special member that acts as a 'return value' from latest node visit. Every
  // visit node should set this variable.
  // See {generate} utility method.
  llvm::Value *ReturnValue{nullptr};
};

constexpr auto CodeGenVisitor::currentScope() -> Scope & {

  if (scopeValues.empty()) {
    throw std::logic_error("Scope values is empty. There must have been a "
                           "skill issue as global scope disappeared");
  }

  return scopeValues.back();
}

constexpr void CodeGenVisitor::beginScope() {
  scopeValues.emplace_back(currentScope());

  DEBUG_ONLY(printScope("Symbols in after beginning new scope:"));
}
constexpr void CodeGenVisitor::endScope() {
  if (scopeValues.empty()) {
    throw std::logic_error("Trying to pop an empty scope. There must be a "
                           "mismatch between creating and deleting scopes");
  }

  DEBUG_ONLY(printScope("Symbols in before ending scope:"));
  scopeValues.pop_back();
  DEBUG_ONLY(printScope("Symbols in after beginning new scope:"));
}
