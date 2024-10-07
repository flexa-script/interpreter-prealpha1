#include <iostream>
#include <utility>

#include "compiler.hpp"
#include "exception_handler.hpp"
#include "graphics.hpp"
#include "token.hpp"
#include "builtin.hpp"

#include "vendor/axeutils.hpp"
#include "vendor/axeuuid.hpp"

using namespace visitor;
using namespace parser;
using namespace lexer;

Compiler::Compiler(ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	auto builtin = std::unique_ptr<modules::Builtin>(new modules::Builtin());
	//builtin->register_functions(this);
};

void Compiler::start() {
	visit(current_program);
}

void Compiler::visit(ASTProgramNode* astnode) {
	auto pop = push_namespace(astnode->alias.empty() ? default_namespace : astnode->alias);
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}
	}
	pop_namespace(pop);
}

void Compiler::visit(ASTUsingNode* astnode) {
	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (built_in_libs.find(libname) != built_in_libs.end()) {
		//built_in_libs.find(libname)->second->register_functions(this);
	}

	auto program = programs[libname];

	// add lib to current program
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::StringUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		auto pop = push_namespace(program->alias);
		start();
		current_program = prev_program;
		pop_namespace(pop);
	}
}

void Compiler::visit(ASTNamespaceManagerNode* astnode) {}

void Compiler::visit(ASTEnumNode* astnode) {
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(i));
		add_instruction(OpCode::OP_SET_TYPE, byteopnd8(Type::T_INT));
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifiers[i]));
	}
}

void Compiler::visit(ASTDeclarationNode* astnode) {
	auto pop = push_namespace(astnode->type_name_space);

	type_definition_operations(*astnode);

	if (astnode->expr) {
		astnode->expr->accept(this);

		// todo parse {1} array build
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifier));

	pop_namespace(pop);
}

void Compiler::visit(ASTUnpackedDeclarationNode* astnode) {
	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void Compiler::visit(ASTAssignmentNode* astnode) {
	auto pop = push_namespace(astnode->nmspace);

	astnode->expr->accept(this);
	add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifier));

	pop_namespace(pop);
}

void Compiler::visit(ASTReturnNode* astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		//add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_RETURN, byteopnd_n);
}

void Compiler::visit(ASTFunctionCallNode* astnode) {
	auto pop = push_namespace(astnode->nmspace);

	for (const auto& param : astnode->parameters) {
		param->accept(this);
	}

	add_instruction(OpCode::OP_CALL, byteopnd_s(astnode->identifier));

	pop_namespace(pop);
}

void Compiler::visit(ASTFunctionDefinitionNode* astnode) {
	auto pop = push_namespace(astnode->type_name_space);

	if (astnode->block) {
		add_instruction(OpCode::OP_FUN_START, byteopnd_s(astnode->identifier));

		type_definition_operations(*astnode);

		for (auto& param : astnode->parameters) {
			if (param.default_value) {
				auto param_dcl = std::make_unique<ASTDeclarationNode>(param.identifier, param.type, param.array_type,
					param.dim, param.type_name, param.type_name_space, param.default_value, false,
					astnode->row, astnode->col);
				param_dcl->accept(this);
			}
		}

		add_instruction(OpCode::OP_FUN_PARAM_END, byteopnd_n);

		astnode->block->accept(this);

		add_instruction(OpCode::OP_FUN_END, byteopnd_n);
	}

	pop_namespace(pop);
}

void Compiler::visit(ASTFunctionExpression* astnode) {
	astnode->fun->identifier = axe::UUID::generate();
	astnode->fun->accept(this);
	add_instruction(OpCode::OP_PUSH_FUNCTION, byteopnd_s(astnode->fun->identifier));
}

void Compiler::visit(ASTBlockNode* astnode) {
	for (const auto& stmt : astnode->statements) {
		stmt->accept(this);
	}
}

void Compiler::visit(ASTExitNode* astnode) {
	astnode->exit_code->accept(this);
	add_instruction(OpCode::OP_HALT, byteopnd_n);
}

void Compiler::visit(ASTContinueNode* astnode) {
	add_instruction(OpCode::OP_CONTINUE, byteopnd_n);
}

void Compiler::visit(ASTBreakNode* astnode) {
	add_instruction(OpCode::OP_BREAK, byteopnd_n);
}

void Compiler::visit(ASTSwitchNode* astnode) {
	astnode->condition->accept(this);

	for (size_t i = 0; i < astnode->statements.size(); ++i) {
		for (const auto& [key, value] : astnode->parsed_case_blocks) {
			if (i == value || i == astnode->default_block) {
				add_instruction(OpCode::OP_JUMP_IF_FALSE_OR_NEXT, nullptr);
			}
		}
		add_instruction(OpCode::OP_POP, byteopnd_n);
		astnode->statements[i]->accept(this);
		replace_last_operand(byteopnd8(pointer));
	}
}

