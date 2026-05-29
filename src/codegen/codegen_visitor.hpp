#pragma once
#include "codegen/module.hpp"
#include "debug.hpp"
#include "parser/ast_nodes.hpp"

namespace llvm {
  struct AllocaInst;
  class Function;
}  // namespace llvm

class CodeGenVisitor : public AstNodeVisitor {
 public:
  constexpr CodeGenVisitor(CompilerState *state) noexcept
      : state_{state} {}

  void visit(IntegerAstNode *integer) override;
  void visit(FloatAstNode *flt) override;
  void visit(VariableAstNode *var) override;
  void visit(AssignmentAstNode *assignment) override;
  void visit(CallAstNode *call) override;
  void visit(BinaryExprAstNode *binExpr) override;
  void visit(BlockAstNode *block) override;
  void visit(IfElseAstNode *ifElse) override;
  void visit(LetAstNode *let) override;
  void visit(CastNode *cast) override;
  void visit(FunctionPrototype *proto) override;
  void visit(Function *func) override;

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

  auto allocateInEntryBlock(llvm::Function *func, std::string_view variableName,
                            TivaType type) -> llvm::AllocaInst *;

  constexpr auto currentScope() -> Scope &;
  constexpr void beginScope();
  constexpr void endScope();

 private:
  std::vector<Scope> scopeValues_{Scope{}};  // Global scope
  CompilerState *state_{nullptr};

  // Special member that acts as a 'return value' from latest node visit. Every
  // visit node should set this variable.
  // See {generate} utility method.
  llvm::Value *ReturnValue{nullptr};  // NOLINT
};

constexpr auto CodeGenVisitor::currentScope() -> Scope & {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Scope values is empty. There must have been a "
        "skill issue as global scope disappeared");
  }

  return scopeValues_.back();
}

constexpr void CodeGenVisitor::beginScope() {
  scopeValues_.emplace_back(currentScope());

  DEBUG_ONLY(printScope("Symbols in after beginning new scope:"));
}
constexpr void CodeGenVisitor::endScope() {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Trying to pop an empty scope. There must be a "
        "mismatch between creating and deleting scopes");
  }

  DEBUG_ONLY(printScope("Symbols in before ending scope:"));
  scopeValues_.pop_back();
  DEBUG_ONLY(printScope("Symbols in after beginning new scope:"));
}
