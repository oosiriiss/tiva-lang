#pragma once
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
  void visit(CastNode *let) override;
  void visit(Function *func) override;

 private:
  static void ensureValidTypes(std::unique_ptr<AstNode> &left,
                               std::unique_ptr<AstNode> &right);
  static constexpr void cast(std::unique_ptr<AstNode> &input,
                             TivaType targetType);
};

constexpr void SemanticAnalysisVisitor::cast(std::unique_ptr<AstNode> &input,
                                             TivaType targetType) {
  input = std::make_unique<CastNode>(std::move(input), targetType);
}

