#include <utility>
#include <numeric>

#include "ast.hpp"
#include "vendor/util.hpp"


using namespace parser;


TypeDefinition::TypeDefinition(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::string type_name_space)
	: type(type), type_name(type_name), type_name_space(type_name_space), array_type(array_type), dim(dim) {}

TypeDefinition TypeDefinition::get_basic(parser::Type type) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "");
}

TypeDefinition TypeDefinition::get_array(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim) {
	return TypeDefinition(type, array_type, dim, "", "");
}

TypeDefinition TypeDefinition::get_struct(parser::Type type, std::string type_name, std::string type_name_space) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), type_name, type_name_space);
}

SemanticValue::SemanticValue(parser::Type type, unsigned int row, unsigned int col)
	: hash(0), is_const(false),
	TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""), CodePosition(row, col) {}

SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::string type_name_space, unsigned int hash, bool is_const, unsigned int row, unsigned int col)
	: hash(hash), is_const(is_const),
	TypeDefinition(type, array_type, dim, type_name, type_name_space), CodePosition(row, col) {}

void SemanticValue::copy_from(SemanticValue* value) {
	type = value->type;
	array_type = value->array_type;
	dim = value->dim;
	type_name = value->type_name;
	hash = value->hash;
	is_const = value->is_const;
	row = value->row;
	col = value->col;
}

SemanticVariable::SemanticVariable(std::string identifier, Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::string type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col)
	: identifier(identifier), value(value), is_const(is_const),
	TypeDefinition(type, array_type, dim, type_name, type_name_space), CodePosition(row, col) {}

void SemanticVariable::copy_from(SemanticVariable* var) {
	identifier = var->identifier;
	type = var->type;
	array_type = var->array_type;
	dim = var->dim;
	type_name = var->type_name;
	is_const = value->is_const;
	row = var->row;
	col = var->col;

	delete value;
	value = new SemanticValue();
	value->copy_from(var->value);
}

VariableDefinition::VariableDefinition(std::string identifier, Type type, std::string type_name, std::string type_name_space,
	Type array_type, std::vector<ASTExprNode*> dim, unsigned int row, unsigned int col)
	: identifier(identifier),
	TypeDefinition(type, array_type, dim, type_name, type_name_space), CodePosition(row, col) {}

VariableDefinition VariableDefinition::get_basic(std::string identifier, parser::Type type, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), row, col);
}

VariableDefinition VariableDefinition::get_array(std::string identifier, parser::Type type,
	parser::Type array_type, std::vector<ASTExprNode*> dim, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", array_type, dim, row, col);
}

VariableDefinition VariableDefinition::get_struct(std::string identifier, parser::Type type,
	std::string type_name, std::string type_name_space, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, type_name, type_name_space, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), row, col);
}

StructureDefinition::StructureDefinition(std::string identifier, std::vector<VariableDefinition> variables, unsigned int row, unsigned int col)
	: CodePosition(row, col), identifier(identifier), variables(variables) {}

FunctionDefinition::FunctionDefinition(std::string identifier, Type type, std::string type_name, std::string type_name_space, Type array_type,
	std::vector<ASTExprNode*> dim, std::vector<parser::TypeDefinition> signature, std::vector<parser::VariableDefinition> parameters, unsigned int row, unsigned int col)
	: identifier(identifier), signature(signature), parameters(parameters), is_variable(false),
	TypeDefinition(type, array_type, dim, type_name, type_name_space), CodePosition(row, col) {}

FunctionDefinition::FunctionDefinition(std::string identifier, unsigned int row, unsigned int col)
	: identifier(identifier), signature(std::vector<parser::TypeDefinition>()), parameters(std::vector<parser::VariableDefinition>()), is_variable(true),
	TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""), CodePosition(row, col) {}

Identifier::Identifier(std::string identifier, std::vector<ASTExprNode*> access_vector)
	: identifier(identifier), access_vector(access_vector) {}


// Program Node
ASTProgramNode::ASTProgramNode(std::vector<ASTNode*> statements, std::string name, std::string alias)
	: ASTNode(row, col), statements(std::move(statements)), name(name), alias(alias), libs(std::vector<std::string>()) {}

// Statement Nodes
ASTUsingNode::ASTUsingNode(std::vector<std::string> library, std::string alias, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col),
	library(std::move(library)), alias(std::move(alias)) {}

ASTAsNamespaceNode::ASTAsNamespaceNode(std::string nmspace, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), nmspace(std::move(nmspace)) {}

ASTDeclarationNode::ASTDeclarationNode(std::string identifier, Type type, Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::string type_name_space, ASTExprNode* expr, bool is_const,
	unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(std::move(identifier)), expr(expr), is_const(is_const) {}

ASTAssignmentNode::ASTAssignmentNode(std::vector<Identifier> identifier_vector, std::string nmspace,
	std::string op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col),
	identifier_vector(identifier_vector), nmspace(nmspace), expr(expr), op(std::move(op)) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), expr(expr) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTNode*> statements, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), statements(std::move(statements)) {}

ASTContinueNode::ASTContinueNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTBreakNode::ASTBreakNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTExitNode::ASTExitNode(ASTExprNode* exit_code,  unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), exit_code(exit_code){}

