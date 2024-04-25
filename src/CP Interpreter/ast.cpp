#include <utility>
#include <numeric>

#include "ast.hpp"
#include "util.hpp"


using namespace parser;


VariableDefinition::VariableDefinition(std::string identifier, Type type, std::string type_name, Type array_type, std::vector<ASTExprNode*> dim, bool is_const, bool has_value, unsigned int row, unsigned int col, bool is_parameter)
	: identifier(identifier), type(type), type_name(type_name), array_type(array_type), dim(dim), is_const(is_const), has_value(has_value), row(row), col(col), is_parameter(is_parameter) {};

VariableDefinition::VariableDefinition()
	: identifier(""), type(Type::T_ND), type_name(""), array_type(Type::T_ND), dim(std::vector<ASTExprNode*>()), is_const(false), has_value(false), row(0), col(0), is_parameter(false) {};

StructureDefinition::StructureDefinition(std::string identifier, std::vector<VariableDefinition_t> variables, unsigned int row, unsigned int col)
	: identifier(identifier), variables(variables), row(row), col(col) {};

StructureDefinition::StructureDefinition()
	: identifier(""), variables(std::vector<VariableDefinition_t>()), row(0), col(0) {};

FunctionDefinition::FunctionDefinition(std::string identifier, Type type, std::string type_name, std::vector<parser::Type> signature, bool is_any, unsigned int row, unsigned int col)
	: identifier(identifier), type(type), type_name(type_name), signature(signature), is_any(is_any), row(row), col(col) {};

FunctionDefinition::FunctionDefinition()
	: identifier(""), type(Type::T_ND), type_name(""), signature(std::vector<parser::Type>()), is_any(false), row(0), col(0) {};


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name)
	: statements(std::move(statements)), name(name) {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::string library, unsigned int row, unsigned int col)
	: library(std::move(library)), row(row), col(col) {}

ASTDeclarationNode::ASTDeclarationNode(Type type, std::string type_name, std::string identifier, ASTExprNode* expr, bool is_const, Type array_type, std::vector<ASTExprNode*> dim, unsigned int row, unsigned int col)
	: type(type), type_name(std::move(type_name)), identifier(std::move(identifier)), expr(expr), is_const(is_const), array_type(array_type), dim(dim), row(row), col(col) {}

ASTAssignmentNode::ASTAssignmentNode(std::string identifier, std::vector<std::string> identifier_vector, ASTExprNode* expr, std::vector<ASTExprNode*> access_vector, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), identifier_vector(identifier_vector), expr(expr), access_vector(access_vector), row(row), col(col) {}

ASTPrintNode::ASTPrintNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTNode*> statements, unsigned int row, unsigned int col)
	: statements(std::move(statements)), row(row), col(col) {}

ASTBreakNode::ASTBreakNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTSwitchNode::ASTSwitchNode(ASTExprNode* condition, std::vector<ASTNode*>* statements, std::map<ASTExprNode*, unsigned int>* case_blocks, unsigned int default_block, unsigned int row, unsigned int col)
	: condition(condition), statements(statements), case_blocks(case_blocks), default_block(default_block), parsed_case_blocks(new std::map<unsigned int, unsigned int>()), row(row), col(col) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, std::vector<ASTElseIfNode*> else_ifs, unsigned int row, unsigned int col, ASTBlockNode* else_block)
	: condition(condition), if_block(if_block), else_ifs(else_ifs), row(row), col(col), else_block(else_block) {}

ASTForNode::ASTForNode(std::array<ASTNode*, 3> dci, ASTBlockNode* block, unsigned int row, unsigned int col)
	: dci(dci), block(block), row(row), col(col) {}

