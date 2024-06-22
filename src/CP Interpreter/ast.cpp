#include <utility>
#include <numeric>

#include "ast.hpp"
#include "vendor/util.hpp"


using namespace visitor;
using namespace parser;


TypeDefinition::TypeDefinition(Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: type(type), array_type(array_type), dim(dim), type_name(type_name), type_name_space(type_name_space) {}

TypeDefinition::TypeDefinition()
	: type(Type::T_UNDEFINED), array_type(Type::T_UNDEFINED), dim(std::vector<ASTExprNode*>()), type_name(""), type_name_space("") {}

TypeDefinition TypeDefinition::get_basic(Type type) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "");
}

TypeDefinition TypeDefinition::get_array(Type type, Type array_type, std::vector<ASTExprNode*>&& dim) {
	return TypeDefinition(type, array_type, std::move(dim), "", "");
}

TypeDefinition TypeDefinition::get_struct(Type type, const std::string& type_name, const std::string& type_name_space) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), type_name, type_name_space);
}

SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*>&& dim,
	const std::string& type_name, const std::string& type_name_space, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space),
	hash(hash), is_const(is_const) {}

SemanticValue::SemanticValue(Type type, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	hash(0), is_const(false) {}

SemanticValue::SemanticValue()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	hash(0), is_const(false) {}

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

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), value(value), is_const(is_const) {}

SemanticVariable::SemanticVariable()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(""), value(nullptr), is_const(false) {}

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

VariableDefinition::VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {}

VariableDefinition::VariableDefinition()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(""), default_value(nullptr), is_rest(false) {}

VariableDefinition VariableDefinition::get_basic(const std::string& identifier, Type type,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_array(const std::string& identifier, parser::Type type, parser::Type array_type,
	std::vector<ASTExprNode*>&& dim, ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", array_type, std::move(dim), default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_struct(const std::string& identifier, parser::Type type,
	const std::string& type_name, const std::string& type_name_space,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, type_name, type_name_space, Type::T_UNDEFINED,
		std::vector<ASTExprNode*>(), default_value, is_rest, row, col);
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), parameters(parameters), signature(signature) {}

FunctionDefinition::FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(identifier), signature(std::vector<TypeDefinition>()), parameters(std::vector<VariableDefinition>()) {}

FunctionDefinition::FunctionDefinition()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""), CodePosition(0, 0),
	identifier(""), signature(std::vector<TypeDefinition>()), parameters(std::vector<VariableDefinition>()) {}

void FunctionDefinition::check_signature() const {
	bool has_default = false;
	for (size_t i = 0; i < parameters.size(); ++i) {
		if (parameters[i].is_rest && parameters.size() - 1 != i) {
			throw std::runtime_error("'" + identifier + "': the rest parameter must be the last parameter");
		}
		if (parameters[i].default_value) {
			has_default = true;
		}
		if (!parameters[i].default_value && has_default) {
			throw std::runtime_error("'" + identifier + "': the rest parameter must be the last parameter");
		}
	}
}

StructureDefinition::StructureDefinition(const std::string& identifier, const std::vector<VariableDefinition>& variables,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), identifier(identifier), variables(variables) {}

StructureDefinition::StructureDefinition()
	: CodePosition(row, col), identifier(""), variables(std::vector<VariableDefinition>()) {}

Identifier::Identifier(const std::string& identifier, std::vector<ASTExprNode*>&& access_vector)
	: identifier(identifier), access_vector(std::move(access_vector)) {}

Identifier::Identifier(const std::string& identifier)
	: identifier(identifier) {}

Identifier::Identifier()
	: identifier(""), access_vector(std::vector<ASTExprNode*>()) {}

ASTProgramNode::ASTProgramNode(const std::string& name, const std::string& alias, std::vector<ASTNode*>&& statements)
	: ASTNode(row, col), name(name), alias(alias), statements(std::move(statements)), libs(std::vector<std::string>()) {}

