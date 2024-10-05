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

Compiler::Compiler(std::shared_ptr<CompilerScope> global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	scopes[default_namespace].push_back(global_scope);

	auto builtin = std::unique_ptr<modules::Builtin>(new modules::Builtin());
	//builtin->register_functions(this);
};

void Compiler::start() {
	visit(current_program);
}

void Compiler::visit(ASTProgramNode* astnode) {
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}
	}
}

void Compiler::visit(ASTUsingNode* astnode) {
	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (built_in_libs.find(libname) != built_in_libs.end()) {
		//built_in_libs.find(libname)->second->register_functions(this);
	}

	if (programs.find(libname) == programs.end()) {
		throw std::runtime_error("lib '" + libname + "' not found");
	}

	auto program = programs[libname];

	// add lib to current program
	if (axe::StringUtils::contains(current_program->libs, libname)) {
		throw std::runtime_error("lib '" + libname + "' already declared in " + current_program->name);
	}
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::StringUtils::contains(parsed_libs, libname)) {
		if (!program->alias.empty()) {
			nmspaces.push_back(program->alias);
		}
		parsed_libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;

		if (current_program->alias == default_namespace) {
			throw std::runtime_error("namespace '" + current_program->alias + "' is not valid ");
		}

		program_nmspaces[get_namespace(current_program->alias)].push_back(default_namespace);

		if (!program->alias.empty()) {
			scopes[program->alias].push_back(std::make_shared<CompilerScope>());
		}
		start();
		current_program = prev_program;
	}
}

void Compiler::visit(ASTNamespaceManagerNode* astnode) {}

void Compiler::visit(ASTEnumNode* astnode) {
	const auto& nmspace = get_namespace();
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		auto var_id = scopes[nmspace].back()->declare_variable(astnode->identifiers[i], TypeDefinition(Type::T_INT));

		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_PUSH_INT, byteopnd8(i) });
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_SET_VAR_IDENTIFIER, byteopnd_s(astnode->identifiers[i]) });
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_SET_VAR_TYPE, byteopnd8(Type::T_INT) });
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_STORE_VAR, byteopnd8(var_id) });
	}
}

void Compiler::visit(ASTDeclarationNode* astnode) {
	const auto& nmspace = get_namespace();
	std::shared_ptr<CompilerScope> current_scope = scopes[nmspace].back();

	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_PUSH_UNDEFINED, byteopnd_n });
	}

	auto var_id = current_scope->declare_variable(astnode->identifier, TypeDefinition(astnode->type,
		astnode->array_type, astnode->dim,
		astnode->type_name, astnode->type_name_space));

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_SET_VAR_ARRAY_DIM, byteopnd8(astnode->dim.size()) });
	for (auto& s : astnode->dim) {
		s->accept(this);
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_SET_VAR_ARRAY_SIZE, byteopnd_n });
	}
	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_SET_VAR_TYPE, byteopnd8(astnode->type) });

	// todo parse {1} array build

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_STORE_VAR, byteopnd8(var_id) });
}

void Compiler::visit(ASTUnpackedDeclarationNode* astnode) {
	const auto& nmspace = get_namespace();
	std::shared_ptr<CompilerScope> current_scope = scopes[nmspace].back();

	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void Compiler::visit(ASTAssignmentNode* astnode) {
	const auto& identifier = astnode->identifier;
	const auto& nmspace = get_namespace(astnode->nmspace);
	std::shared_ptr<CompilerScope> curr_scope;
	try {
		curr_scope = get_inner_most_variable_scope(nmspace, identifier);
	}
	catch (...) {}

	auto declared_variable = curr_scope->find_declared_variable(identifier);

	astnode->expr->accept(this);

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_STORE_VAR, byteopnd8(declared_variable.second) });
}

void Compiler::visit(ASTReturnNode* astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		current_expression = TypeDefinition();
		//bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_PUSH_UNDEFINED, byteopnd_n });
	}

	if (!current_function.empty()) {
		const auto& currfun = current_function.top();
		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_RETURN, byteopnd_n });
	}
}

void Compiler::visit(ASTFunctionCallNode* astnode) {
	const auto& nmspace = get_namespace(astnode->nmspace);
	bool strict = true;
	std::vector<TypeDefinition> signature = std::vector<TypeDefinition>();

	for (const auto& param : astnode->parameters) {
		param->accept(this);
	}

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_CALL, byteopnd(astnode) });

}

