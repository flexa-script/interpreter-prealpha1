#include <utility>
#include <numeric>

#include "ast.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;
using namespace parser;

Identifier::Identifier(const std::string& identifier, const std::vector<void*>& access_vector)
	: identifier(identifier), access_vector(access_vector) {}

Identifier::Identifier(const std::string& identifier)
	: identifier(identifier) {}

Identifier::Identifier()
	: identifier(""), access_vector(std::vector<void*>()) {}

ASTProgramNode::ASTProgramNode(const std::string& name, const std::string& alias, const std::vector<ASTNode*>& statements)
	: ASTNode(row, col), name(name), alias(alias), statements(statements), libs(std::vector<std::string>()) {}

ASTUsingNode::ASTUsingNode(const std::vector<std::string>& library, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), library(library) {}

ASTNamespaceManagerNode::ASTNamespaceManagerNode(const std::string& image, const std::string& nmspace, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), image(image), nmspace(nmspace) {}

ASTDeclarationNode::ASTDeclarationNode(const std::string& identifier, Type type, Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space, ASTExprNode* expr, bool is_const, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), expr(expr), is_const(is_const) {}

ASTUnpackedDeclarationNode::ASTUnpackedDeclarationNode(Type type, Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space, const std::vector<ASTDeclarationNode*>& declarations,
	ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	declarations(declarations), expr(expr) {}

ASTAssignmentNode::ASTAssignmentNode(const std::vector<Identifier>& identifier_vector, const std::string& nmspace,
	const std::string& op, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifier(identifier_vector[0].identifier),
	identifier_vector(identifier_vector), nmspace(nmspace), expr(expr), op(op) {}

ASTReturnNode::ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), expr(expr) {}

ASTBlockNode::ASTBlockNode(const std::vector<ASTNode*>& statements, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), statements(statements) {}

ASTContinueNode::ASTContinueNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTBreakNode::ASTBreakNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTExitNode::ASTExitNode(ASTExprNode* exit_code, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), exit_code(exit_code) {}

ASTSwitchNode::ASTSwitchNode(ASTExprNode* condition, const std::vector<ASTNode*>& statements,
	const std::map<ASTExprNode*, unsigned int>& case_blocks,
	unsigned int default_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), statements(statements), case_blocks(case_blocks),
	default_block(default_block), parsed_case_blocks(std::map<unsigned int, unsigned int>()) {}

ASTIfNode::ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, const std::vector<ASTElseIfNode*>& else_ifs,
	ASTBlockNode* else_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), if_block(if_block), else_ifs(else_ifs), else_block(else_block) {}

ASTElseIfNode::ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTEnumNode::ASTEnumNode(const std::vector<std::string>& identifiers, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifiers(identifiers) {}

ASTTryCatchNode::ASTTryCatchNode(ASTStatementNode* decl, ASTBlockNode* try_block, ASTBlockNode* catch_block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), decl(decl), try_block(try_block), catch_block(catch_block) {}

ASTThrowNode::ASTThrowNode(ASTExprNode* error, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), error(error) {}

ASTReticencesNode::ASTReticencesNode(unsigned int row, unsigned int col)
	: ASTStatementNode(row, col) {}

ASTForNode::ASTForNode(const std::array<ASTNode*, 3>& dci, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), dci(dci), block(block) {}

ASTForEachNode::ASTForEachNode(ASTStatementNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), itdecl(itdecl), collection(collection), block(block) {}

ASTWhileNode::ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), condition(condition), block(block) {}

ASTDoWhileNode::ASTDoWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTWhileNode(condition, block, row, col) {}

ASTStructDefinitionNode::ASTStructDefinitionNode(const std::string& identifier, const std::map<std::string, VariableDefinition>& variables,
	unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), identifier(identifier), variables(variables) {}

ASTFunctionDefinitionNode::ASTFunctionDefinitionNode(const std::string& identifier, const std::vector<TypeDefinition*>& parameters,
	Type type, const std::string& type_name, const std::string& type_name_space, Type array_type, const std::vector<void*>& dim,
	ASTBlockNode* block, unsigned int row, unsigned int col)
	: ASTStatementNode(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), parameters(parameters), block(block) {}

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

ASTIdentifierNode::ASTIdentifierNode(const std::vector<Identifier>& identifier_vector, std::string nmspace, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier(identifier_vector[0].identifier),
	identifier_vector(identifier_vector), nmspace(nmspace) {}

ASTTernaryNode::ASTTernaryNode(ASTExprNode* condition, ASTExprNode* value_if_true, ASTExprNode* value_if_false, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), condition(condition), value_if_true(value_if_true), value_if_false(value_if_false) {}

ASTInNode::ASTInNode(ASTExprNode* value, ASTExprNode* collection, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), value(value), collection(collection) {}

ASTFunctionCallNode::ASTFunctionCallNode(const std::string& nmspace,
	const std::vector<Identifier>& identifier_vector,
	const std::vector<ASTExprNode*>& parameters, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), identifier(identifier_vector[0].identifier),
	nmspace(nmspace), identifier_vector(identifier_vector), parameters(parameters) {}

ASTTypeParseNode::ASTTypeParseNode(Type type, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), type(type), expr(expr) {}

ASTTypingNode::ASTTypingNode(const std::string& image, ASTExprNode* expr, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), image(image), expr(expr) {}

ASTFunctionExpression::ASTFunctionExpression(ASTFunctionDefinitionNode* fun, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), fun(fun) {}

ASTValueNode::ASTValueNode(Value* value, unsigned int row, unsigned int col)
	: ASTExprNode(row, col), value(value) {}

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

void ASTFunctionExpression::accept(Visitor* v) {
	v->visit(this);
}

long long ASTFunctionExpression::hash(Visitor* v) { return 0; }

void ASTValueNode::accept(Visitor* v) {
	v->visit(this);
}

long long ASTValueNode::hash(Visitor* v) {
	return v->hash(this);
}

void ASTDeclarationNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTUnpackedDeclarationNode::accept(Visitor* v) {
	v->visit(this);
}

void ASTNamespaceManagerNode::accept(Visitor* v) {
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