void Compiler::visit(ASTElseIfNode* astnode) {
	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

	astnode->block->accept(this);

	replace_last_operand(byteopnd8(pointer));
}

void Compiler::visit(ASTIfNode* astnode) {
	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

	astnode->if_block->accept(this);

	replace_last_operand(byteopnd8(pointer));

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

		astnode->else_block->accept(this);

		replace_last_operand(byteopnd8(pointer));
	}
}

void Compiler::visit(ASTForNode* astnode) {
	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	auto start = pointer;
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);
	}
	astnode->block->accept(this);
	if (astnode->dci[2]) {
		astnode->dci[2]->accept(this);
	}
	add_instruction(OpCode::OP_JUMP_IF_TRUE, byteopnd8(start));
}

void Compiler::visit(ASTForEachNode* astnode) {
	astnode->collection->accept(this);

	add_instruction(OpCode::OP_GET_ITERATOR, byteopnd_n);

	astnode->itdecl->accept(this);

	add_instruction(OpCode::OP_NEXT_ELEMENT, byteopnd_n);

	auto start = pointer;

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->itdecl)) {
		for (auto decl : idnode->declarations) {
			add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(decl->identifier));
		}
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl)) {
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(idnode->identifier));
	}

	astnode->block->accept(this);

	add_instruction(OpCode::OP_NEXT_ELEMENT, byteopnd_n);

	add_instruction(OpCode::OP_JUMP_IF_TRUE, byteopnd8(start));
}

void Compiler::visit(ASTTryCatchNode* astnode) {
	add_instruction(OpCode::OP_TRY_START, byteopnd_n);

	astnode->try_block->accept(this);

	add_instruction(OpCode::OP_TRY_END, byteopnd_n);

	add_instruction(OpCode::OP_JUMP_IF_TRUE, nullptr);

	astnode->decl->accept(this);

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->decl)) {
		for (auto decl : idnode->declarations) {
			add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(decl->identifier));
		}
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(idnode->identifier));
	}

	astnode->catch_block->accept(this);

	replace_last_operand(byteopnd8(pointer));
}

void Compiler::visit(parser::ASTThrowNode* astnode) {
	astnode->error->accept(this);
	add_instruction(OpCode::OP_THROW, byteopnd_n);
}

void Compiler::visit(ASTReticencesNode* astnode) {}

void Compiler::visit(ASTWhileNode* astnode) {
	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

	astnode->block->accept(this);

	replace_last_operand(byteopnd8(pointer));
}

void Compiler::visit(ASTDoWhileNode* astnode) {
	auto start = pointer;

	astnode->block->accept(this);

	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_TRUE, byteopnd8(pointer));
}

void Compiler::visit(ASTStructDefinitionNode* astnode) {}

void Compiler::visit(ASTLiteralNode<cp_bool>* astnode) {
	add_instruction(OpCode::OP_PUSH_BOOL, byteopnd(astnode->val));
}

void Compiler::visit(ASTLiteralNode<cp_int>* astnode) {
	add_instruction(OpCode::OP_PUSH_INT, byteopnd(astnode->val));
}

void Compiler::visit(ASTLiteralNode<cp_float>* astnode) {
	add_instruction(OpCode::OP_PUSH_FLOAT, byteopnd(&astnode->val));
}

void Compiler::visit(ASTLiteralNode<cp_char>* astnode) {
	add_instruction(OpCode::OP_PUSH_CHAR, byteopnd(astnode->val));
}

void Compiler::visit(ASTLiteralNode<cp_string>* astnode) {
	add_instruction(OpCode::OP_PUSH_CHAR, byteopnd_s(astnode->val));
}

void Compiler::visit(ASTArrayConstructorNode* astnode) {
	auto size = astnode->values.size();

	add_instruction(OpCode::OP_CREATE_ARRAY, byteopnd(size));

	for (size_t i = 0; i < size; ++i) {
		astnode->values[i]->accept(this);
		add_instruction(OpCode::OP_SET_ELEMENT, byteopnd(i));
	}
}

void Compiler::visit(ASTStructConstructorNode* astnode) {
	add_instruction(OpCode::OP_SET_TYPE_NAME, byteopnd_s(astnode->nmspace));
	add_instruction(OpCode::OP_CREATE_STRUCT, byteopnd_s(astnode->type_name));

	for (const auto& expr : astnode->values) {
		expr.second->accept(this);
		add_instruction(OpCode::OP_SET_ELEMENT, byteopnd_s(expr.first));
	}
}

void Compiler::visit(ASTIdentifierNode* astnode) {
	// subvalues??
	add_instruction(OpCode::OP_LOAD_VAR, byteopnd_s(astnode->identifier));
}

void Compiler::visit(ASTBinaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->left->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto lexpr = current_expression;

	astnode->right->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto rexpr = current_expression;

	current_expression = SemanticValue(do_operation(astnode->op, lexpr, lexpr, nullptr, rexpr, true), 0, false, 0, 0);
	current_expression.is_const = lexpr.is_const && rexpr.is_const;
}

