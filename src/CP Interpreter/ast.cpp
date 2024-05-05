#include <utility>
#include <numeric>

#include "ast.hpp"
#include "util.hpp"


using namespace parser;


SemanticValue::SemanticValue(parser::Type current_type, parser::Type array_type, std::string type_name,
	ASTExprNode* expr, bool is_const, unsigned int row, unsigned int col)
	:type(current_type), array_type(array_type), dim(std::vector<ASTExprNode*>()), type_name(type_name),
	struct_vars(std::map<std::string, SemanticValue*>()), expr(expr), is_const(is_const), row(row), col(col) {}

SemanticValue::SemanticValue(parser::Type current_type, parser::Type array_type, std::vector<ASTExprNode*> dim, std::string type_name,
	std::map<std::string, SemanticValue*> struct_vars, ASTExprNode* expr, bool is_const, unsigned int row, unsigned int col)
	:type(current_type), array_type(array_type), dim(dim), type_name(type_name),
	struct_vars(struct_vars), expr(expr), is_const(is_const), row(row), col(col) {}

void SemanticValue::copy_from(SemanticValue* value) {
	type = value->type;
	array_type = value->array_type;
	dim = value->dim;
	type_name = value->type_name;
	struct_vars = value->struct_vars;
	expr = value->expr;
	is_const = value->is_const;
	row = value->row;
	col = value->col;
}

SemanticVariable::SemanticVariable(std::string identifier, Type type, bool is_const, parser::Type current_type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::map<std::string, SemanticValue*> struct_vars, ASTExprNode* expr, bool is_expr_const,
	unsigned int row, unsigned int col, bool is_parameter)
	: identifier(identifier), type(type), is_const(is_const), row(row), col(col), is_parameter(is_parameter) {
	this->value = new SemanticValue_t(current_type, array_type, dim, type_name, struct_vars, expr, is_expr_const, row, col);
}

SemanticVariable::SemanticVariable(std::string identifier, Type type, bool is_const, SemanticValue* expr,
	unsigned int row, unsigned int col, bool is_parameter)
	: identifier(identifier), type(type), is_const(is_const), value(expr), row(row), col(col), is_parameter(is_parameter) { }

void SemanticVariable::copy_from(SemanticVariable* var) {
	identifier = var->identifier;
	type = var->type;
	is_parameter = var->is_parameter;
	value->copy_from(var->value);
	is_const = value->is_const;
	row = var->row;
	col = var->col;
}

VariableDefinition::VariableDefinition(std::string identifier, Type type, std::string type_name,
	Type any_type, Type array_type, std::vector<ASTExprNode*> dim, unsigned int row, unsigned int col)
	: identifier(identifier), type(type), type_name(type_name),
	array_type(array_type), dim(dim), row(row), col(col) {}

StructureDefinition::StructureDefinition(std::string identifier, std::vector<VariableDefinition_t> variables, unsigned int row, unsigned int col)
	: identifier(identifier), variables(variables), row(row), col(col) {}

FunctionDefinition::FunctionDefinition(std::string identifier, Type type, std::string type_name, Type any_type, Type array_type,
	std::vector<ASTExprNode*> dim, std::vector<parser::Type> signature, std::vector<parser::VariableDefinition_t> parameters,
	ASTBlockNode* block, unsigned int row, unsigned int col)
	: identifier(identifier), type(type), type_name(type_name), any_type(any_type), array_type(array_type),
	dim(dim), signature(signature), parameters(parameters), block(block), row(row), col(col) {};


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name)
	: statements(std::move(statements)), name(name), alias("") {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::string library, std::string alias, unsigned int row, unsigned int col)
	: library(std::move(library)), alias(std::move(alias)), row(row), col(col) {}

ASTDeclarationNode::ASTDeclarationNode(Type type, std::string type_name, std::string identifier, ASTExprNode* expr, bool is_const,
	Type array_type, std::vector<ASTExprNode*> dim, unsigned int row, unsigned int col)
	: type(type), type_name(std::move(type_name)), identifier(std::move(identifier)), expr(expr), is_const(is_const), array_type(array_type), dim(dim), row(row), col(col) {}

ASTAssignmentNode::ASTAssignmentNode(std::vector<std::string> identifier_vector, std::string op, ASTExprNode* expr, std::vector<ASTExprNode*> access_vector, unsigned int row, unsigned int col)
	: identifier_vector(identifier_vector), expr(expr), op(std::move(op)), access_vector(access_vector), row(row), col(col) {}