ASTUsingNode::ASTUsingNode(const std::vector<std::string>& library, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), library(library) {}

ASTAsNamespaceNode::ASTAsNamespaceNode(const std::string& nmspace, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), nmspace(nmspace) {}

ASTDeclarationNode::ASTDeclarationNode(const std::string& identifier, Type type, Type array_type, std::vector<ASTExprNode*>&& dim,
	const std::string& type_name, const std::string& type_name_space, ASTExprNode* expr, bool is_const, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space),
	identifier(identifier), expr(expr), is_const(is_const) {}

ASTAssignmentNode::ASTAssignmentNode(std::vector<Identifier>&& identifier_vector, const std::string& nmspace,
	const std::string& op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col),
	identifier_vector(std::move(identifier_vector)), nmspace(nmspace), expr(expr), op(op) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), expr(expr) {}

ASTBlockNode::ASTBlockNode(std::vector<ASTNode*>&& statements, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), statements(std::move(statements)) {}

ASTContinueNode::ASTContinueNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTBreakNode::ASTBreakNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTExitNode::ASTExitNode(ASTExprNode* exit_code, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), exit_code(exit_code) {}

ASTSwitchNode::ASTSwitchNode(ASTExprNode* condition, std::vector<ASTNode*>&& statements,
	std::map<ASTExprNode*, unsigned int>&& case_blocks,
	unsigned int default_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), statements(std::move(statements)), case_blocks(std::move(case_blocks)),
	default_block(default_block), parsed_case_blocks(std::map<unsigned int, unsigned int>()) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, std::vector<ASTElseIfNode*>&& else_ifs,
	ASTBlockNode* else_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), if_block(if_block), else_ifs(std::move(else_ifs)), else_block(else_block) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTEnumNode::ASTEnumNode(std::vector<std::string>&& identifiers, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifiers(std::move(identifiers)) {}

ASTTryCatchNode::ASTTryCatchNode(ASTStatementNode* decl, ASTBlockNode* try_block, ASTBlockNode* catch_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), decl(decl), try_block(try_block), catch_block(catch_block) {}

ASTThrowNode::ASTThrowNode(ASTExprNode* error, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), error(error) {}

ASTReticencesNode::ASTReticencesNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTForNode::ASTForNode(std::array<ASTNode*, 3>&& dci, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), dci(std::move(dci)), block(block) {}

ASTForEachNode::ASTForEachNode(ASTDeclarationNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), itdecl(itdecl), collection(collection), block(block) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTDoWhileNode::ASTDoWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTWhileNode(condition, block, row, col) {}

ASTStructDefinitionNode::ASTStructDefinitionNode(const std::string& identifier, std::vector<VariableDefinition>&& variables,
	unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifier(identifier), variables(std::move(variables)) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(const std::string& identifier, std::vector<VariableDefinition>&& parameters,
	Type type, const std::string& type_name, const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
	ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space),
	identifier(identifier), parameters(parameters), block(block) {
	// generate signature
	this->signature = std::vector<TypeDefinition>();
	
	for (const auto& param : this->parameters) {
		variable_names.push_back(param.identifier);
		auto dim_copy = param.dim;
		auto td = TypeDefinition(param.type, param.array_type, std::move(dim_copy), param.type_name, param.type_name_space);
		signature.push_back(td);
	}
}

ASTArrayConstructorNode::ASTArrayConstructorNode(const std::vector<ASTExprNode*>& values, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), values(values) {}

ASTStructConstructorNode::ASTStructConstructorNode(const std::string& type_name, const std::string& nmspace,
	const std::map<std::string, ASTExprNode*>& values, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), type_name(type_name), nmspace(nmspace), values(values) {}

ASTNullNode::ASTNullNode(unsigned int row, unsigned int col)
	: ASTExprNode(row, col) {}

ASTThisNode::ASTThisNode(unsigned int row, unsigned int col)
	: ASTExprNode(row, col) {}

ASTBinaryExprNode::ASTBinaryExprNode(const std::string& op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), op(op), left(left), right(right) {}

ASTUnaryExprNode::ASTUnaryExprNode(const std::string& unary_op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), unary_op(unary_op), expr(expr) {}

ASTIdentifierNode::ASTIdentifierNode(std::vector<Identifier>&& identifier_vector, std::string nmspace, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier_vector(std::move(identifier_vector)), nmspace(nmspace) {}

ASTTernaryNode::ASTTernaryNode(ASTExprNode* condition, ASTExprNode* value_if_true, ASTExprNode* value_if_false, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), condition(condition), value_if_true(value_if_true), value_if_false(value_if_false) {}

ASTInNode::ASTInNode(ASTExprNode* value, ASTExprNode* collection, bool vbv, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), value(value), collection(collection), vbv(vbv) {}

ASTFunctionCallNode::ASTFunctionCallNode(const std::string& identifier, const std::string& nmspace, std::vector<ASTExprNode*>&& access_vector,
	std::vector<std::pair<bool, ASTExprNode*>>&& parameters, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier(identifier), nmspace(nmspace), access_vector(std::move(access_vector)), parameters(std::move(parameters)) {}

ASTTypeParseNode::ASTTypeParseNode(Type type, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), type(type), expr(expr) {}

ASTTypingNode::ASTTypingNode(const std::string& image, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), image(image), expr(expr) {}


void ASTLiteralNode<cp_bool>::accept(Visitor* v) {
	v->visit(this);
}

long long ASTLiteralNode<cp_bool>::hash(Visitor* v) {
	return v->hash(this);
}

void ASTLiteralNode<cp_int>::accept(Visitor* v) {
	v->visit(this);
}

long long ASTLiteralNode<cp_int>::hash(Visitor* v) {
	return v->hash(this);
}

void ASTLiteralNode<cp_float>::accept(Visitor* v) {
	v->visit(this);
}

long long ASTLiteralNode<cp_float>::hash(Visitor* v) {
	return v->hash(this);
}

void ASTLiteralNode<cp_char>::accept(Visitor* v) {
	v->visit(this);
}

long long ASTLiteralNode<cp_char>::hash(Visitor* v) {
	return v->hash(this);
}

void ASTLiteralNode<cp_string>::accept(Visitor* v) {
	v->visit(this);
}

long long ASTLiteralNode<cp_string>::hash(Visitor* v) {
	return v->hash(this);
}

void ASTArrayConstructorNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTArrayConstructorNode::hash(Visitor* v) { return 0; }

void ASTStructConstructorNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTStructConstructorNode::hash(Visitor* v) { return 0; }

void ASTBinaryExprNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTBinaryExprNode::hash(Visitor* v) { return 0; }

void ASTInNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTInNode::hash(Visitor* v) { return 0; }

void ASTFunctionCallNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTFunctionCallNode::hash(Visitor* v) { return 0; }

void ASTIdentifierNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTTypingNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTTypingNode::hash(Visitor* v) { return 0; }

long long ASTIdentifierNode::hash(Visitor* v) {
	return v->hash(this);
}

void ASTUnaryExprNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTUnaryExprNode::hash(Visitor* v) { return 0; }

void ASTTernaryNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTTernaryNode::hash(Visitor* v) { return 0; }

void ASTTypeParseNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTTypeParseNode::hash(Visitor* v) { return 0; }

void ASTNullNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTNullNode::hash(Visitor* v) { return 0; }

void ASTThisNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTThisNode::hash(Visitor* v) { return 0; }

void ASTDeclarationNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTAsNamespaceNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTAssignmentNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTReturnNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTExitNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTBlockNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTContinueNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTBreakNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTSwitchNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTEnumNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTTryCatchNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTThrowNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTReticencesNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTElseIfNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTIfNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTForNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTForEachNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTWhileNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTDoWhileNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTFunctionDefinitionNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTStructDefinitionNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTUsingNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTProgramNode::accept(Visitor* v) {
	v->visit(this);
}
