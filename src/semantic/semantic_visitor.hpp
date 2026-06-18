#pragma once
#include <llvm/IR/Instructions.h>

#include <memory>
#include <unordered_map>

#include "core/scope_map.hpp"
#include "parser/ast_nodes.hpp"
#include "semantic/types.hpp"

class SemanticAnalysisVisitor : public AstNodeVisitor {
 public:
  void visit(IntegerAstNode *integer) override;
  void visit(FloatAstNode *flt) override;
  void visit(BooleanAstNode *boolean) override;
  void visit(VariableAstNode *var) override;
  void visit(AssignmentAstNode *assignment) override;
  void visit(CallAstNode *call) override;
  void visit(BinaryExprAstNode *binExp) override;
  void visit(BlockAstNode *block) override;
  void visit(IfElseAstNode *ifElse) override;
  void visit(LetAstNode *let) override;
  void visit(CastAstNode *cast) override;
  void visit(FunctionPrototype *proto) override;
  void visit(Function *func) override;
  void visit(TranslationUnitAstNode *unit) override;

 private:
  using Scope = std::unordered_map<std::string, TivaType>;

  static void ensureValidTypes(std::unique_ptr<AstNode> &left,
                               std::unique_ptr<AstNode> &right);
  static constexpr void cast(std::unique_ptr<AstNode> &input,
                             TivaType targetType);

  ScopeContainer<TivaType> scopes_;

  std::unordered_map<std::string, FunctionSignature> functionTable_;
};

constexpr void SemanticAnalysisVisitor::cast(std::unique_ptr<AstNode> &input,
                                             TivaType targetType) {
  input = std::make_unique<CastAstNode>(std::move(input), targetType);
}
