#pragma once
#include "codegen/module.hpp"
#include "core/scope_map.hpp"
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
  void visit(BooleanAstNode *boolean) override;
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
  void visit(TranslationUnitAstNode *unit) override;

 private:
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

  auto allocateInEntryBlock(llvm::Function *func, std::string_view variableName,
                            TivaType type) -> llvm::AllocaInst *;

 private:
  ScopeContainer<llvm::AllocaInst *> scopes_;
  CompilerState *state_{nullptr};

  // Special member that acts as a 'return value' from latest node visit. Every
  // visit node should set this variable.
  // See {generate} utility method.
  llvm::Value *ReturnValue{nullptr};  // NOLINT
};
