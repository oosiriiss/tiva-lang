
#pragma once
#include "lexer.hpp"
#include "utility.hpp"
#include <llvm/IR/Value.h>
#include <memory>
#include <string_view>

void initalizeLlvmModule();
void printGeneratedCode();

class AstNode {
public:
  virtual ~AstNode() = default;
  virtual auto codegen() const -> llvm::Value * = 0;
};

struct NumberAstNode : public AstNode {
  double val;

  constexpr NumberAstNode(std::string_view numStr)
      : val{util::toNumber<double>(numStr)} {}
  auto codegen() const -> llvm::Value * override;
};

struct VariableAstNode : public AstNode {
  std::string name;

  constexpr VariableAstNode(std::string_view name) : name{name} {}
  auto codegen() const -> llvm::Value * override;
};

struct BinaryExprAstNode : public AstNode {

  TokenType op;
  std::unique_ptr<AstNode> lhs;
  std::unique_ptr<AstNode> rhs;

  constexpr BinaryExprAstNode(TokenType op, std::unique_ptr<AstNode> lhs,
                              std::unique_ptr<AstNode> rhs)
      : op{op}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
  auto codegen() const -> llvm::Value * override;
};
