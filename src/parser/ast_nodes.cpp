#include "ast_nodes.hpp"

void NumberAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void VariableAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void AssignmentAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}

void CallAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void BinaryExprAstNode::accept(AstNodeVisitor *visitor) {
  visitor->visit(this);
}

void BlockAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void IfElseAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void LetAstNode::accept(AstNodeVisitor *visitor) { visitor->visit(this); }

void Function::accept(AstNodeVisitor *visitor) { visitor->visit(this); }
