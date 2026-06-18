#include "codegen_visitor.hpp"

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/STLForwardCompat.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/PassInstrumentation.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCInstrDesc.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include <logzy/logzy.hpp>

#include "codegen/module.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "parser/ast_nodes.hpp"
#include "semantic/types.hpp"
#include "utility/utility.hpp"

[[nodiscard]] auto CodeGenVisitor::generate(AstNode *node) -> llvm::Value * {
  ReturnValue = nullptr;
  node->accept(this);

  if (ReturnValue == nullptr) {
    logzy::error("Expression didn't return a value");
  }
  return ReturnValue;
}

[[nodiscard]] auto CodeGenVisitor::generate(std::unique_ptr<AstNode> &node)
    -> llvm::Value * {
  return generate(node.get());
}

auto CodeGenVisitor::allocateInEntryBlock(llvm::Function *func,
                                          std::string_view variableName,
                                          TivaType type) -> llvm::AllocaInst * {
  llvm::Type *llvmType = toLlvm(state_->context, type);
  llvm::IRBuilder<> entryBuilder(&func->getEntryBlock(),
                                 func->getEntryBlock().begin());

  return entryBuilder.CreateAlloca(llvmType, nullptr, variableName);
}

void CodeGenVisitor::visit(IntegerAstNode *integer) {
  logzy::trace("generating code for number '{}'", integer->val);
  ReturnValue = llvm::ConstantInt::get(
      state_->context, llvm::APInt(typing::INT_BITS, integer->val, true));
}

void CodeGenVisitor::visit(FloatAstNode *flt) {
  logzy::trace("generating code for number '{}'", flt->val);
  ReturnValue = llvm::ConstantFP::get(state_->context, llvm::APFloat(flt->val));
}

void CodeGenVisitor::visit(BooleanAstNode *boolean) {
  logzy::trace("generating code for boolean '{}'", boolean->val);

  ReturnValue = (boolean->val) ? llvm::ConstantInt::getTrue(state_->context)
                               : llvm::ConstantInt::getFalse(state_->context);
  logzy::trace("Generated code for boolean");
}

