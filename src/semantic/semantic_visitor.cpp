
#include "semantic_visitor.hpp"

#include <llvm/IR/InlineAsm.h>

#include <cstddef>

#include "debug.hpp"
#include "logzy/logzy.hpp"
#include "parser/ast_nodes.hpp"
#include "semantic/types.hpp"
#include "utility/utility.hpp"

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

void SemanticAnalysisVisitor::visit(BooleanAstNode *boolean) {
  logzy::trace("Semantic check of BooleanAstNode");
  boolean->resolvedType = TivaType::Boolean;
  logzy::trace("{} resolved as bool", boolean->val);
}

void SemanticAnalysisVisitor::visit(VariableAstNode *var) {
  logzy::trace("Semantic check of VariableAstNode");

  auto variable = scopes_.findVariable(var->name);
  if (!variable) {
    logzy::error("no variable '{}' in current scope", var->name);
    return;
  }

  var->resolvedType = *variable;

  logzy::trace("variable '{}' resolved as {}", var->name, var->resolvedType);
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

  for (auto &arg : call->args) {
    dispatch(arg.get());
  }

  // TODO :: Implement FindCallable function that will resolve the node into a
  // callable name or something, for now only variables

  auto variableNode = util::uniqueDynamicCast<VariableAstNode>(call->toCall);
  if (variableNode == nullptr) {
    logzy::error("Call node's left side is not callable");
    return;
  }

  auto sigIter = functionTable_.find(variableNode->name);
  if (sigIter == functionTable_.end()) {
    logzy::error(
        "Trying to call undefined function '{}', defined functions: {}",
        variableNode->name, functionTable_.size());
    return;
  }
  auto &sig = sigIter->second;
  if (call->args.size() != sig.parameters.size()) {
    logzy::error(
        "The number of arguments passed ({}) to function '{}'  doesnt match "
        "expected number ()",
        call->args.size(), variableNode->name, sig.parameters.size());
    return;
  }
  for (size_t i = 0; i < call->args.size(); ++i) {
    auto &arg = call->args[i];
    auto &param = sig.parameters[i];
    if (arg->resolvedType != param.declaredType) {
      logzy::error(
          "Argument {} (parameter={}) passed to function '{}' doesn't match "
          "declared type. expected={} actual={}",
          i, param.name, variableNode->name, param.declaredType,
          arg->resolvedType);
      return;
    }
  }

  call->resolvedType = sig.returnType;
  logzy::trace("Call to function '{}' resolved as type '{}'",
               variableNode->name, call->resolvedType);
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

  switch (binExp->op) {
    case TokenType::Equality:
      binExp->resolvedType = TivaType::Boolean;
      break;
    default:
      break;
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

  {
    auto scope = scopes_.scopeGuard();
    for (const auto &expr : block->expressions) {
      dispatch(expr.get());
    }
  }

  auto &lastExpression = block->expressions.back();
  block->resolvedType = lastExpression->resolvedType;
  logzy::trace("Block '{}' has resolved type of '{}'", block->name,
               block->resolvedType);
}
void SemanticAnalysisVisitor::visit(IfElseAstNode *ifElse) {
  logzy::trace("Semantic check of IfElseAstNode");
  // TODO :: convert condition to bool

  auto *condition = ifElse->condition.get();
  auto *body = ifElse->body.get();
  auto *elseBody = ifElse->elseBody.get();

  dispatch(condition);

  if (condition->resolvedType != TivaType::Boolean) {
    logzy::error("If's condition is not a boolean expression");
    return;
  }

  dispatch(body);

  if (elseBody != nullptr) {
    dispatch(elseBody);
  }

  TivaType resolvedType = TivaType::Void;

  if (elseBody != nullptr) {
    if (body->resolvedType == elseBody->resolvedType) {
      resolvedType = body->resolvedType;
    } else {
      logzy::warn(
          "If/elseif/else branches have different types. if expression cannot "
          "return value. ");
    }
  }

  ifElse->resolvedType = resolvedType;

  // No else block, expression returns void

  logzy::trace("if/elseif/else block evaluated to: {}. Has else block?: {}",
               ifElse->resolvedType,
               (ifElse->elseBody == nullptr) ? "no" : "yes");

  logzy::trace("If/elseif/else checked");
}
void SemanticAnalysisVisitor::visit(LetAstNode *let) {
  logzy::trace("Semantic check of LetAstNode");
  dispatch(let->rhs.get());

  if (let->declaredType != TivaType::Unknown &&
      let->rhs->resolvedType != let->declaredType) {
    cast(let->rhs, let->declaredType);
  }

  let->resolvedType = let->rhs->resolvedType;

  scopes_.putVariable(let->varName, let->resolvedType);

  logzy::trace("let expression's of '{}' = '{}' resolved typye is {}",
               let->varName, let->rhs->resolvedType, let->resolvedType);
}

void SemanticAnalysisVisitor::visit(CastAstNode *cast) {
  logzy::trace("Semantic check of cast node");
  dispatch(cast->operand.get());
  cast->resolvedType = cast->targetType;
}

void SemanticAnalysisVisitor::visit(FunctionPrototype *proto) {
  logzy::trace("Semantic check of function '{}' prototype", proto->name);
  for (auto &param : proto->params) {
    scopes_.putVariable(param.name, param.declaredType);
  }

  auto sigIter = functionTable_.find(proto->name);
  if (sigIter != functionTable_.end()) {
    FunctionSignature &sig = sigIter->second;
    if (proto->params.size() != sig.parameters.size()) {
      logzy::error(
          "Function '{}' redeclared with different number of parameters "
          "current={} "
          "vs previous={}",
          proto->name, proto->params.size(), sig.parameters.size());
      return;
    }
    // TODO :: Chekcing types of parameters
  }

  functionTable_[proto->name] =
      FunctionSignature{.name = proto->name,
                        .parameters = proto->params,
                        .returnType = proto->returnType};
}
void SemanticAnalysisVisitor::visit(Function *func) {
  logzy::trace("Semantic check of Function");
  {
    auto scope = scopes_.scopeGuard();
    dispatch(func->prototype.get());
    dispatch(func->body.get());
  }

  logzy::trace("Function's body inferred type: '{}'", func->body->resolvedType);
  if (func->prototype->returnType != TivaType::Unknown &&
      func->body->resolvedType != func->prototype->returnType) {
    logzy::trace("Casting functin return type expected:{} and (inferred:{}",
                 func->prototype->returnType, func->body->resolvedType);
    cast(func->body, func->prototype->returnType);
  }

  // Updating inferred type in function table
  DEBUG_ASSERT(functionTable_.contains(func->prototype->name));
  FunctionSignature &sig = functionTable_[func->prototype->name];
  if (sig.returnType == TivaType::Unknown) {
    sig.returnType = func->body->resolvedType;
  }

  func->prototype->returnType = func->body->resolvedType;

  logzy::trace("Function's body final return type: '{}'",
               func->body->resolvedType);
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

void SemanticAnalysisVisitor::visit(TranslationUnitAstNode *unit) {
  for (auto &declaration : unit->declarations) {
    declaration->accept(this);
  }
}
