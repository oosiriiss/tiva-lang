
#pragma once
#include "lexer.hpp"
#include "utility.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string_view>

void initalizeLlvmModule();
void printGeneratedCode();
void emitObjectFile(std::string_view fileName);

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

struct CallAstNode : public AstNode {
  std::string toCall;
  std::vector<std::unique_ptr<AstNode>> args;

  constexpr CallAstNode(std::string_view toCall,
                        std::vector<std::unique_ptr<AstNode>> &&args)
      : toCall{toCall}, args{std::move(args)} {}
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

struct BlockAstNode : public AstNode {
  std::string blockName;
  std::vector<std::unique_ptr<AstNode>> expressions;

  constexpr BlockAstNode(std::string_view name,
                         std::vector<std::unique_ptr<AstNode>> &&expressions)
      : blockName{name}, expressions{std::move(expressions)} {}
  auto codegen() const -> llvm::Value * override;
};

struct FunctionPrototype {
  std::string name;
  std::vector<std::string> args;

  constexpr FunctionPrototype(std::string_view name,
                              std::vector<std::string> &&argNames)
      : name{name}, args{argNames} {}
  auto codegen() const -> llvm::Function *;
};

struct Function {
  std::unique_ptr<FunctionPrototype> prototype;
  std::unique_ptr<AstNode> body;

  constexpr Function(std::unique_ptr<FunctionPrototype> prototype,
                     std::unique_ptr<AstNode> expression)
      : prototype{std::move(prototype)}, body{std::move(expression)} {}
  auto codegen() const -> llvm::Function *;
};
