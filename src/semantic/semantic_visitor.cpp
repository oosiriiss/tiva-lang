#include "semantic_visitor.hpp"

#include "debug.hpp"
#include "logzy/logzy.hpp"
#include "parser/ast_nodes.hpp"
#include "semantic/types.hpp"

void SemanticAnalysisVisitor::visit(IntegerAstNode *integer) {
  logzy::trace("Semantic check of IntegerAstNode");
  integer->resolvedType = TivaType::Int;
  logzy::trace("{} resolved as integer", integer->val);
}
void SemanticAnalysisVisitor::visit(FloatAstNode *flt) {
  logzy::trace("Semantic check of FloatAstNode");
  flt->resolvedType = TivaType::Float;
  logzy::trace("{} resolved as float", flt->val);
}
void SemanticAnalysisVisitor::visit(VariableAstNode *var) {
  logzy::trace("Semantic check of VariableAstNode");
  var->resolvedType = TivaType::Int;  // TODO :: No declaration of variables of
                                      // other type than int yet
  logzy::trace("All variable '{}' resolved as {}", var->name,
               var->resolvedType);
  logzy::warn("TODO ::All variables resolved as integer");
}
void SemanticAnalysisVisitor::visit(AssignmentAstNode *assignment) {
  logzy::trace("Semantic check of AssignmentAstNode");
  dispatch(assignment->lhs.get());
  dispatch(assignment->rhs.get());
  assignment->resolvedType = assignment->rhs->resolvedType;

  const auto *variable = dynamic_cast<VariableAstNode *>(assignment->lhs.get());
  if (variable == nullptr) {
    logzy::error("Left side of assignment has to a variable (lvalue)");
    return;
  }

  logzy::trace(
      "Assignment of variabe '{}' of type '{}' to expression of type '{}'",
      variable->name, variable->resolvedType, assignment->rhs->resolvedType);
}
void SemanticAnalysisVisitor::visit(CallAstNode *call) {
  logzy::trace("Semantic check of CallAstNode");
  call->resolvedType = TivaType::Int;  // TODO :: No declaration of functions
                                       // returning other type than int yet
  logzy::trace("Call to function '{}' resolved as type '{}'", call->toCall,
               call->resolvedType);
  logzy::warn("TODO :: All call return types resolved as integer");
}
void SemanticAnalysisVisitor::visit(BinaryExprAstNode *binExp) {
  logzy::trace("Semantic check of BinaryExprAstNode");
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

  logzy::trace("Expression lhs:{} op:{} rhs:{} has return type of {}", leftType,
               binExp->op, rightType, binExp->resolvedType);
}
void SemanticAnalysisVisitor::visit(BlockAstNode *block) {
  logzy::trace("Semantic check of BlockAstNode");
  if (block->expressions.empty()) {
    logzy::error("Empty block '{}'", block->name);
    return;
  }

  for (const auto &expr : block->expressions) {
    dispatch(expr.get());
  }

  auto &lastExpression = block->expressions.back();
  block->resolvedType = lastExpression->resolvedType;
  logzy::trace("Block '{}' has resolved type of '{}'", block->name,
               block->resolvedType);
}
void SemanticAnalysisVisitor::visit(IfElseAstNode *ifElse) {
  logzy::trace("Semantic check of IfElseAstNode");
  dispatch(ifElse->condition.get());
  dispatch(ifElse->ifBody.get());
  dispatch(ifElse->elseBody.get());

  // TODO :: convert condition to bool

  auto conditonType = ifElse->condition->resolvedType;
  auto ifBodyType = ifElse->ifBody->resolvedType;
  auto elseBodyType = ifElse->elseBody->resolvedType;

  if (ifBodyType != elseBodyType) {
    logzy::error("if else types do not match. (int({}) vs int({}))", ifBodyType,
                 elseBodyType);
    return;
  }
  ifElse->resolvedType = ifBodyType;

  logzy::trace("if else's types evaluated to: if(cond:{}) {} else {}",
               conditonType, ifBodyType, elseBodyType);
}
void SemanticAnalysisVisitor::visit(LetAstNode *let) {
  logzy::trace("Semantic check of LetAstNode");
  dispatch(let->rhs.get());

  let->resolvedType = let->rhs->resolvedType;

  logzy::trace("let expression's of '{}' = '{}' resolved typye is {}",
               let->varName, let->rhs->resolvedType, let->resolvedType);
}

void SemanticAnalysisVisitor::visit(CastNode * /*cast*/) {
  logzy::warn("Renalyzing ImplicitCastNode");
}
void SemanticAnalysisVisitor::visit(Function *func) {
  logzy::trace("Semantic check of Function");
  dispatch(func->body.get());
}

void SemanticAnalysisVisitor::ensureValidTypes(
    std::unique_ptr<AstNode> &left, std::unique_ptr<AstNode> &right) {
  logzy::trace("Semantic check of std");
  DEBUG_ASSERT(left->resolvedType != TivaType::Unknown,
               "Left's type must be resolved");
  DEBUG_ASSERT(right->resolvedType != TivaType::Unknown,
               "Rights's type must be resolved");

  logzy::trace("Ensuring operation with {} and {} is valid", left->resolvedType,
               right->resolvedType);

  TivaType lft = left->resolvedType;
  TivaType rgh = right->resolvedType;

  if (lft == rgh) {
    logzy::trace("same types, valid");
    return;
  }

  using TivaType::Float;
  using TivaType::Int;

  // Int promotion to float
  if (lft == Int && rgh == Float) {
    logzy::trace("Upcasting left:{} to right:{}", lft, rgh);
    cast(left, Float);
  } else if (lft == Float && rgh == Int) {
    logzy::trace("Upcasting right:{} to left:{}", rgh, lft);
    cast(right, Float);
  }
}