ASTSwitchNode::ASTSwitchNode(ASTExprNode* condition, std::vector<ASTNode*>* statements,
	std::map<ASTExprNode*, unsigned int>* case_blocks, unsigned int default_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), statements(statements), case_blocks(case_blocks),
	default_block(default_block), parsed_case_blocks(new std::map<unsigned int, unsigned int>()) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTEnumNode::ASTEnumNode(std::vector<std::string> identifiers, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifiers(identifiers) {}

ASTTryCatchNode::ASTTryCatchNode(ASTStatementNode* decl, ASTBlockNode* try_block, ASTBlockNode* catch_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), decl(decl), try_block(try_block), catch_block(catch_block) {}

ASTThrowNode::ASTThrowNode(ASTExprNode* error, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), error(error) {}

ASTReticencesNode::ASTReticencesNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, std::vector<ASTElseIfNode*> else_ifs,
	ASTBlockNode* else_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), if_block(if_block), else_ifs(else_ifs), else_block(else_block) {}

ASTForNode::ASTForNode(std::array<ASTNode*, 3> dci, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), dci(dci), block(block) {}

ASTForEachNode::ASTForEachNode(ASTDeclarationNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), itdecl(itdecl), collection(collection), block(block) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTDoWhileNode::ASTDoWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTWhileNode(condition, block, row, col) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(std::string identifier, std::vector<VariableDefinition> parameters,
	Type type, std::string type_name, std::string type_name_space, Type array_type, std::vector<ASTExprNode*> dim,
	ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(std::move(identifier)), parameters(std::move(parameters)), block(block) {
	// generate signature
	this->signature = std::vector<TypeDefinition>();

	for (auto param : this->parameters) {
		variable_names.push_back(param.identifier);
		auto td = TypeDefinition(param.type, param.array_type, param.dim, param.type_name, param.type_name_space);
		signature.push_back(td);
	}
}

ASTTypingNode::ASTTypingNode(std::string image, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), image(image), expr(expr) {}

ASTStructDefinitionNode::ASTStructDefinitionNode(std::string identifier, std::vector<VariableDefinition> variables,
	unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifier(std::move(identifier)), variables(std::move(variables)) {}


// Expression Nodes
ASTArrayConstructorNode::ASTArrayConstructorNode(std::vector<ASTExprNode*> values, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), values(values) {}

ASTStructConstructorNode::ASTStructConstructorNode(std::string type_name, std::string nmspace,
	std::map<std::string, ASTExprNode*> values, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), type_name(type_name), nmspace(nmspace), values(values) {}

ASTNullNode::ASTNullNode(unsigned int row, unsigned int col)
	: ASTExprNode(row, col) {}

ASTThisNode::ASTThisNode(unsigned int row, unsigned int col)
	: ASTExprNode(row, col) {}

ASTBinaryExprNode::ASTBinaryExprNode(std::string op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), op(std::move(op)), left(left), right(right) {}

ASTUnaryExprNode::ASTUnaryExprNode(std::string unary_op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), unary_op(std::move(unary_op)), expr(expr) {}

ASTIdentifierNode::ASTIdentifierNode(std::vector<Identifier> identifier_vector, std::string nmspace, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier_vector(identifier_vector), nmspace(nmspace) {}

ASTTernaryNode::ASTTernaryNode(ASTExprNode* condition, ASTExprNode* value_if_true, ASTExprNode* value_if_false, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), condition(condition), value_if_true(value_if_true), value_if_false(value_if_false) {}

ASTInNode::ASTInNode(ASTExprNode* value, ASTExprNode* collection, bool vbv, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), value(value), collection(collection), vbv(vbv) {}

ASTFunctionCallNode::ASTFunctionCallNode(std::string identifier, std::string nmspace, std::vector<ASTExprNode*> access_vector,
	std::vector<std::pair<bool, ASTExprNode*>> parameters, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier(std::move(identifier)), nmspace(nmspace), parameters(std::move(parameters)) {}

ASTTypeParseNode::ASTTypeParseNode(Type type, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), type(type), expr(expr) {}


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

void ASTInNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTInNode::hash(visitor::Visitor* v) { return 0; }

void ASTFunctionCallNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTFunctionCallNode::hash(visitor::Visitor* v) { return 0; }

void ASTIdentifierNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTTypingNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTypingNode::hash(visitor::Visitor* v) { return 0; }

unsigned int ASTIdentifierNode::hash(visitor::Visitor* v) {
	return v->hash(this);
}

void ASTUnaryExprNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTUnaryExprNode::hash(visitor::Visitor* v) { return 0; }

void ASTTernaryNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

unsigned int ASTTernaryNode::hash(visitor::Visitor* v) { return 0; }

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

void ASTAsNamespaceNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTAssignmentNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReturnNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTExitNode::accept(visitor::Visitor* v) {
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

void ASTEnumNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTTryCatchNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTThrowNode::accept(visitor::Visitor* v) {
	v->visit(this);
}

void ASTReticencesNode::accept(visitor::Visitor* v) {
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

void ASTDoWhileNode::accept(visitor::Visitor* v) {
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
