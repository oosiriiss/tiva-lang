#pragma once
#include "parser/ast_nodes.hpp"

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
  void visit(Function *func) override;
};