ASTPrintNode::ASTPrintNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: expr(expr), row(row), col(col) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTNode*> statements, unsigned int row, unsigned int col)
	: statements(std::move(statements)), row(row), col(col) {}

ASTContinueNode::ASTContinueNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTBreakNode::ASTBreakNode(unsigned int row, unsigned int col)
	: row(row), col(col) {}

ASTSwitchNode::ASTSwitchNode(ASTExprNode* condition, std::vector<ASTNode*>* statements, std::map<ASTExprNode*, unsigned int>* case_blocks, unsigned int default_block, unsigned int row, unsigned int col)
	: condition(condition), statements(statements), case_blocks(case_blocks), default_block(default_block), parsed_case_blocks(new std::map<unsigned int, unsigned int>()), row(row), col(col) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, std::vector<ASTElseIfNode*> else_ifs, ASTBlockNode* else_block, unsigned int row, unsigned int col)
	: condition(condition), if_block(if_block), else_ifs(else_ifs), else_block(else_block), row(row), col(col) {}

ASTForNode::ASTForNode(std::array<ASTNode*, 3> dci, ASTBlockNode* block, unsigned int row, unsigned int col)
	: dci(dci), block(block), row(row), col(col) {}

ASTForEachNode::ASTForEachNode(ASTNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col)
	: itdecl(itdecl), collection(collection), block(block), row(row), col(col) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: condition(condition), block(block), row(row), col(col) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<VariableDefinition_t> parameters,
	Type type, std::string type_name, Type array_type, std::vector<ASTExprNode*> dim, ASTBlockNode* block, unsigned int row, unsigned int col)
	: identifier(std::move(identifier)), parameters(std::move(parameters)), type(type), type_name(type_name),
	array_type(array_type), dim(dim), block(block), row(row), col(col) {
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

ASTIdentifierNode::ASTIdentifierNode(std::vector<std::string> identifier_vector, std::vector<ASTExprNode*> access_vector, unsigned int row, unsigned int col)
	: identifier_vector(identifier_vector), access_vector(access_vector), row(row), col(col) {}

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

	unsigned int ASTLiteralNode<cp_bool>::hash(visitor::Visitor* v) {
		return v->hash(this);
	}

	template<>
	void ASTLiteralNode<cp_int>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_int>::hash(visitor::Visitor* v) {
		return v->hash(this);
	}

	template<>
	void ASTLiteralNode<cp_float>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_float>::hash(visitor::Visitor* v) {
		return v->hash(this);
	}

	template<>
	void ASTLiteralNode<cp_char>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_char>::hash(visitor::Visitor* v) {
		return v->hash(this);
	}

	template<>
	void ASTLiteralNode<cp_string>::accept(visitor::Visitor* v) {
		v->visit(this);
	}

	unsigned int ASTLiteralNode<cp_string>::hash(visitor::Visitor* v) {
		return v->hash(this);
	}
}

void ASTArrayConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTArrayConstructorNode::hash(visitor::Visitor* v) { return 0; }

void ASTStructConstructorNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTStructConstructorNode::hash(visitor::Visitor* v) { return 0; }

void ASTBinaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTBinaryExprNode::hash(visitor::Visitor* v) { return 0; }

void ASTFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTFunctionCallNode::hash(visitor::Visitor* v) { return 0; }

void ASTTypeNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTypeNode::hash(visitor::Visitor* v) { return 0; }

void ASTRoundNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTRoundNode::hash(visitor::Visitor* v) { return 0; }

void ASTLenNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTLenNode::hash(visitor::Visitor* v) { return 0; }

void ASTReadNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTReadNode::hash(visitor::Visitor* v) { return 0; }

void ASTIdentifierNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTIdentifierNode::hash(visitor::Visitor* v) {
	return v->hash(this);
}

void ASTUnaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTUnaryExprNode::hash(visitor::Visitor* v) { return 0; }

void ASTTypeParseNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTypeParseNode::hash(visitor::Visitor* v) { return 0; }

void ASTNullNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTNullNode::hash(visitor::Visitor* v) { return 0; }

void ASTThisNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTThisNode::hash(visitor::Visitor* v) { return 0; }

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

void ASTContinueNode::accept(visitor::Visitor* v) {
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
