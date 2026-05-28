#include "semantic_visitor.hpp"
#include <utility>

#include "logzy/logzy.hpp"
#include "semantic/types.hpp"

void SemanticAnalysisVisitor::visit(IntegerAstNode *integer) {
  integer->resolvedType = TivaType::Int;
}
void SemanticAnalysisVisitor::visit(FloatAstNode *flt) {
  flt->resolvedType = TivaType::Float;
}
void SemanticAnalysisVisitor::visit(VariableAstNode *var) {
  var->resolvedType = TivaType::Int;  // TODO :: No declaration of variables of
                                      // other type than int yet
}
void SemanticAnalysisVisitor::visit(AssignmentAstNode *assignment) {
  dispatch(assignment->var.get());
  dispatch(assignment->rhs.get());
  assignment->resolvedType = assignment->rhs->resolvedType;
}
void SemanticAnalysisVisitor::visit(CallAstNode *call) {
  call->resolvedType = TivaType::Int;  // TODO :: No declaration of functions
                                       // returning other type than int yet
}
void SemanticAnalysisVisitor::visit(BinaryExprAstNode *binExp) {
  dispatch(binExp->lhs.get());
  dispatch(binExp->rhs.get());

  auto leftType = binExp->lhs->resolvedType;
  auto rightType = binExp->rhs->resolvedType;
  // Promotion to float
  if (leftType == TivaType::Float || rightType == TivaType::Float) {
    binExp->resolvedType = TivaType::Float;
  } else {
    binExp->resolvedType = TivaType::Int;
  }
}
void SemanticAnalysisVisitor::visit(BlockAstNode *block) {
  if (block->expressions.empty()) {
    logzy::error("Empty block '{}'", block->blockName);
    return;
  }

  auto &lastExpression = block->expressions.back();
  dispatch(lastExpression.get());

  block->resolvedType = lastExpression->resolvedType;
}
void SemanticAnalysisVisitor::visit(IfElseAstNode *ifElse) {
  dispatch(ifElse->condition.get());
  dispatch(ifElse->ifBody.get());
  dispatch(ifElse->elseBody.get());

  // TODO :: convert condition to bool

  auto conditonType = ifElse->condition->resolvedType;
  auto ifBodyType = ifElse->ifBody->resolvedType;
  auto elseBodyType = ifElse->elseBody->resolvedType;

  if (ifBodyType != elseBodyType) {
    logzy::error("if else types do not match. (int({}) vs int({}))",
                 std::to_underlying(ifBodyType),
                 std::to_underlying(elseBodyType));
    return;
  }
  ifElse->resolvedType = ifBodyType;
}
void SemanticAnalysisVisitor::visit(LetAstNode *let) {
  dispatch(let->rhs.get());

  let->resolvedType = let->rhs->resolvedType;
}
void SemanticAnalysisVisitor::visit(Function *func) {
  dispatch(func->body.get());
  logzy::error("Function type is not defined");
}
