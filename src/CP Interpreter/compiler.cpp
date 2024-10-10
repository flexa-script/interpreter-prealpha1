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
	: Visitor(programs, main_program, default_namespace) {
	auto builtin = std::unique_ptr<modules::Builtin>(new modules::Builtin());
	//builtin->register_functions(this);
};

void Compiler::start() {
	visit(current_program);
}

void Compiler::visit(ASTProgramNode* astnode) {
	auto pop = push_namespace(default_namespace);
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}
	}
	add_instruction(OpCode::OP_HALT, byteopnd_n);
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
		program_nmspaces[get_namespace()].push_back(default_namespace);
		start();
		current_program = prev_program;
		pop_namespace(pop);
	}
}

void Compiler::visit(ASTNamespaceManagerNode* astnode) {
	const auto& nmspace = get_namespace();

	if (astnode->image == "include") {
		program_nmspaces[nmspace].push_back(astnode->nmspace);
	}
	else {
		size_t pos = std::distance(program_nmspaces[nmspace].begin(),
			std::find(program_nmspaces[nmspace].begin(),
				program_nmspaces[nmspace].end(), astnode->nmspace));
		program_nmspaces[nmspace].erase(program_nmspaces[nmspace].begin() + pos);
	}
}

void Compiler::visit(ASTEnumNode* astnode) {
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(i));
		add_instruction(OpCode::OP_SET_TYPE, byteopnd8(Type::T_INT));
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(build_namespace(astnode->identifiers[i])));
	}
}

void Compiler::visit(ASTDeclarationNode* astnode) {
	auto pop = push_namespace(astnode->type_name_space);

	type_definition_operations(*astnode);

	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(build_namespace(astnode->identifier)));

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

	if (has_sub_value(astnode->identifier_vector)) {
		access_sub_value_operations(astnode->identifier_vector);
		add_instruction(OpCode::OP_ASSIGN_SUB, byteopnd_n);
	}
	else {
		nmspace_array_operations();
		add_instruction(OpCode::OP_ASSIGN_VAR, byteopnd_s(build_namespace(astnode->identifier)));
	}

	pop_namespace(pop);
}

void Compiler::visit(ASTReturnNode* astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_RETURN, byteopnd_n);
}

void Compiler::visit(ASTFunctionCallNode* astnode) {
	auto pop = push_namespace(astnode->nmspace);

	for (const auto& param : astnode->parameters) {
		param->accept(this);
	}

	nmspace_array_operations();

	add_instruction(OpCode::OP_CALL, byteopnd_s(astnode->identifier));

	access_sub_value_operations(astnode->identifier_vector);

	pop_namespace(pop);
}

void Compiler::visit(ASTFunctionDefinitionNode* astnode) {
	auto pop = push_namespace(astnode->type_name_space);

	if (astnode->block) {
		add_instruction(OpCode::OP_FUN_START, byteopnd_s(build_namespace(astnode->identifier)));

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
			nmspace_array_operations();
			add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(build_namespace(decl->identifier)));
		}
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl)) {
		nmspace_array_operations();
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(build_namespace(idnode->identifier)));
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

void Compiler::visit(ASTStructDefinitionNode* astnode) {
	add_instruction(OpCode::OP_STRUCT_START, byteopnd_s(astnode->identifier));

	for (const auto& var : astnode->variables) {
		type_definition_operations(var.second);
		add_instruction(OpCode::OP_STRUCT_SET_VAR, byteopnd_s(var.first));
	}
	
	add_instruction(OpCode::OP_STRUCT_END, byteopnd_s(astnode->identifier));
}

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
	add_instruction(OpCode::OP_PUSH_STRING, byteopnd_s(astnode->val));
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
	if (has_sub_value(astnode->identifier_vector)) {
		access_sub_value_operations(astnode->identifier_vector);
	}
	else {
		nmspace_array_operations();
		add_instruction(OpCode::OP_LOAD_VAR, byteopnd_s(astnode->identifier));
	}
}

void Compiler::visit(ASTBinaryExprNode* astnode) {
	astnode->left->accept(this);
	astnode->right->accept(this);

	auto op = OpCode::OP_RES;

	if (astnode->op == "or") {
		op = OpCode::OP_OR;
	}
	else if (astnode->op == "and") {
		op = OpCode::OP_AND;
	}
	else if (astnode->op == "|") {
		op = OpCode::OP_BIT_OR;
	}
	else if (astnode->op == "^") {
		op = OpCode::OP_BIT_XOR;
	}
	else if (astnode->op == "&") {
		op = OpCode::OP_BIT_AND;
	}
	else if (astnode->op == "==") {
		op = OpCode::OP_EQL;
	}
	else if (astnode->op == "!=") {
		op = OpCode::OP_DIF;
	}
	else if (astnode->op == "<") {
		op = OpCode::OP_LT;
	}
	else if (astnode->op == "<=") {
		op = OpCode::OP_LTE;
	}
	else if (astnode->op == ">") {
		op = OpCode::OP_GT;
	}
	else if (astnode->op == ">=") {
		op = OpCode::OP_GTE;
	}
	else if (astnode->op == "<=>") {
		op = OpCode::OP_SPACE_SHIP;
	}
	else if (astnode->op == "<<") {
		op = OpCode::OP_LEFT_SHIFT;
	}
	else if (astnode->op == ">>") {
		op = OpCode::OP_RIGHT_SHIFT;
	}
	else if (astnode->op == "+") {
		op = OpCode::OP_ADD;
	}
	else if (astnode->op == "-") {
		op = OpCode::OP_SUB;
	}
	else if (astnode->op == "*") {
		op = OpCode::OP_MUL;
	}
	else if (astnode->op == "/") {
		op = OpCode::OP_DIV;
	}
	else if (astnode->op == "%") {
		op = OpCode::OP_REMAINDER;
	}
	else if (astnode->op == "/%") {
		op = OpCode::OP_FLOOR_DIV;
	}
	else if (astnode->op == "**") {
		op = OpCode::OP_EXP;
	}

	add_instruction(op, byteopnd_n);
}