void CodeGenVisitor::visit(VariableAstNode *var) {
  logzy::trace("generating code for variable '{}' (r-value)", var->name);

  auto variable = scopes_.findVariable(var->name);
  if (!variable) {
    logzy::warn("variable not found in this scope");
    ReturnValue = nullptr;
    return;
  }

  // Generates a value (R-value)
  ReturnValue = state_->builder.CreateLoad(variable->get()->getAllocatedType(),
                                           variable->get(), var->name);
}
void CodeGenVisitor::visit(AssignmentAstNode *assignment) {
  auto *var = dynamic_cast<VariableAstNode *>(assignment->lhs.get());
  logzy::trace("Generating code for assignemnt of var: '{}'", var->name);

  auto variable = scopes_.findVariable(var->name);
  if (!variable) {
    logzy::warn("variable not found in this scope");
    ReturnValue = nullptr;
    return;
  }

  llvm::AllocaInst *lhs = variable->get();
  auto *rhsValue = generate(assignment->rhs);

  if (rhsValue == nullptr) {
    logzy::error("Couldn't generate code for assginemnt rhs");
    ReturnValue = nullptr;
    return;
  }

  state_->builder.CreateStore(rhsValue, lhs);

  ReturnValue = rhsValue;
}
void CodeGenVisitor::visit(CallAstNode *call) {
  auto variableNode = util::uniqueDynamicCast<VariableAstNode>(call->toCall);
  if (variableNode == nullptr) {
    logzy::error("Trying to call non-variable");
    ReturnValue = nullptr;
    return;
  }

  std::vector<std::unique_ptr<AstNode>> &args = call->args;

  logzy::trace("generating code for call to '{}' with args.count()={}",
               variableNode->name, args.size());

  llvm::Function *functionToCall =
      state_->module->getFunction(variableNode->name);
  if (functionToCall == nullptr) {
    logzy::error("Trying to call undefined function '{}'", variableNode->name);
    ReturnValue = nullptr;
    return;
  }

  if (functionToCall->arg_size() != args.size()) {
    logzy::error("Invalid number of arguments to the function '{}'",
                 variableNode->name);
    ReturnValue = nullptr;
    return;
  }

  std::vector<llvm::Value *> argValues;
  argValues.reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    auto *argValue = generate(args[i]);
    if (argValue == nullptr) {
      logzy::debug("Couldnt generate code for arg[{}] of function {}", i,
                   variableNode->name);
      ReturnValue = nullptr;
      return;
    }
    argValues.emplace_back(argValue);
  }

  ReturnValue =
      state_->builder.CreateCall(functionToCall, argValues, "calltmp");
}
void CodeGenVisitor::visit(BinaryExprAstNode *binExpr) {
  std::unique_ptr<AstNode> &lhs = binExpr->lhs;
  std::unique_ptr<AstNode> &rhs = binExpr->rhs;
  TokenType oper = binExpr->op;

  logzy::trace("generating code for binary expression with operator'{}'", oper);

  // Special case for assginemnts

  llvm::Value *left = generate(lhs);
  llvm::Value *right = generate(rhs);
  if (lhs == nullptr || rhs == nullptr) {
    logzy::warn("Binary expression has no left or right side");
    ReturnValue = nullptr;
    return;
  }

  // Dirty way just to check working of floats
  llvm::Type *leftTy = left->getType();
  llvm::Type *rightTy = right->getType();

  if (leftTy->isDoubleTy() && rightTy->isIntegerTy()) {
    logzy::trace("Upcasting right operand to float");
    right = state_->builder.CreateSIToFP(right, leftTy, "upcast_tmp");

  } else if (leftTy->isIntegerTy() && rightTy->isDoubleTy()) {
    logzy::trace("Upcasting left operand to float");
    left = state_->builder.CreateSIToFP(left, rightTy, "upcast_tmp");
  }

  bool isFloat = (leftTy->isDoubleTy() || rightTy->isDoubleTy());

  if (isFloat) {
    switch (oper) {
      case TokenType::Plus:
        ReturnValue = state_->builder.CreateFAdd(left, right, "addftmp");
        break;
      case TokenType::Minus:
        ReturnValue = state_->builder.CreateFSub(left, right, "subftmp");
        break;
      case TokenType::Divide:
        ReturnValue = state_->builder.CreateFDiv(left, right, "divftmp");
        break;
      case TokenType::Multiply:
        ReturnValue = state_->builder.CreateFMul(left, right, "mulftmp");
        break;
      default:
        logzy::error("invalid operator '{}'", oper);
        ReturnValue = nullptr;
        break;
    }
  } else {
    switch (oper) {
      case TokenType::Plus:
        ReturnValue = state_->builder.CreateAdd(left, right, "addtmp");
        break;
      case TokenType::Minus:
        ReturnValue = state_->builder.CreateSub(left, right, "subtmp");
        break;
      case TokenType::Divide:
        ReturnValue = state_->builder.CreateSDiv(left, right, "divtmp");
        break;
      case TokenType::Multiply:
        ReturnValue = state_->builder.CreateMul(left, right, "multmp");
        break;
      default:
        logzy::error("invalid operator '{}'", oper);
        ReturnValue = nullptr;
        break;
    }
  }
}
void CodeGenVisitor::visit(BlockAstNode *block) {
  std::vector<std::unique_ptr<AstNode>> &expressions = block->expressions;

  logzy::trace("Generating code for block with {} expressions",
               expressions.size());

  {
    auto scope = scopes_.scopeGuard();

    for (auto &expr : block->expressions) {
      ReturnValue = generate(expr);

      if (ReturnValue == nullptr) {
        logzy::error("Failed to generate code for expression");
        ReturnValue = nullptr;
        return;
      }
    }
  }

  // Block empty
  if (ReturnValue == nullptr) {
    logzy::error("Empty block encountered");
    ReturnValue = nullptr;
    return;
  }

  // ReturnValue set while evaluating expressions
}

