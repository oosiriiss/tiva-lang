#include "semantic_visitor.hpp"

#include <iterator>
#include <utility>

#include "debug.hpp"
#include "logzy/logzy.hpp"
#include "parser/ast_nodes.hpp"
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

  for (const auto &expr : block->expressions) {
    dispatch(expr.get());
  }

  auto &lastExpression = block->expressions.back();
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

void SemanticAnalysisVisitor::visit(CastNode *cast) {
  logzy::warn("Renalyzing ImplicitCastNode");
}
void SemanticAnalysisVisitor::visit(Function *func) {
  dispatch(func->body.get());
}

void SemanticAnalysisVisitor::ensureValidTypes(
    std::unique_ptr<AstNode> &left, std::unique_ptr<AstNode> &right) {
  DEBUG_ASSERT(left->resolvedType != TivaType::Unknown,
               "Left's type must be resolved");
  DEBUG_ASSERT(right->resolvedType != TivaType::Unknown,
               "Rights's type must be resolved");

  TivaType lft = left->resolvedType;
  TivaType rgh = right->resolvedType;

  if (lft == rgh) {
    return;
  }

  using TivaType::Float;
  using TivaType::Int;

  // Int promotion to float
  if (lft == Int && rgh == Float) {
    cast(left, Float);
  } else if (lft == Float && rgh == Int) {
    cast(right, Float);
  }
}