ASTForEachNode::ASTForEachNode(ASTNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col)
	: itdecl(itdecl), collection(collection), block(block), row(row), col(col) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<VariableDefinition_t> parameters, Type type, std::string type_name, ASTBlockNode* block, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), type(type), type_name(type_name), block(block), row(row), col(col) {
	// generate signature
	this->signature = std::vector<Type>();

	for (auto param : this->parameters) {
		variable_names.push_back(param.identifier);
		signature.push_back(param.type);
	}
}

ASTStructDefinitionNode::ASTStructDefinitionNode(std::string identifier, std::vector<VariableDefinition_t> variables, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), variables(std::move(variables)), row(row), col(col) {}


// Expression Nodes
ASTArrayConstructorNode::ASTArrayConstructorNode(std::vector<ASTExprNode*> values, unsigned int row, unsigned int col)
	: values(values), row(row), col(col) {}

ASTStructConstructorNode::ASTStructConstructorNode(std::string type_name, std::map<std::string, ASTExprNode*> values, unsigned int row, unsigned int col)
	: type_name(type_name), values(values), row(row), col(col) {}

ASTNullNode::ASTNullNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTThisNode::ASTThisNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTBinaryExprNode::ASTBinaryExprNode(std::string op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col)
	: op(std::move(op)), left(left), right(right), row(row), col(col) {}

ASTIdentifierNode::ASTIdentifierNode(std::string identifier, std::vector<std::string> identifier_vector, std::vector<ASTExprNode*> access_vector, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), identifier_vector(identifier_vector), access_vector(access_vector), row(row), col(col) {}

ASTUnaryExprNode::ASTUnaryExprNode(std::string unary_op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: unary_op(std::move(unary_op)), expr(expr), row(row), col(col) {}

ASTFunctionCallNode::ASTFunctionCallNode(std::string identifier, std::vector<ASTExprNode*> parameters, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), row(row), col(col) {}

ASTTypeParseNode::ASTTypeParseNode(Type type, ASTExprNode* expr, unsigned int row, unsigned int col)
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

namespace parser {
	template<>
	void ASTLiteralNode<cp_bool>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_bool>::hash() {
		return static_cast<unsigned int>(val);
	}

	template<>
	void ASTLiteralNode<cp_int>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_int>::hash() {
		return static_cast<unsigned int>(val);
	}

	template<>
	void ASTLiteralNode<cp_float>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_float>::hash() {
		return static_cast<unsigned int>(val);
	}

	template<>
	void ASTLiteralNode<cp_char>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_char>::hash() {
		return static_cast<unsigned int>(val);
	}

	template<>
	void ASTLiteralNode<cp_string>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_string>::hash() {
		return axe::hashcode(val);
	}
}

void ASTArrayConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTArrayConstructorNode::hash() {
	unsigned int h = 0;
	for (auto& expr : values) {
		h = h * 31 + expr->hash();
	}
	return h;
}

void ASTStructConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTStructConstructorNode::hash() {
	unsigned int h = 0;
	for (auto& expr : values) {
		h = h * 31 + axe::hashcode(expr.first) + expr.second->hash();
	}
	return axe::hashcode(type_name) + h;
}

void ASTBinaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTBinaryExprNode::hash() { return 0; }

void ASTFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTFunctionCallNode::hash() { return 0; }

void ASTTypeNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTypeNode::hash() { return 0; }

void ASTRoundNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTRoundNode::hash() { return 0; }

void ASTLenNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTLenNode::hash() { return 0; }

void ASTReadNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTReadNode::hash() { return 0; }

void ASTIdentifierNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTIdentifierNode::hash() {
	return -1;
}

void ASTUnaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTUnaryExprNode::hash() { return 0; }

void ASTTypeParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTypeParseNode::hash() { return 0; }

void ASTNullNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTNullNode::hash() { return 0; }

void ASTThisNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTThisNode::hash() { return 0; }

void ASTDeclarationNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTAssignmentNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTPrintNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReturnNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTBlockNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTBreakNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTSwitchNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTElseIfNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTIfNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTForNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTForEachNode::accept(visitor::Visitor* v) {
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
