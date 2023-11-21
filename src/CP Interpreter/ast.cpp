#include <utility>

#include "ast.h"


using namespace parser;


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name)
	: statements(std::move(statements)), name(name) {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::string library, unsigned int lineNumber) :
	library(std::move(library)), lineNumber(lineNumber) {}

ASTDeclarationNode::ASTDeclarationNode(TYPE type, std::string identifier, ASTExprNode* expr, unsigned int lineNumber) :
	type(type), identifier(std::move(identifier)), expr(expr), lineNumber(lineNumber) {}

ASTAssignmentNode::ASTAssignmentNode(std::string identifier, ASTExprNode* expr, unsigned int lineNumber) :
	identifier(std::move(identifier)), expr(expr), lineNumber(lineNumber) {}

ASTFloatParseNode::ASTFloatParseNode(ASTExprNode* expr, unsigned int lineNumber) :
	expr(expr), lineNumber(lineNumber) {}

ASTIntParseNode::ASTIntParseNode(ASTExprNode* expr, unsigned int lineNumber) :
	expr(expr), lineNumber(lineNumber) {}

ASTStringParseNode::ASTStringParseNode(ASTExprNode* expr, unsigned int lineNumber) :
	expr(expr), lineNumber(lineNumber) {}

ASTPrintNode::ASTPrintNode(ASTExprNode* expr, unsigned int lineNumber) :
	expr(expr), lineNumber(lineNumber) {}

ASTReadNode::ASTReadNode(unsigned int lineNumber) :
	lineNumber(lineNumber) {}

ASTFunctionCallNode::ASTFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int lineNumber) :
	identifier(std::move(identifier)), parameters(std::move(parameters)), lineNumber(lineNumber) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int lineNumber) :
	expr(expr), lineNumber(lineNumber) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTStatementNode*> statements, unsigned int lineNumber)
	: statements(std::move(statements)), lineNumber(lineNumber) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* ifBlock, unsigned int lineNumber, ASTBlockNode* elseBlock)
	: condition(condition), ifBlock(ifBlock), lineNumber(lineNumber), elseBlock(elseBlock) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int lineNumber)
	: condition(condition), block(block), lineNumber(lineNumber) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<std::pair<std::string, TYPE>> parameters, TYPE type, ASTBlockNode* block, unsigned int lineNumber)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), type(type), block(block), lineNumber(lineNumber) {
	// generate signature
	this->signature = std::vector<TYPE>();

	for (auto param : this->parameters) {
		variableNames.push_back(param.first);
		signature.push_back(param.second);
	}
}


// Expression Nodes
ASTBinaryExprNode::ASTBinaryExprNode(std::string op, ASTExprNode* left, ASTExprNode* right, unsigned int lineNumber) :
	op(std::move(op)), left(left), right(right), lineNumber(lineNumber) {}

ASTIdentifierNode::ASTIdentifierNode(std::string identifier, unsigned int lineNumber) :
	identifier(std::move(identifier)), lineNumber(lineNumber) {}

ASTUnaryExprNode::ASTUnaryExprNode(std::string unaryOp, ASTExprNode* expr, unsigned int lineNumber) :
	unaryOp(std::move(unaryOp)), expr(expr), lineNumber(lineNumber) {}

ASTExprFunctionCallNode::ASTExprFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int lineNumber) :
	identifier(std::move(identifier)), parameters(std::move(parameters)), lineNumber(lineNumber) {}

ASTExprReadNode::ASTExprReadNode(unsigned int lineNumber) :
	lineNumber(lineNumber) {}


// Accept functions for visitors
void ASTBinaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

namespace parser {
	template<>
	void ASTLiteralNode<__int64_t>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<long double>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<bool>::accept(visitor::Visitor* v) {
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