void Compiler::visit(ASTFunctionDefinitionNode* astnode) {
	if (astnode->block) {
		const auto& nmspace = get_namespace(astnode->type_name_space);

		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_FUN_START, byteopnd_n });

		for (auto& par : astnode->parameters) {

		}

		if (astnode->identifier != "") {
			try {
				auto& declfun = scopes[nmspace].back()->find_declared_function(astnode->identifier, &astnode->signature, evaluate_access_vector_ptr, true);
				declfun.first.block = astnode->block;
			}
			catch (...) {
				scopes[nmspace].back()->declare_function(astnode->identifier, astnode->type, astnode->type_name, astnode->type_name_space,
					astnode->array_type, astnode->dim, astnode->signature, astnode->parameters, astnode->block, astnode->row, astnode->row);
			}

			auto& curr_function = scopes[nmspace].back()->find_declared_function(astnode->identifier, &astnode->signature, evaluate_access_vector_ptr);

			current_function.push(curr_function);
		}

		astnode->block->accept(this);

		if (astnode->identifier != "") {
			current_function.pop();
		}

		bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_FUN_END, byteopnd_n });
	}
}

void Compiler::visit(ASTFunctionExpression* astnode) {
	const auto& nmspace = get_namespace(astnode->type_name_space);

	auto fun_node = dynamic_cast<ASTFunctionDefinitionNode*>(astnode->fun);

	std::string identifier = "__unnamed_function_" + axe::UUID::generate();

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_FUN_START, byteopnd_n });

	scopes[nmspace].back()->declare_function(identifier, fun_node->type, fun_node->type_name, fun_node->type_name_space,
		fun_node->array_type, fun_node->dim, fun_node->signature, fun_node->parameters, fun_node->block, fun_node->row, fun_node->row);

	auto& curr_function = scopes[nmspace].back()->find_declared_function(identifier, &fun_node->signature, evaluate_access_vector_ptr);

	current_function.push(curr_function);

	fun_node->block->accept(this);

	current_function.pop();

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_FUN_END, byteopnd_n });

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_PUSH_FUNCTION, byteopnd8(curr_function.second) });

	current_expression = TypeDefinition();
	current_expression.type = Type::T_FUNCTION;
	current_expression.dim = fun_node->dim;
	current_expression.type_name = fun_node->type_name;
	current_expression.type_name_space = fun_node->type_name_space;
}

void Compiler::visit(ASTBlockNode* astnode) {
	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(std::make_shared<CompilerScope>());

	if (!current_function.empty()) {
		for (const auto& param : current_function.top().first.parameters) {
			if (is_function(param.type) || is_any(param.type)) {
				scopes[nmspace].back()->declare_variable_function(param.identifier, param.row, param.row);
			}

			if (!is_function(param.type)) {
				auto var_expr = std::make_shared<TypeDefinition>();
				var_expr->type = param.type;
				var_expr->array_type = param.array_type;
				var_expr->dim = param.dim;
				var_expr->type_name = param.type_name;
				var_expr->type_name_space = param.type_name_space;

				scopes[nmspace].back()->declare_variable(param.identifier, param.type, param.array_type,
					param.dim, param.type_name, param.type_name_space, param.row, param.col);
			}
		}
	}

	for (const auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	scopes[nmspace].pop_back();
}

void Compiler::visit(ASTExitNode* astnode) {
	astnode->exit_code->accept(this);

	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_HALT, byteopnd_n });
}

void Compiler::visit(ASTContinueNode* astnode) {
	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_CONTINUE, byteopnd_n });
}

void Compiler::visit(ASTBreakNode* astnode) {
	bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_BREAK, byteopnd_n });
}

void Compiler::visit(ASTSwitchNode* astnode) {
	const auto& nmspace = get_namespace();
	scopes[nmspace].push_back(std::make_shared<CompilerScope>());

	for (size_t i = 0; i < astnode->statements.size(); ++i) {
		for (const auto& [key, value] : astnode->parsed_case_blocks) {
			if (i == value) {
				bytecode_program.push_back(BytecodeInstruction{ OpCode::OP_PUSH_FUNCTION, byteopnd8() });
			}
		}
	}

	for (const auto& expr : astnode->case_blocks) {
		expr.first->accept(this);

	}

	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	astnode->condition->accept(this);

	scopes[nmspace].pop_back();
}

