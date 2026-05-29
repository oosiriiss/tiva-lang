
#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "lexer.hpp"
#include "semantic/types.hpp"
#include "utility.hpp"

struct CompilerState;
class AstNodeVisitor;

class AstNode {
 public:
  AstNode() = default;
  constexpr explicit AstNode(TivaType type) noexcept
      : resolvedType{type} {}
  virtual ~AstNode() = default;
  virtual void accept(AstNodeVisitor *) = 0;

 public:
  TivaType resolvedType = TivaType::Unknown;
};

struct IntegerAstNode : public AstNode {
  std::uint64_t val;

  constexpr IntegerAstNode(std::string_view numStr)
      : val{util::toNumber<std::uint64_t>(numStr)} {}

  void accept(AstNodeVisitor *) override;
};

struct FloatAstNode : public AstNode {
  double val;

  constexpr FloatAstNode(std::string_view numStr)
      : val{util::toNumber<double>(numStr)} {}

  void accept(AstNodeVisitor *) override;
};

struct VariableAstNode : public AstNode {
  std::string name;

  constexpr VariableAstNode(std::string_view name)
      : name{name} {}
  void accept(AstNodeVisitor *) override;
};

struct AssignmentAstNode : public AstNode {
  std::unique_ptr<VariableAstNode> var;
  std::unique_ptr<AstNode> rhs;

  constexpr AssignmentAstNode(std::unique_ptr<VariableAstNode> lhs,
                              std::unique_ptr<AstNode> rhs)
      : var{std::move(lhs)},
        rhs{std::move(rhs)} {}
  void accept(AstNodeVisitor *) override;
};

struct CallAstNode : public AstNode {
  std::string toCall;
  std::vector<std::unique_ptr<AstNode>> args;

  constexpr CallAstNode(std::string_view toCall,
                        std::vector<std::unique_ptr<AstNode>> &&args)
      : toCall{toCall},
        args{std::move(args)} {}
  void accept(AstNodeVisitor *) override;
};

struct BinaryExprAstNode : public AstNode {
  TokenType op;
  std::unique_ptr<AstNode> lhs;
  std::unique_ptr<AstNode> rhs;

  constexpr BinaryExprAstNode(TokenType op, std::unique_ptr<AstNode> lhs,
                              std::unique_ptr<AstNode> rhs)
      : op{op},
        lhs{std::move(lhs)},
        rhs{std::move(rhs)} {}
  void accept(AstNodeVisitor *) override;
};

struct BlockAstNode : public AstNode {
  std::string name;
  std::vector<std::unique_ptr<AstNode>> expressions;

  constexpr BlockAstNode(std::string_view name,
                         std::vector<std::unique_ptr<AstNode>> &&expressions)
      : name{name},
        expressions{std::move(expressions)} {}
  void accept(AstNodeVisitor *) override;
};

struct IfElseAstNode : public AstNode {
  std::unique_ptr<AstNode> condition;
  std::unique_ptr<AstNode> ifBody;
  std::unique_ptr<AstNode> elseBody;

  constexpr IfElseAstNode(std::unique_ptr<AstNode> condition,
                          std::unique_ptr<AstNode> ifBlock,
                          std::unique_ptr<AstNode> elseBlock)
      : condition{std::move(condition)},
        ifBody{std::move(ifBlock)},
        elseBody{std::move(elseBlock)} {}
  void accept(AstNodeVisitor *) override;
};

struct LetAstNode : public AstNode {
  std::string varName;
  std::unique_ptr<AstNode> rhs;

  constexpr LetAstNode(std::string_view variableName,
                       std::unique_ptr<AstNode> rhs)
      : varName{variableName},
        rhs{std::move(rhs)} {}

  void accept(AstNodeVisitor *) override;
};

struct CastNode : public AstNode {
  std::unique_ptr<AstNode> operand;
  TivaType targetType;

  constexpr CastNode(std::unique_ptr<AstNode> operand,
                     TivaType targetType) noexcept
      : AstNode{targetType},
        operand{std::move(operand)},
        targetType{targetType} {}
  void accept(AstNodeVisitor *) override;
};

struct FunctionPrototype {
  std::string name;
  std::vector<std::string> args;

  constexpr FunctionPrototype(std::string_view name,
                              std::vector<std::string> &&argNames)
      : name{name},
        args{std::move(argNames)} {}
};

struct Function : public AstNode {
  std::unique_ptr<FunctionPrototype> prototype;
  std::unique_ptr<AstNode> body;

  constexpr Function(std::unique_ptr<FunctionPrototype> prototype,
                     std::unique_ptr<AstNode> expression)
      : prototype{std::move(prototype)},
        body{std::move(expression)} {}
  void accept(AstNodeVisitor *) override;
};

struct AstNodeVisitor {
  void dispatch(AstNode *node) { node->accept(this); }
  virtual void visit(IntegerAstNode *) = 0;
  virtual void visit(FloatAstNode *) = 0;
  virtual void visit(VariableAstNode *) = 0;
  virtual void visit(AssignmentAstNode *) = 0;
  virtual void visit(CallAstNode *) = 0;
  virtual void visit(BinaryExprAstNode *) = 0;
  virtual void visit(BlockAstNode *) = 0;
  virtual void visit(IfElseAstNode *) = 0;
  virtual void visit(LetAstNode *) = 0;
  virtual void visit(CastNode *) = 0;
  virtual void visit(Function *) = 0;
};
