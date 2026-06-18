
#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "lexer/token.hpp"
#include "semantic/types.hpp"
#include "utility/utility.hpp"

struct CompilerState;
class AstNodeVisitor;

struct AstNode {
 public:
  AstNode() = default;
  AstNode(const AstNode &) = default;
  AstNode(AstNode &&) = delete;
  auto operator=(const AstNode &) -> AstNode & = default;
  auto operator=(AstNode &&) -> AstNode & = delete;
  virtual ~AstNode() = default;

  constexpr explicit AstNode(TivaType type) noexcept
      : resolvedType{type} {}

  virtual void accept(AstNodeVisitor *visitor) = 0;

 public:
  TivaType resolvedType = TivaType::Unknown;
};

struct IntegerAstNode : public AstNode {
 public:
  constexpr explicit IntegerAstNode(std::string_view numStr)
      : val{util::toNumber<std::uint64_t>(numStr)} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  std::uint64_t val;
};

struct FloatAstNode : public AstNode {
 public:
  constexpr explicit FloatAstNode(std::string_view numStr)
      : val{util::toNumber<double>(numStr)} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  double val;
};

struct BooleanAstNode : public AstNode {
 public:
  constexpr BooleanAstNode(bool value)
      : val{value} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  bool val;
};

struct VariableAstNode : public AstNode {
 public:
  constexpr explicit VariableAstNode(std::string_view name)
      : name{name} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::string name;
};

struct AssignmentAstNode : public AstNode {
 public:
  constexpr AssignmentAstNode(std::unique_ptr<AstNode> lhs,
                              std::unique_ptr<AstNode> rhs)
      : lhs{std::move(lhs)},
        rhs{std::move(rhs)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::unique_ptr<AstNode> lhs;
  std::unique_ptr<AstNode> rhs;
};

struct CallAstNode : public AstNode {
 public:
  constexpr CallAstNode(std::unique_ptr<AstNode> toCall,
                        std::vector<std::unique_ptr<AstNode>> &&args)
      : toCall{std::move(toCall)},
        args{std::move(args)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::unique_ptr<AstNode> toCall;
  std::vector<std::unique_ptr<AstNode>> args;
};

struct BinaryExprAstNode : public AstNode {
 public:
  constexpr BinaryExprAstNode(TokenType binOp, std::unique_ptr<AstNode> lhs,
                              std::unique_ptr<AstNode> rhs)
      : op{binOp},
        lhs{std::move(lhs)},
        rhs{std::move(rhs)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  TokenType op;
  std::unique_ptr<AstNode> lhs;
  std::unique_ptr<AstNode> rhs;
};

struct BlockAstNode : public AstNode {
 public:
  constexpr BlockAstNode(std::string_view name,
                         std::vector<std::unique_ptr<AstNode>> &&expressions)
      : name{name},
        expressions{std::move(expressions)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::string name;
  std::vector<std::unique_ptr<AstNode>> expressions;
};

struct IfElseAstNode : public AstNode {
 public:
  constexpr IfElseAstNode(std::unique_ptr<AstNode> condition,
                          std::unique_ptr<AstNode> body,
                          std::unique_ptr<AstNode> elseBlock)
      : condition{std::move(condition)},
        body{std::move(body)},
        elseBody{std::move(elseBlock)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::unique_ptr<AstNode> condition;
  std::unique_ptr<AstNode> body;
  std::unique_ptr<AstNode> elseBody;
};

struct LetAstNode : public AstNode {
 public:
  constexpr LetAstNode(std::string_view variableName,
                       std::unique_ptr<AstNode> rhs, TivaType declaredType)
      : varName{variableName},
        rhs{std::move(rhs)},
        declaredType{declaredType} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  std::string varName;
  std::unique_ptr<AstNode> rhs;
  TivaType declaredType;
};

struct CastAstNode : public AstNode {
 public:
  constexpr CastAstNode(std::unique_ptr<AstNode> operand,
                        TivaType targetType) noexcept
      : AstNode{targetType},
        operand{std::move(operand)},
        targetType{targetType} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::unique_ptr<AstNode> operand;
  TivaType targetType;
};

struct FunctionPrototype : public AstNode {
 public:
  constexpr FunctionPrototype(std::string_view name,
                              std::vector<Parameter> &&params,
                              TivaType returnType)
      : name{name},
        params{std::move(params)},
        returnType{returnType} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  std::string name;
  std::vector<Parameter> params;
  TivaType returnType;
};

struct Function : public AstNode {
 public:
  constexpr Function(std::unique_ptr<FunctionPrototype> prototype,
                     std::unique_ptr<AstNode> expression)
      : prototype{std::move(prototype)},
        body{std::move(expression)} {}
  void accept(AstNodeVisitor *visitor) override;

 public:
  std::unique_ptr<FunctionPrototype> prototype;
  std::unique_ptr<AstNode> body;
};

struct TranslationUnitAstNode : public AstNode {
 public:
  constexpr TranslationUnitAstNode(
      std::vector<std::unique_ptr<AstNode>> &&declarations)
      : declarations{std::move(declarations)} {}

  void accept(AstNodeVisitor *visitor) override;

 public:
  std::vector<std::unique_ptr<AstNode>> declarations;
};

struct AstNodeVisitor {
  AstNodeVisitor() = default;
  AstNodeVisitor(const AstNodeVisitor &) = default;
  auto operator=(const AstNodeVisitor &) -> AstNodeVisitor & = default;
  AstNodeVisitor(AstNodeVisitor &&) = delete;
  auto operator=(AstNodeVisitor &&) -> AstNodeVisitor & = delete;

  virtual ~AstNodeVisitor() = default;
  void dispatch(AstNode *node) { node->accept(this); }
  virtual void visit(IntegerAstNode *) = 0;
  virtual void visit(FloatAstNode *) = 0;
  virtual void visit(BooleanAstNode *) = 0;
  virtual void visit(VariableAstNode *) = 0;
  virtual void visit(AssignmentAstNode *) = 0;
  virtual void visit(CallAstNode *) = 0;
  virtual void visit(BinaryExprAstNode *) = 0;
  virtual void visit(BlockAstNode *) = 0;
  virtual void visit(IfElseAstNode *) = 0;
  virtual void visit(LetAstNode *) = 0;
  virtual void visit(CastAstNode *) = 0;
  virtual void visit(FunctionPrototype *) = 0;
  virtual void visit(Function *) = 0;
  virtual void visit(TranslationUnitAstNode *) = 0;
};

inline void IntegerAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void FloatAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void BooleanAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void VariableAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void AssignmentAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void CallAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void BinaryExprAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void BlockAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void IfElseAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void LetAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void FunctionPrototype::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void CastAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
inline void Function::accept(AstNodeVisitor *visitor) { visitor->visit(this); }
inline void TranslationUnitAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}