void Compiler::visit(ASTElseIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
}

void Compiler::visit(ASTIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->if_block->accept(this);

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		astnode->else_block->accept(this);
	}
}

void Compiler::visit(ASTForNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(std::make_shared<CompilerScope>());

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		if (!is_bool(current_expression.type)
			&& !is_any(current_expression.type)) {
			ExceptionHandler::throw_condition_type_err();
		}
	}
	if (astnode->dci[2]) {
		astnode->dci[2]->accept(this);
	}
	astnode->block->accept(this);

	scopes[nmspace].pop_back();
	is_loop = false;
}

void Compiler::visit(ASTForEachNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	TypeDefinition col_type;
	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(std::make_shared<CompilerScope>());
	std::shared_ptr<CompilerScope> back_scope = scopes[nmspace].back();

	astnode->itdecl->accept(this);

	astnode->collection->accept(this);
	col_type = current_expression;

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->itdecl)) {
		if (!is_struct(col_type.type) && !is_any(col_type.type)) {
			throw std::runtime_error("[key, value] can only be used with struct");
		}

		if (idnode->declarations.size() != 2) {
			throw std::runtime_error("invalid number of values");
		}

		const auto& decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		auto& decl_key_ptr = decl_key.first;
		decl_key_ptr->value = std::make_shared<SemanticValue>(Type::T_STRING, astnode->row, astnode->col);

		back_scope = scopes[nmspace].back();
		const auto& decl_val = back_scope->find_declared_variable(idnode->declarations[1]->identifier);
		auto& decl_val_ptr = decl_val.first;
		decl_val_ptr->value = std::make_shared<SemanticValue>(Type::T_ANY, astnode->row, astnode->col);
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl)) {
		if (!is_array(col_type.type)
			&& !is_string(col_type.type)
			&& !is_struct(col_type.type)
			&& !is_any(col_type.type)) {
			throw std::runtime_error("expected iterable in foreach");
		}

		if (is_struct(col_type.type)) {
			try {
				auto s = get_inner_most_struct_definition_scope("cp", "Pair");
			}
			catch (...) {
				throw std::runtime_error("struct 'cp::Pair' not found");
			}
			col_type = TypeDefinition::get_struct("Pair", "cp");
		}

		const auto& declared_variable = back_scope->find_declared_variable(idnode->identifier);
		auto& declared_variable_ptr = declared_variable.first;

		if (!match_type(declared_variable_ptr->type, col_type.type)
			&& !match_type(declared_variable_ptr->type, col_type.array_type)
			&& is_char(declared_variable_ptr->type) && !is_string(col_type.type)
			&& !is_any(declared_variable_ptr->type)
			&& !is_any(col_type.type)
			&& !is_any(col_type.array_type)) {
			throw std::runtime_error("mismatched types");
		}

		if (is_struct(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_STRUCT;
			declared_variable_ptr->value->type_name = "Pair";
			declared_variable_ptr->value->type_name_space = "cp";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (is_string(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_CHAR;
			declared_variable_ptr->value->type_name = "";
			declared_variable_ptr->value->type_name_space = "";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (is_any(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_ANY;
			declared_variable_ptr->value->type_name = "";
			declared_variable_ptr->value->type_name_space = "";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (col_type.dim.size() > 1) {
			if (!is_any(declared_variable_ptr->type)) {
				declared_variable_ptr->value->type = declared_variable_ptr->type;
				declared_variable_ptr->value->array_type = declared_variable_ptr->array_type;
				declared_variable_ptr->value->type_name = declared_variable_ptr->type_name;
				declared_variable_ptr->value->type_name_space = declared_variable_ptr->type_name_space;
			}
			else {
				declared_variable_ptr->value->type = col_type.type;
				declared_variable_ptr->value->array_type = current_expression.array_type;
				if (!current_expression.type_name.empty()) {
					declared_variable_ptr->value->type_name = current_expression.type_name;
					declared_variable_ptr->value->type_name_space = current_expression.type_name_space;
				}
			}
		}
		else {
			declared_variable_ptr->value->type = col_type.array_type;
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
			if (!current_expression.type_name.empty()) {
				declared_variable_ptr->value->type_name = current_expression.type_name;
				declared_variable_ptr->value->type_name_space = current_expression.type_name_space;
			}
		}

	}
	else {
		throw std::runtime_error("expected declaration");
	}

	astnode->block->accept(this);

	scopes[nmspace].pop_back();
	is_loop = false;
}

void Compiler::visit(ASTTryCatchNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	astnode->try_block->accept(this);

	scopes[nmspace].push_back(std::make_shared<CompilerScope>());

	astnode->decl->accept(this);

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->decl)) {
		if (idnode->declarations.size() != 1) {
			throw std::runtime_error("invalid number of values");
		}

		std::shared_ptr<CompilerScope> back_scope = scopes[nmspace].back();
		const auto& decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		auto& decl_key_ptr = decl_key.first;
		decl_key_ptr->value = std::make_shared<SemanticValue>(Type::T_STRING, astnode->row, astnode->col);

	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}

		auto& declared_variable = current_expression.ref;
		declared_variable->value->type = Type::T_STRUCT;
		declared_variable->value->type_name = "Exception";
		declared_variable->value->type_name_space = "cp";
	}
	else if (!dynamic_cast<ASTReticencesNode*>(astnode->decl)) {
		throw std::runtime_error("expected declaration");
	}

	astnode->catch_block->accept(this);

	scopes[nmspace].pop_back();
}

void Compiler::visit(parser::ASTThrowNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->error->accept(this);

	if (is_struct(current_expression.type)
		&& current_expression.type_name == "Exception") {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}
	}
	else if (!is_string(current_expression.type)) {
		throw std::runtime_error("expected Exception or string in throw");
	}
}

void Compiler::visit(ASTReticencesNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
}

void Compiler::visit(ASTWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void Compiler::visit(ASTDoWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void Compiler::visit(ASTStructDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	try {
		std::shared_ptr<CompilerScope> curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->identifier);
		throw std::runtime_error("struct '" + astnode->identifier +
			"' already defined");
	}
	catch (...) {}

	scopes[nmspace].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void Compiler::visit(ASTLiteralNode<cp_bool>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_int>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_float>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_char>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_CHAR;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_string>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.is_const = true;
}

void Compiler::visit(ASTArrayConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto is_const = true;
	cp_int arr_size = 0;

	if (current_expression_array_dim.size() == 0) {
		current_expression_array_type = TypeDefinition();
		current_expression_array_dim_max = 0;
		is_max = false;
	}

	++current_expression_array_dim_max;
	if (!is_max) {
		current_expression_array_dim.push_back(new ASTLiteralNode<cp_int>(0, astnode->row, astnode->col));
	}

	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
		if (!current_expression.is_const) {
			is_const = false;
		}

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		if (is_undefined(current_expression_array_type.type) || is_array(current_expression_array_type.type)) {
			current_expression_array_type = current_expression;
		}
		else {
			if (!match_type(current_expression_array_type.type, current_expression.type)
				&& !is_any(current_expression.type) && !is_void(current_expression.type)
				&& !is_array(current_expression.type)) {
				current_expression_array_type = TypeDefinition::get_basic(Type::T_ANY);
			}
		}

		++arr_size;
	}

	if (!is_max) {
		((ASTLiteralNode<cp_int>*)current_expression_array_dim.back())->val = arr_size;
	}

	is_max = true;

	current_expression = SemanticValue();
	current_expression.type = Type::T_ARRAY;
	current_expression.array_type = current_expression_array_type.type;
	current_expression.type_name = current_expression_array_type.type_name;
	current_expression.type_name_space = current_expression_array_type.type_name_space;
	current_expression.is_const = is_const;
	--current_expression_array_dim_max;
	size_t stay = current_expression_array_dim.size() - current_expression_array_dim_max;
	std::vector<ASTExprNode*> current_expression_array_dim_aux;
	size_t curr_dim_i = current_expression_array_dim.size() - 1;
	for (size_t i = 0; i < stay; ++i) {
		current_expression_array_dim_aux.emplace(current_expression_array_dim_aux.begin(), current_expression_array_dim.at(curr_dim_i));
		--curr_dim_i;
	}
	current_expression.dim = current_expression_array_dim_aux;

	if (current_expression_array_dim_max == 0) {
		if (is_undefined(current_expression.array_type)) {
			current_expression.array_type = Type::T_ANY;
		}
		current_expression_array_dim.clear();
	}
}

void Compiler::visit(ASTStructConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto is_const = true;

	std::shared_ptr<CompilerScope> curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error("struct '" + astnode->type_name +
			"' was not declared");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);
	auto& type_struct_ptr = type_struct.first;

	for (const auto& expr : astnode->values) {
		if (type_struct_ptr.variables.find(expr.first) == type_struct_ptr.variables.end()) {
			ExceptionHandler::throw_struct_member_err(astnode->nmspace, astnode->type_name, expr.first);
		}
		const auto& var_type_struct = type_struct_ptr.variables[expr.first];
		expr.second->accept(this);
		if (!current_expression.is_const) {
			is_const = false;
		}

		if (!TypeDefinition::is_any_or_match_type(var_type_struct,
			current_expression, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_struct_type_err(astnode->nmspace, astnode->type_name, var_type_struct, evaluate_access_vector_ptr);
		}
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRUCT;
	current_expression.type_name = astnode->type_name;
	current_expression.type_name_space = astnode->nmspace;
	current_expression.is_const = is_const;
}

void Compiler::visit(ASTIdentifierNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::shared_ptr<CompilerScope> curr_scope;
	const auto& nmspace = get_namespace(astnode->nmspace);
	try {
		curr_scope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (...) {
		current_expression = SemanticValue();
		if (astnode->identifier == "bool") {
			current_expression.type = Type::T_BOOL;
			return;
		}
		else if (astnode->identifier == "int") {
			current_expression.type = Type::T_INT;
			return;
		}
		else if (astnode->identifier == "float") {
			current_expression.type = Type::T_FLOAT;
			return;
		}
		else if (astnode->identifier == "char") {
			current_expression.type = Type::T_CHAR;
			return;
		}
		else if (astnode->identifier == "string") {
			current_expression.type = Type::T_STRING;
			return;
		}
		try {
			curr_scope = get_inner_most_struct_definition_scope(
				nmspace, astnode->identifier);
			current_expression.type = Type::T_STRUCT;
			return;
		}
		catch (...) {
			try {
				curr_scope = get_inner_most_function_scope(nmspace,
					astnode->identifier, nullptr);
				current_expression.type = Type::T_FUNCTION;

				return;
			}
			catch (...) {
				throw std::runtime_error("identifier '" + astnode->identifier +
					"' was not declared");
			}
		}
	}

	const auto& declared_variable = curr_scope->find_declared_variable(astnode->identifier);
	const auto& declared_variable_ptr = declared_variable.first;
	auto variable_expr = access_value(declared_variable_ptr->value, astnode->identifier_vector);
	variable_expr->reset_ref();

	if (is_undefined(variable_expr->type)) {
		throw std::runtime_error("variable '" + astnode->identifier + "' is undefined");
	}

	current_expression = *variable_expr;
	current_expression.reset_ref();
	current_expression.is_sub = declared_variable_ptr->value != variable_expr;

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

bool Compiler::namespace_exists(const std::string& nmspace) {
	return scopes.find(nmspace) != scopes.end();
}

std::shared_ptr<CompilerScope> Compiler::get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_variable(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("identifier '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<CompilerScope> Compiler::get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_structure_definition(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("struct '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<CompilerScope> Compiler::get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>* signature, bool strict) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("function '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

TypeDefinition Compiler::do_operation(const std::string& op, TypeDefinition lvar, TypeDefinition lvalue, TypeDefinition* rvar, TypeDefinition rvalue, bool is_expr) {
	Type l_var_type = lvar.type;
	lvalue = is_undefined(lvalue.type) ? lvar : lvalue;
	Type l_type = lvalue.type;
	Type r_var_type = rvar ? rvar->type : rvalue.type;
	Type r_type = rvalue.type;

	if ((is_any(l_var_type) || is_any(r_var_type)
		|| is_any(l_type) || is_any(r_type)
		|| is_void(l_type) || is_void(r_type)) && op == "=") {
		return rvalue;
	}

	if ((is_void(l_type) || is_void(r_type))
		&& Token::is_equality_op(op)) {
		return TypeDefinition::get_basic(Type::T_BOOL);
	}

	if (is_any(l_type) || is_any(r_type)) {
		if (Token::is_relational_op(op)) {
			return TypeDefinition::get_basic(Type::T_BOOL);
		}
		return is_any(r_type) ?
			!rvar || is_any(r_var_type) ?
			is_any(l_type) ? lvar : lvalue
			: *rvar
			: rvalue;
	}

	switch (r_type) {
	case Type::T_BOOL: {
		if (!is_bool(l_type)
			|| op != "="
			&& op != "and"
			&& op != "or"
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_INT: {
		if (is_float(l_type) && is_any(l_var_type)
			|| is_int(l_type) && is_any(l_var_type)
			&& !Token::is_int_ex_op(op)) {
			if (!Token::is_float_op(op)
				&& !Token::is_relational_op(op)
				&& !Token::is_equality_op(op)) {
				ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
			}
		}
		else if (is_int(l_type)
			&& !Token::is_int_op(op)
			&& !Token::is_relational_op(op)
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}
		else if (!is_numeric(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_FLOAT: {
		if ((is_float(l_type) || is_int(l_type))
			&& !Token::is_float_op(op)
			&& !Token::is_relational_op(op)
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}
		else if (!is_numeric(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_CHAR: {
		if (is_string(l_type) && !Token::is_collection_op(op)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}
		else if (is_char(l_type)) {
			if (op != "=" && !Token::is_equality_op(op) && !is_expr) {
				ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
			}
		}
		else if (!is_text(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_STRING: {
		if ((!is_string(l_type)
			|| (!Token::is_collection_op(op)
				&& !Token::is_equality_op(op)))
			&& (is_expr && (!is_char(l_type)
				|| !Token::is_expression_collection_op(op)))) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}
		else if (!is_text(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_ARRAY: {
		if (!TypeDefinition::match_type_array(lvalue, rvalue, evaluate_access_vector_ptr)
			|| (!Token::is_collection_op(op)
				&& !Token::is_equality_op(op))) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_STRUCT: {
		if (!is_struct(l_type) || (!Token::is_equality_op(op) && op != "=")) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_FUNCTION: {
		if (!is_function(l_type) || (!Token::is_equality_op(op) && op != "=")) {
			ExceptionHandler::throw_operation_err(op, lvalue, rvalue, evaluate_access_vector_ptr);
		}

		break;
	}
	default:
		throw std::runtime_error("cannot determine type of operation");

	}

	if (Token::is_equality_op(op) || Token::is_relational_op(op)) {
		return TypeDefinition::get_basic(Type::T_BOOL);
	}

	return is_float(lvalue.type) || is_string(lvalue.type) ? lvalue : rvalue;
}

void Compiler::equals_value(const SemanticValue& lval, const SemanticValue& rval) {
	if (lval.use_ref && !rval.use_ref) {
		throw std::runtime_error("both values must be references");
	}
	if (!lval.use_ref && rval.use_ref) {
		throw std::runtime_error("both values must be unreferenced");
	}
}

std::shared_ptr<SemanticValue> Compiler::access_value(std::shared_ptr<SemanticValue> value, const std::vector<Identifier>& identifier_vector, size_t i) {
	std::shared_ptr<SemanticValue> next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		if (access_vector.size() == value->dim.size()) {
			next_value = std::make_shared<SemanticValue>(next_value->array_type, Type::T_UNDEFINED,
				std::vector<ASTExprNode*>(), next_value->type_name, next_value->type_name_space,
				0, false, next_value->row, next_value->col);
		}
		else if (access_vector.size() - 1 == value->dim.size()
			&& is_string(next_value->type)) {
			next_value = std::make_shared<SemanticValue>(Type::T_CHAR, Type::T_UNDEFINED,
				std::vector<ASTExprNode*>(), "", "",
				0, false, next_value->row, next_value->col);
		}
	}

	++i;

	if (i < identifier_vector.size()) {
		if (next_value->type_name.empty()) {
			next_value = std::make_shared<SemanticValue>(Type::T_ANY, next_value->row, next_value->col);
		}
		else {
			std::shared_ptr<CompilerScope> curr_scope;
			try {
				auto nmspace = get_namespace(next_value->type_name_space);
				curr_scope = get_inner_most_struct_definition_scope(nmspace, next_value->type_name);
			}
			catch (...) {
				throw std::runtime_error("cannot find struct");
			}
			const auto& type_struct = curr_scope->find_declared_structure_definition(next_value->type_name);
			auto type_struct_ptr = type_struct.first;

			if (type_struct_ptr.variables.find(identifier_vector[i].identifier) == type_struct_ptr.variables.end()) {
				ExceptionHandler::throw_struct_member_err(next_value->type_name_space, next_value->type_name, identifier_vector[i].identifier);
			}

			next_value = std::make_shared<SemanticValue>(type_struct_ptr.variables[identifier_vector[i].identifier], 0, false, next_value->row, next_value->col);
		}

		if (identifier_vector[i].access_vector.size() > 0 || i < identifier_vector.size()) {
			return access_value(next_value, identifier_vector, i);
		}
	}

	return next_value;
}

void Compiler::check_is_struct_exists(parser::Type type, const std::string& nmspace, const std::string& type_name) {
	if (is_struct(type)) {
		try {
			get_inner_most_struct_definition_scope(get_namespace(nmspace), type_name);
		}
		catch (...) {
			throw std::runtime_error("struct '" + type_name + "' was not defined");
		}
	}
}

std::vector<unsigned int> Compiler::evaluate_access_vector(const std::vector<ASTExprNode*>& expr_access_vector) {
	auto access_vector = std::vector<unsigned int>();
	for (const auto& expr : expr_access_vector) {
		unsigned int val = 0;
		if (expr) {
			expr->accept(this);
			val = expr->hash(this);
			if (!is_int(current_expression.type) && !is_any(current_expression.type)) {
				throw std::runtime_error("array index access must be a integer value");
			}
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

bool Compiler::returns(ASTNode* astnode) {
	if (dynamic_cast<ASTReturnNode*>(astnode)
		|| dynamic_cast<ASTThrowNode*>(astnode)) {
		return true;
	}

	if (const auto& block = dynamic_cast<ASTBlockNode*>(astnode)) {
		for (const auto& blk_stmt : block->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	if (const auto& ifstmt = dynamic_cast<ASTIfNode*>(astnode)) {
		bool ifreturn = returns(ifstmt->if_block);
		bool elifreturn = true;
		bool elsereturn = true;
		for (const auto& elif : ifstmt->else_ifs) {
			if (!returns(elif->block)) {
				elifreturn = false;
				break;
			}
		}
		if (ifstmt->else_block) {
			elsereturn = returns(ifstmt->else_block);
		}
		return ifreturn && elifreturn && elsereturn;
	}

	if (const auto& trycatchstmt = dynamic_cast<ASTTryCatchNode*>(astnode)) {
		return returns(trycatchstmt->try_block) && returns(trycatchstmt->catch_block);
	}

	if (const auto& switchstmt = dynamic_cast<ASTSwitchNode*>(astnode)) {
		for (const auto& blk_stmt : switchstmt->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	if (const auto& forstmt = dynamic_cast<ASTForNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (const auto& forstmt = dynamic_cast<ASTForEachNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (const auto& whilestmt = dynamic_cast<ASTWhileNode*>(astnode)) {
		return returns(whilestmt->block);
	}

	return false;
}

long long Compiler::hash(ASTExprNode* astnode) {
	astnode->accept(this);
	return 0;
}

long long Compiler::hash(ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Compiler::hash(ASTLiteralNode<cp_int>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Compiler::hash(ASTLiteralNode<cp_float>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Compiler::hash(ASTLiteralNode<cp_char>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Compiler::hash(ASTLiteralNode<cp_string>* astnode) {
	return axe::StringUtils::hashcode(astnode->val);
}

long long Compiler::hash(ASTIdentifierNode* astnode) {
	astnode->accept(this);
	return current_expression.hash;
}

const std::string& Compiler::get_namespace(const std::string& nmspace) const {
	return get_namespace(current_program, nmspace);
}

const std::string& Compiler::get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace) const {
	return nmspace.empty() ? (
		program->alias.empty() ? (
			!current_function.empty() && !current_function.top().type_name_space.empty() ? current_function.top().type_name_space : default_namespace
			) : program->alias
		) : nmspace;
}

void Compiler::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Compiler::msg_header() {
	return "(SERR) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
