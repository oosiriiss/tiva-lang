#pragma once
#include <llvm/IR/Instructions.h>

#include <memory>
#include <unordered_map>

#include "parser/ast_nodes.hpp"
#include "semantic/types.hpp"

class SemanticAnalysisVisitor : public AstNodeVisitor {
 public:
  void visit(IntegerAstNode *integer) override;
  void visit(FloatAstNode *flt) override;
  void visit(VariableAstNode *var) override;
  void visit(AssignmentAstNode *assignment) override;
  void visit(CallAstNode *call) override;
  void visit(BinaryExprAstNode *binExp) override;
  void visit(BlockAstNode *block) override;
  void visit(IfElseAstNode *ifElse) override;
  void visit(LetAstNode *let) override;
  void visit(CastNode *cast) override;
  void visit(FunctionPrototype *proto) override;
  void visit(Function *func) override;
  void visit(TranslationUnitAstNode *unit) override;

 private:
  using Scope = std::unordered_map<std::string, TivaType>;

  static void ensureValidTypes(std::unique_ptr<AstNode> &left,
                               std::unique_ptr<AstNode> &right);
  static constexpr void cast(std::unique_ptr<AstNode> &input,
                             TivaType targetType);

  constexpr auto currentScope() -> Scope &;
  constexpr void beginScope();
  constexpr void endScope();

  std::vector<Scope> scopeValues_{Scope{}};  // Global scope

  std::unordered_map<std::string, FunctionSignature> functionTable_;
};

constexpr void SemanticAnalysisVisitor::cast(std::unique_ptr<AstNode> &input,
                                             TivaType targetType) {
  input = std::make_unique<CastNode>(std::move(input), targetType);
}

constexpr auto SemanticAnalysisVisitor::currentScope() -> Scope & {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Scope values is empty. There must have been a "
        "skill issue as global scope disappeared");
  }

  return scopeValues_.back();
}

constexpr void SemanticAnalysisVisitor::beginScope() {
  scopeValues_.emplace_back(currentScope());
}
constexpr void SemanticAnalysisVisitor::endScope() {
  if (scopeValues_.empty()) {
    throw std::logic_error(
        "Trying to pop an empty scope. There must be a "
        "mismatch between creating and deleting scopes");
  }
  scopeValues_.pop_back();
}
