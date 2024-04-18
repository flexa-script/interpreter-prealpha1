#include <utility>

#include "ast.h"


using namespace parser;


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name)
	: statements(std::move(statements)), name(name) {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::string library, unsigned int row, unsigned int col)
	: library(std::move(library)), row(row), col(col) {}

ASTDeclarationNode::ASTDeclarationNode(TYPE type, std::string typeName, std::string identifier, ASTExprNode* expr, bool isConst, TYPE arrayType, std::vector<int> dim, unsigned int row, unsigned int col)
	: type(type), typeName(std::move(typeName)), identifier(std::move(identifier)), expr(expr), isConst(isConst), arrayType(arrayType), dim(dim), row(row), col(col) {}

ASTAssignmentNode::ASTAssignmentNode(std::string identifier, std::vector<std::string> identifierVector, ASTExprNode* expr, std::vector<ASTExprNode*> accessVector, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), identifierVector(identifierVector), expr(expr), accessVector(accessVector), row(row), col(col) {}

ASTPrintNode::ASTPrintNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTNode*> statements, unsigned int row, unsigned int col)
	: statements(std::move(statements)), row(row), col(col) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* ifBlock, unsigned int row, unsigned int col, std::vector<ASTElseIfNode*> elseIf, ASTBlockNode* elseBlock)
	: condition(condition), ifBlock(ifBlock), row(row), col(col), elseIf(elseIf), elseBlock(elseBlock) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<VariableDefinition_t> parameters, TYPE type, std::string typeName, ASTBlockNode* block, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), type(type), typeName(typeName), block(block), row(row), col(col) {
	// generate signature
	this->signature = std::vector<TYPE>();

	for (auto param : this->parameters) {
		variableNames.push_back(param.identifier);
		signature.push_back(param.type);
	}
}

ASTStructDefinitionNode::ASTStructDefinitionNode(std::string identifier, std::vector<VariableDefinition_t> variables, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), variables(std::move(variables)), row(row), col(col) {}


// Expression Nodes
ASTArrayConstructorNode::ASTArrayConstructorNode(std::vector<ASTExprNode*> values, unsigned int row, unsigned int col)
	: values(values), row(row), col(col) {}

ASTStructConstructorNode::ASTStructConstructorNode(std::string typeName, std::map<std::string, ASTExprNode*> values, unsigned int row, unsigned int col)
	: typeName(typeName), values(values), row(row), col(col) {}

ASTNullNode::ASTNullNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTThisNode::ASTThisNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTBinaryExprNode::ASTBinaryExprNode(std::string op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col)
	: op(std::move(op)), left(left), right(right), row(row), col(col) {}

ASTIdentifierNode::ASTIdentifierNode(std::string identifier, std::vector<std::string> identifierVector, std::vector<unsigned int> accessVector, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), identifierVector(identifierVector), accessVector(accessVector), row(row), col(col) {}

ASTUnaryExprNode::ASTUnaryExprNode(std::string unaryOp, ASTExprNode* expr, unsigned int row, unsigned int col)
	: unaryOp(std::move(unaryOp)), expr(expr), row(row), col(col) {}

ASTFunctionCallNode::ASTFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), row(row), col(col) {}

ASTTypeParseNode::ASTTypeParseNode(TYPE type, ASTExprNode* expr, unsigned int row, unsigned int col)
	: type(type), expr(expr), row(row), col(col) {}

ASTRoundNode::ASTRoundNode(ASTExprNode* expr, unsigned int ndigits, unsigned int row, unsigned int col)
	: expr(expr), ndigits(ndigits), row(row), col(col) {}

ASTLenNode::ASTLenNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTTypeNode::ASTTypeNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTReadNode::ASTReadNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}


// Accept functions for visitors
void ASTBinaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

namespace parser {
	template<>
	void ASTLiteralNode<cp_bool>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<cp_int>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<cp_float>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<cp_char>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	template<>
	void ASTLiteralNode<cp_string>::accept(visitor::Visitor* v) {
		v->visit(this);
	}
}

void ASTArrayConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTStructConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTTypeNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTRoundNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTLenNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReadNode::accept(visitor::Visitor* v) {
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

void ASTTypeParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTPrintNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTNullNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTThisNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReturnNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTBlockNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTElseIfNode::accept(visitor::Visitor* v) {
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

void ASTStructDefinitionNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTUsingNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTProgramNode::accept(visitor::Visitor* v) {
	v->visit(this);
}