void Compiler::visit(ASTUnaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (astnode->unary_op == "ref" || astnode->unary_op == "unref") {
		if (astnode->unary_op == "ref") {
			current_expression.use_ref = true;
		}
		if (astnode->unary_op == "unref") {
			current_expression.use_ref = false;
		}
	}
	else {
		switch (current_expression.type) {
		case Type::T_INT:
			if (astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++"
				&& astnode->unary_op != "~") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of int expression");
			}
			break;
		case Type::T_FLOAT:
			if (astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of float expression");
			}
			break;
		case Type::T_BOOL:
			if (astnode->unary_op != "not") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
			}
			break;
		case Type::T_ANY:
			if (astnode->unary_op != "not" && astnode->unary_op != "~"
				&& astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
			}
			break;
		default:
			throw std::runtime_error("incompatible unary operator '" + astnode->unary_op +
				"' in front of " + type_str(current_expression.type) + " expression");
		}
	}
}

void Compiler::visit(ASTTernaryNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	astnode->value_if_true->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	astnode->value_if_false->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}
}

void Compiler::visit(ASTInNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->value->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto valtype = current_expression.type;

	astnode->collection->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto coltype = current_expression.type;
	auto colarrtype = current_expression.type;

	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;

	if (is_any(valtype)
		&& (is_any(coltype)
			|| is_array(coltype)
			&& is_any(colarrtype))) {
		return;
	}

	if (!match_type(valtype, colarrtype)
		&& !is_any(valtype) && !is_any(coltype) && !is_any(colarrtype)
		&& is_string(valtype) && !is_string(coltype)
		&& is_char(valtype) && !is_string(coltype)) {
		throw std::runtime_error("types don't match '" + type_str(valtype) + "' and '" + type_str(coltype) + "'");
	}

	if (!is_collection(coltype) && !is_any(coltype)) {
		throw std::runtime_error("invalid type '" + type_str(coltype) + "', value must be a array or string");
	}

}

void Compiler::visit(ASTTypeParseNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if ((is_array(current_expression.type) || is_struct(current_expression.type))
		&& !is_string(astnode->type)) {
		throw std::runtime_error("invalid type conversion from "
			+ type_str(current_expression.type) + " to " + type_str(astnode->type));
	}

	current_expression = SemanticValue();
	current_expression.type = astnode->type;
}

void Compiler::visit(ASTNullNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_VOID;
}

void Compiler::visit(ASTThisNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
}

void Compiler::visit(ASTTypingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	current_expression = SemanticValue();
	if (astnode->image == "typeid" || astnode->image == "refid") {
		current_expression.type = Type::T_INT;
	}
	else if (astnode->image == "typeof") {
		current_expression.type = Type::T_STRING;
	}
	else {
		current_expression.type = Type::T_BOOL;
	}
}

void Compiler::type_definition_operations(TypeDefinition type) {
	add_instruction(OpCode::OP_SET_NAME_SPACE, byteopnd_s(get_namespace()));

	add_instruction(OpCode::OP_SET_TYPE, byteopnd8(type.type));

	if (!type.type_name.empty()) {
		add_instruction(OpCode::OP_SET_TYPE_NAME, byteopnd_s(type.type_name));
	}

	auto dim = type.dim.size();

	if (dim > 0) {
		add_instruction(OpCode::OP_SET_ARRAY_DIM, byteopnd8(dim));
		for (auto& s : type.dim) {
			s->accept(this);
			add_instruction(OpCode::OP_SET_ARRAY_SIZE, byteopnd_n);
		}
	}
}

void Compiler::add_instruction(OpCode opcode, uint8_t* operand) {
	bytecode_program.push_back(BytecodeInstruction{ opcode, operand });
	++pointer;
}

void Compiler::replace_last_operand(uint8_t* operand) {
	for (size_t i = bytecode_program.size() - 1; i > 0; --i) {
		if (!bytecode_program[i].operand) {
			bytecode_program[i].operand = operand;
			break;
		}
		if (i == 0) break;
	}
}

bool Compiler::push_namespace(const std::string nmspace) {
	if (!nmspace.empty()) {
		current_namespace.push(nmspace);
		return true;
	}
	return false;
}
void Compiler::pop_namespace(bool pop) {
	if (pop) {
		current_namespace.pop();
	}
}

long long Compiler::hash(ASTExprNode* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_bool>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_int>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_float>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_char>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_string>* astnode) { return 0; }

long long Compiler::hash(ASTIdentifierNode* astnode) { return 0; }

const std::string& Compiler::get_namespace(const std::string& nmspace) const {
	return current_namespace.top();
}

const std::string& Compiler::get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace) const {
	return nmspace.empty() ? (
		program->alias.empty() ? default_namespace : program->alias
		) : nmspace;
}

void Compiler::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Compiler::msg_header() {
	return "(CMP) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
