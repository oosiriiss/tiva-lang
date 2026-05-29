#include "codegen_visitor.hpp"

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
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

void CodeGenVisitor::printScope(std::string_view message) {
  llvm::errs() << message << '\n';
  for (auto &[key, val] : scopeValues_.back()) {
    llvm::errs() << "(" << key << "," << val << ")\n";
  }
}

auto CodeGenVisitor::allocateInEntryBlock(llvm::Function *func,
                                          std::string_view variableName,
                                          TivaType type) -> llvm::AllocaInst * {
  llvm::Type *llvmType = nullptr;

  switch (type) {
    case TivaType::Int:
      llvmType = llvm::Type::getInt32Ty(state_->context);
      break;
    case TivaType::Float:
      llvmType = llvm::Type::getDoubleTy(state_->context);
      break;
    case TivaType::Unknown:
      logzy::error("Unknowntype encountered. couldn't allocate variable '{}'",
                   variableName);
      break;
  }

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

void CodeGenVisitor::visit(VariableAstNode *var) {
  logzy::trace("generating code for variable '{}' (r-value)", var->name);
  auto val = currentScope().find(var->name);
  if (val == currentScope().end()) {
    logzy::warn("variable not found in this scope");
    ReturnValue = nullptr;
    return;
  }

  // Generates a value (R-value)
  ReturnValue = state_->builder.CreateLoad(val->second->getAllocatedType(),
                                           val->second, val->first.c_str());
}
void CodeGenVisitor::visit(AssignmentAstNode *assignment) {
  auto *var = dynamic_cast<VariableAstNode *>(assignment->lhs.get());
  logzy::trace("Generating code for assignemnt of var: '{}'", var->name);

  auto varIter = currentScope().find(var->name);
  if (varIter == currentScope().end()) {
    logzy::warn("variable not found in this scope");
    ReturnValue = nullptr;
    return;
  }

  llvm::AllocaInst *lhs = varIter->second;
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
  std::string &toCall = call->toCall;
  std::vector<std::unique_ptr<AstNode>> &args = call->args;

  logzy::trace("generating code for call to '{}' with args.count()={}", toCall,
               args.size());

  llvm::Function *functionToCall = state_->module->getFunction(toCall);
  if (functionToCall == nullptr) {
    logzy::error("Trying to call undefined function '{}'", toCall);
    ReturnValue = nullptr;
    return;
  }

  if (functionToCall->arg_size() != args.size()) {
    logzy::error("Invalid number of arguments to the function '{}'", toCall);
    ReturnValue = nullptr;
    return;
  }

  std::vector<llvm::Value *> argValues;
  argValues.reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    auto *argValue = generate(args[i]);
    if (argValue == nullptr) {
      logzy::debug("Couldnt generate code for arg[{}] of function {}", i,
                   toCall);
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

  beginScope();

  for (auto &expr : block->expressions) {
    ReturnValue = generate(expr);

    if (ReturnValue == nullptr) {
      logzy::error("Failed to generate code for expression");
      ReturnValue = nullptr;
      return;
    }
  }

  // Block empty
  if (ReturnValue == nullptr) {
    logzy::error("Empty block encountered");
    ReturnValue = nullptr;
    return;
  }

  endScope();
  // ReturnValue set by the last expression.
}
void CodeGenVisitor::visit(IfElseAstNode *ifElse) {
  std::unique_ptr<AstNode> &condition = ifElse->condition;
  std::unique_ptr<AstNode> &ifBody = ifElse->ifBody;
  std::unique_ptr<AstNode> &elseBody = ifElse->elseBody;

  logzy::debug("Generating code for ifelse expression");
  auto *conditonValue = generate(condition);
  if (!condition) {
    logzy::error("Coulndt genreate code for the conditon");
    ReturnValue = nullptr;
    return;
  }

  conditonValue = state_->builder.CreateICmpNE(
      conditonValue,
      llvm::ConstantInt::get(state_->context,
                             llvm::APInt(typing::INT_BITS, 0, true)));

  llvm::Function *parentFunc = state_->builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *ifBlock =
      llvm::BasicBlock::Create(state_->context, "if", parentFunc);

  llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(
      state_->context, "else");  // Not yet inserted to the function

  llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(
      state_->context, "ifMerge");  // Not yet inserted to the function

  // TODO :: instead fo generatingthe phi node and branches, a simple seleect
  // instruciton looks simpler. But it looks like the llvm automatically does
  // this.
  //
  //  %1 = icmp sgt i32 %a, %b
  //  %2 = select i1 %1, i32 %a, i32 %b
  //  ret i32 %2

  state_->builder.CreateCondBr(conditonValue, ifBlock, elseBlock);

  state_->builder.SetInsertPoint(ifBlock);
  //
  auto *ifBodyValue = generate(ifBody);
  if (ifBodyValue == nullptr) {
    logzy::error("Couldn't generate code for if body");
    ReturnValue = nullptr;
    return;
  }
  state_->builder.CreateBr(mergeBlock);  // Branches' "return" block

  // codegen for body may change the block, restoringi t
  state_->builder.SetInsertPoint(ifBlock);

  parentFunc->insert(parentFunc->end(), elseBlock);
  state_->builder.SetInsertPoint(elseBlock);
  //
  auto *elseBodyValue = generate(elseBody);
  if (elseBodyValue == nullptr) {
    logzy::error("Couldn't generate code for if body");
    ReturnValue = nullptr;
    return;
  }
  state_->builder.CreateBr(mergeBlock);  // Branches' "return" block

  // codegen for body may change the block, restoringi t
  state_->builder.SetInsertPoint(elseBlock);

  // Merge block
  parentFunc->insert(parentFunc->end(), mergeBlock);
  state_->builder.SetInsertPoint(mergeBlock);

  llvm::PHINode *phi = state_->builder.CreatePHI(
      llvm::Type::getInt32Ty(state_->context), 2, "ifResult");

  phi->addIncoming(ifBodyValue, ifBlock);
  phi->addIncoming(elseBodyValue, elseBlock);

  ReturnValue = phi;
}

namespace {
  auto createFunction(FunctionPrototype *proto, CompilerState &state)
      -> llvm::Function * {
    std::vector<std::string> &args = proto->args;
    // for now all args are ints
    std::vector<llvm::Type *> argTypes(args.size(),
                                       llvm::Type::getInt32Ty(state.context));
    llvm::FunctionType *funcType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(state.context), argTypes, false);
    llvm::Function *func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, proto->name, *state.module);

    size_t idx = 0;
    for (auto &arg : func->args()) {
      DEBUG_ASSERT(idx < args.size(),
                   std::format("idx={} args={}", idx, args.size()));
      arg.setName(args[idx++]);
    }

    return func;
  }
}  // namespace