void Compiler::visit(ASTUnaryExprNode* astnode) {
	auto op = OpCode::OP_RES;

	if (astnode->unary_op == "ref") {
		op = OpCode::OP_REF;
	}
	else if (astnode->unary_op == "unref") {
		op = OpCode::OP_UNREF;
	}
	else if (astnode->unary_op == "-") {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(0));
		op = OpCode::OP_SUB;
	}
	else if (astnode->unary_op == "++") {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(1));
		op = OpCode::OP_ADD;
	}
	else if (astnode->unary_op == "--") {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(1));
		op = OpCode::OP_SUB;
	}
	else if (astnode->unary_op == "not") {
		op = OpCode::OP_NOT;
	}
	else if (astnode->unary_op == "~") {
		op = OpCode::OP_BIT_NOT;
	}

	astnode->expr->accept(this);

	add_instruction(op, byteopnd_n);
}

void Compiler::visit(ASTTernaryNode* astnode) {
	astnode->condition->accept(this);
	astnode->value_if_true->accept(this);
	astnode->value_if_false->accept(this);
	add_instruction(OpCode::OP_TERNARY, byteopnd_n);
}

void Compiler::visit(ASTInNode* astnode) {
	// TODO: binary expression?
	astnode->value->accept(this);
	astnode->collection->accept(this);
	add_instruction(OpCode::OP_IN, byteopnd_n);
}

void Compiler::visit(ASTTypeParseNode* astnode) {
	astnode->expr->accept(this);
	if (is_bool(astnode->type)) {
		add_instruction(OpCode::OP_PUSH_BOOL, byteopnd8(false));
	}
	else if (is_int(astnode->type)) {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(0));
	}
	else if (is_float(astnode->type)) {
		add_instruction(OpCode::OP_PUSH_FLOAT, byteopnd8(0));
	}
	else if (is_char(astnode->type)) {
		add_instruction(OpCode::OP_PUSH_CHAR, byteopnd8('\0'));
	}
	else if (is_string(astnode->type)) {
		add_instruction(OpCode::OP_PUSH_STRING, byteopnd_s(""));
	}
	add_instruction(OpCode::OP_TYPE_PARSE, byteopnd8(astnode->type));
}

void Compiler::visit(ASTNullNode* astnode) {
	add_instruction(OpCode::OP_PUSH_VOID, byteopnd_n);
}

void Compiler::visit(ASTThisNode* astnode) {
	add_instruction(OpCode::OP_PUSH_STRING, byteopnd_s(get_namespace()));
}

void Compiler::visit(ASTTypingNode* astnode) {
	astnode->expr->accept(this);

	if (astnode->image == "typeid") {
		add_instruction(OpCode::OP_TYPEID, byteopnd_n);
	}
	else if (astnode->image == "refid") {
		add_instruction(OpCode::OP_REFID, byteopnd_n);
		}
	else if (astnode->image == "typeof") {
		add_instruction(OpCode::OP_TYPEOF, byteopnd_n);
	}
	else {
		auto type = Type::T_UNDEFINED;
		if (astnode->image == "is_any") {
			type = Type::T_ANY;
		}
		else if (astnode->image == "is_array") {
			type = Type::T_ARRAY;
		}
		else if (astnode->image == "is_struct") {
			type = Type::T_STRUCT;
		}
		add_instruction(OpCode::OP_IS_TYPE, byteopnd8(type));
	}
}

bool Compiler::has_sub_value(std::vector<Identifier> identifier_vector) {
	return identifier_vector.size() > 1 || identifier_vector[0].access_vector.size() > 0;
}

void Compiler::nmspace_array_operations() {
	const auto& nmspace = get_namespace();
	auto size = program_nmspaces[nmspace].size();

	add_instruction(OpCode::OP_CREATE_ARRAY, byteopnd(size));
	for (size_t i = 0; i < size; ++i) {
		add_instruction(OpCode::OP_PUSH_STRING, byteopnd_s(program_nmspaces[nmspace][i]));
		add_instruction(OpCode::OP_SET_ELEMENT, byteopnd(i));
	}
}

void Compiler::type_definition_operations(TypeDefinition type) {

	add_instruction(OpCode::OP_SET_TYPE, byteopnd8(type.type));

	if (!type.type_name.empty()) {
		if (!type.type_name_space.empty()) {
			add_instruction(OpCode::OP_SET_NAME_SPACE, byteopnd_s(type.type_name_space));
		}
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

void Compiler::access_sub_value_operations(std::vector<Identifier> identifier_vector) {
	if (has_sub_value(identifier_vector)) {
		nmspace_array_operations();
		add_instruction(OpCode::OP_LOAD_VAR, byteopnd_s(identifier_vector[0].identifier));

		for (size_t i = 0; i < identifier_vector.size(); ++i) {
			const auto& id = identifier_vector[i];

			if (i > 0) {
				add_instruction(OpCode::OP_LOAD_SUB_ID, byteopnd_s(id.identifier));
			}

			for (auto av : id.access_vector) {
				av->accept(this);
				add_instruction(OpCode::OP_LOAD_SUB_IX, byteopnd_n);
			}
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

std::string Compiler::build_namespace(const std::string& identifier) const {
	return current_namespace.top() + ":" + identifier;
}

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