void CodeGenVisitor::visit(IfElseAstNode *ifElse) {
  logzy::debug("Generating code for if/elseif/else expression");

  llvm::Function *parentFunc = state_->builder.GetInsertBlock()->getParent();
  llvm::BasicBlock *mergeBlock =
      llvm::BasicBlock::Create(state_->context, "ifMergeBlock");

  auto *conditionValue = generate(ifElse->condition);

  if (conditionValue == nullptr) {
    logzy::error("Couldn't generate code for condition");
    ReturnValue = nullptr;
    return;
  }
  logzy::trace("Code for if condition generated");

  // TODO :: For now integer condition automatically converts to int. make
  // sure ther condition is bool LATER.

  logzy::trace(
      "Converting condition of type '{}' to boolean by comparing it to 0",
      ifElse->condition->resolvedType);

  conditionValue = state_->builder.CreateICmpNE(
      conditionValue,
      llvm::ConstantInt::get(state_->context,
                             llvm::APInt(typing::INT_BITS, 0, true)));

  llvm::BasicBlock *bodyBlock =
      llvm::BasicBlock::Create(state_->context, "ifBranch", parentFunc);
  llvm::BasicBlock *elseBlock =
      llvm::BasicBlock::Create(state_->context, "elseBranch", parentFunc);

  state_->builder.CreateCondBr(conditionValue, bodyBlock, elseBlock);

  state_->builder.SetInsertPoint(bodyBlock);
  auto *bodyValue = generate(ifElse->body);
  if (bodyValue == nullptr) {
    logzy::error("Couldn't generate code for branch body ");
    ReturnValue = nullptr;
    return;
  }
  state_->builder.CreateBr(mergeBlock);
  llvm::BasicBlock *nestedBodyEndBlock = state_->builder.GetInsertBlock();

  llvm::Value *elseValue = nullptr;
  state_->builder.SetInsertPoint(elseBlock);

  if (ifElse->elseBody != nullptr) {
    logzy::trace("Else block, found, if can be an expression");
    elseValue = generate(ifElse->elseBody);
    if (elseValue == nullptr) {
      logzy::error("Couldn't generate value for the else body");
      ReturnValue = nullptr;
      return;
    }
    state_->builder.CreateBr(mergeBlock);
  } else {
    logzy::trace(
        "No else block, if cannot be an expression, cleraing phi values");
    state_->builder.CreateBr(mergeBlock);
  }

  llvm::BasicBlock *nestedElseEndBlock = state_->builder.GetInsertBlock();
  parentFunc->insert(parentFunc->end(), mergeBlock);
  state_->builder.SetInsertPoint(mergeBlock);

  if (ifElse->elseBody == nullptr) {
    // Most likely not all branches return value
    logzy::trace("If is not an expression");
    ReturnValue = nullptr;
    return;
  }

  logzy::trace("If is an expression.");
  llvm::PHINode *phi = state_->builder.CreatePHI(
      toLlvm(state_->context, ifElse->resolvedType), 2, "ifResult");
  phi->addIncoming(bodyValue, nestedBodyEndBlock);
  phi->addIncoming(elseValue, nestedElseEndBlock);

  ReturnValue = phi;
  logzy::trace(
      "Generated code for if/elseif/else with branches and {} else block",
      (ifElse->elseBody == nullptr) ? "no" : "an");
}

void CodeGenVisitor::visit(LetAstNode *let) {
  logzy::trace("Generting code for let '{}' = <expr>", let->varName);
  std::string_view varName = let->varName;
  std::unique_ptr<AstNode> &rhs = let->rhs;

  if (scopes_.findVariable(varName).has_value()) {
    logzy::error(
        "There already exists a variable with name {} in current scope.",
        varName);
    ReturnValue = nullptr;
    return;
  }

  logzy::trace("variable '{}'s resolved type is: {}", let->varName,
               rhs->resolvedType);
  llvm::AllocaInst *alloc =
      allocateInEntryBlock(state_->builder.GetInsertBlock()->getParent(),
                           varName, rhs->resolvedType);

  logzy::trace("variable '{}' allocated on stack", let->varName);

  scopes_.putVariable(varName, alloc);

  auto *rhsValue = generate(rhs);
  if (rhsValue == nullptr) {
    logzy::error("couldn't genreate code for let '{}' rhs expression", varName);
    ReturnValue = nullptr;
    return;
  }

  state_->builder.CreateStore(rhsValue, alloc);

  ReturnValue = rhsValue;
}