void CodeGenVisitor::visit(LetAstNode *let) {
  logzy::trace("Generting code for let '{}' = <expr>", let->varName);
  std::string_view varName = let->varName;
  std::unique_ptr<AstNode> &rhs = let->rhs;

  // TODO :: Unnecessary cast to string, make scopeValues comparator transparent
  if (currentScope().contains(std::string(varName))) {
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

  currentScope()[std::string(varName)] = alloc;

  auto *rhsValue = generate(rhs);
  if (rhsValue == nullptr) {
    logzy::error("couldn't genreate code for let '{}' rhs expression", varName);
    ReturnValue = nullptr;
    return;
  }

  state_->builder.CreateStore(rhsValue, alloc);

  ReturnValue = rhsValue;
}

void CodeGenVisitor::visit(CastNode *cast) {
  llvm::Value *operandValue = generate(cast);
  if (operandValue == nullptr) {
    logzy::error("couldnt' cast as operand couldn't be evaluated");
    return;
  }

  using TivaType::Float;
  using TivaType::Int;

  if (cast->resolvedType == Int && cast->targetType == Float) {
    ReturnValue = state_->builder.CreateSIToFP(
        operandValue, llvm::Type::getDoubleTy(state_->context),
        "int_float_cast");
    return;
  }

  logzy::error("Invalid conversion from int({}) to int({})", cast->resolvedType,
               cast->targetType);
}
void CodeGenVisitor::visit(Function *func) {
  std::unique_ptr<FunctionPrototype> &prototype = func->prototype;

  // Checking if function was previously declared.
  logzy::debug("Generating code for function '{}'", prototype->name);
  llvm::Function *llvmFunc = state_->module->getFunction(prototype->name);
  if (llvmFunc == nullptr) {
    logzy::debug("Generating code for function '{}' prototype",
                 prototype->name);
    llvmFunc = createFunction(prototype.get(), *state_);
  }

  if (llvmFunc == nullptr) {
    logzy::error("Couldn't create function '{}'", prototype->name);
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

  beginScope();
  size_t currentArgIndex = 0;
  for (auto &arg : llvmFunc->args()) {
    auto *allocated =
        allocateInEntryBlock(llvmFunc, arg.getName(), TivaType::Int);
    state_->builder.CreateStore(&arg, allocated);
    currentScope()[std::string(arg.getName())] = allocated;
  }

  logzy::debug("Generating code for function '{}' body", prototype->name);
  auto *returnValue = generate(func->body);
  endScope();

  if (returnValue == nullptr) {
    logzy::error("Function didn't return value");
    llvmFunc->eraseFromParent();
    ReturnValue = nullptr;
    return;
  }

  state_->builder.CreateRet(returnValue);

  llvm::verifyFunction(*llvmFunc);
  ReturnValue = llvmFunc;
}
