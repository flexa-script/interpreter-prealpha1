#include <utility>

#include "ast.h"


using namespace parser;


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name)
	: statements(std::move(statements)), name(name) {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::string library, unsigned int row, unsigned int col)
	: library(std::move(library)), row(row), col(col) {}

ASTDeclarationNode::ASTDeclarationNode(TYPE type, std::string identifier, ASTExprNode* expr, unsigned int row, unsigned int col)
	: type(type), identifier(std::move(identifier)), expr(expr), row(row), col(col) {}

ASTAssignmentNode::ASTAssignmentNode(std::string identifier, ASTExprNode* expr, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), expr(expr), row(row), col(col) {}

ASTFloatParseNode::ASTFloatParseNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTIntParseNode::ASTIntParseNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTStringParseNode::ASTStringParseNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTPrintNode::ASTPrintNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTReadNode::ASTReadNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTFunctionCallNode::ASTFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), row(row), col(col) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTStatementNode*> statements, unsigned int row, unsigned int col)
	: statements(std::move(statements)), row(row), col(col) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* ifBlock, unsigned int row, unsigned int col, ASTBlockNode* elseBlock)
	: condition(condition), ifBlock(ifBlock), row(row), col(col), elseBlock(elseBlock) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<std::pair<std::string, TYPE>> parameters, TYPE type, ASTBlockNode* block, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), type(type), block(block), row(row), col(col) {
	// generate signature
	this->signature = std::vector<TYPE>();

	for (auto param : this->parameters) {
		variableNames.push_back(param.first);
		signature.push_back(param.second);
	}
}


// Expression Nodes
ASTBinaryExprNode::ASTBinaryExprNode(std::string op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col)
	: op(std::move(op)), left(left), right(right), row(row), col(col) {}

ASTIdentifierNode::ASTIdentifierNode(std::string identifier, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), row(row), col(col) {}

ASTUnaryExprNode::ASTUnaryExprNode(std::string unaryOp, ASTExprNode* expr, unsigned int row, unsigned int col)
	: unaryOp(std::move(unaryOp)), expr(expr), row(row), col(col) {}

ASTExprFunctionCallNode::ASTExprFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), row(row), col(col) {}

ASTExprReadNode::ASTExprReadNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}


// Accept functions for visitors
void ASTBinaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

namespace parser {
	template<>
	void ASTLiteralNode<bool>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<__int64_t>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<long double>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<char>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<std::string>::accept(visitor::Visitor* v) {
		v->visit(this);
	}
}

void ASTExprFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTExprReadNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTIdentifierNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTUnaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTDeclarationNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTAssignmentNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTFloatParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTIntParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTStringParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTPrintNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReadNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReturnNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTBlockNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTIfNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTWhileNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTFunctionDefinitionNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTUsingNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTProgramNode::accept(visitor::Visitor* v) {
	v->visit(this);
}