void CodeGenVisitor::visit(CastAstNode *cast) {
  logzy::trace("Generating code for CastNode");
  llvm::Value *operandValue = generate(cast->operand.get());
  if (operandValue == nullptr) {
    logzy::error("couldnt' cast as operand couldn't be evaluated");
    return;
  }

  using TivaType::Boolean;
  using TivaType::Float;
  using TivaType::Int;

  TivaType sourceType = cast->operand->resolvedType;
  TivaType targetType = cast->targetType;

  if (sourceType == Int && targetType == Float) {
    logzy::trace("Casting int to float");
    ReturnValue = state_->builder.CreateSIToFP(
        operandValue, llvm::Type::getDoubleTy(state_->context),
        "int_float_cast");
    return;
  }

  if (sourceType == Float && targetType == Int) {
    logzy::trace("Casting float to int");
    ReturnValue = state_->builder.CreateFPToSI(
        operandValue, llvm::Type::getInt32Ty(state_->context));
    return;
  }

  if (sourceType == Boolean && targetType == Int) {
    logzy::trace("Casting boolean to int");
    ReturnValue = state_->builder.CreateIntCast(
        operandValue, toLlvm(state_->context, TivaType::Int), true);
    return;
  }

  logzy::error("Invalid conversion from type '{}' to '{}'", sourceType,
               targetType);
}

void CodeGenVisitor::visit(FunctionPrototype *proto) {
  logzy::trace("Generating code for function '{}' prototype", proto->name);
  llvm::Function *func = state_->module->getFunction(proto->name);
  if (func != nullptr) {
    logzy::trace("Function already defined");
    ReturnValue = func;
    return;
  }

  logzy::trace("Mapping parameters to llvm types");
  auto &params = proto->params;
  std::vector<llvm::Type *> paramTypes;
  paramTypes.reserve(proto->params.size());
  for (const auto &param : proto->params) {
    paramTypes.emplace_back(toLlvm(state_->context, param.declaredType));
  }

  logzy::trace("Creating the function");

  llvm::Type *returnType = toLlvm(state_->context, proto->returnType);
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                proto->name, *state_->module);

  logzy::trace("Setting function parameter names");
  size_t paramIdx = 0;
  for (auto &arg : func->args()) {
    DEBUG_ASSERT(paramIdx < params.size(),
                 std::format("idx={} args={}", paramIdx, params.size()));
    arg.setName(params[paramIdx++].name);
  }

  logzy::trace("Function created");
  ReturnValue = func;
}

void CodeGenVisitor::visit(Function *func) {
  std::unique_ptr<FunctionPrototype> &prototype = func->prototype;

  // Checking if function was previously declared.
  logzy::debug("Generating code for function '{}'", prototype->name);
  auto *llvmFunc = llvm::dyn_cast<llvm::Function>(generate(prototype.get()));

  if (llvmFunc == nullptr) {
    logzy::error("Couldn't create function '{}' prototype", prototype->name);
    ReturnValue = nullptr;
    return;
  }

  if (!llvmFunc->empty()) {
    logzy::error("Function '{}' redefined", prototype->name);
    ReturnValue = nullptr;
    return;
  }

  llvm::BasicBlock *block =
      llvm::BasicBlock::Create(state_->context, "entry", llvmFunc);
  state_->builder.SetInsertPoint(block);

  llvm::Value *retVal = nullptr;
  {
    auto scope = scopes_.scopeGuard();

    size_t currentArgIndex = 0;
    for (auto &arg : llvmFunc->args()) {
      auto *allocated = allocateInEntryBlock(
          llvmFunc, arg.getName(),
          prototype->params[currentArgIndex++].declaredType);
      state_->builder.CreateStore(&arg, allocated);
      scopes_.putVariable(arg.getName(), allocated);
    }

    logzy::debug("Generating code for function '{}' body", prototype->name);
    retVal = generate(func->body);
  }

  if (retVal == nullptr) {
    logzy::error("Function didn't return value");
    llvmFunc->eraseFromParent();
    ReturnValue = nullptr;
    return;
  }

  state_->builder.CreateRet(retVal);

  llvm::verifyFunction(*llvmFunc);
  ReturnValue = llvmFunc;
}

void CodeGenVisitor::visit(TranslationUnitAstNode *unit) {
  for (auto &declaration : unit->declarations) {
    declaration->accept(this);
  }
}
