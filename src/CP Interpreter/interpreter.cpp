#include <iostream>
#include <algorithm> 
#include <cmath>
#include <conio.h>
#include <compare>
#include <functional>

#include "interpreter.hpp"
#include "exception_handler.hpp"
#include "token.hpp"

#include "vendor/axeutils.hpp"
#include "vendor/axewatch.hpp"
#include "vendor/axeuuid.hpp"

#include "graphics.hpp"
#include "files.hpp"
#include "console.hpp"

using namespace lexer;

Interpreter::Interpreter(InterpreterScope* global_scope, ASTProgramNode* main_program, const std::map<std::string, ASTProgramNode*>& programs)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace),
	current_expression_value(new Value(Type::T_UNDEFINED)) {

	if (main_program) {
		current_this_name.push(main_program->name);
	}
	scopes[default_namespace].push_back(global_scope);
	register_built_in_functions();
}

void Interpreter::start() {
	visit(current_program);
}

void Interpreter::visit(ASTProgramNode* astnode) {
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			if (curr_row == 0 || curr_col == 0) {
				set_curr_pos(statement->row, statement->col);
			}
			if (exception) {
				throw std::runtime_error(ex.what());
			}
			exception = true;
			throw std::runtime_error(msg_header() + ex.what());
		}
	}

	if (astnode->statements.size() > 1
		|| astnode->statements.size() > 0
		&& !dynamic_cast<ASTExprNode*>(astnode->statements[0])) {
		current_expression_value = new Value(Type::T_UNDEFINED);
	}
}

void Interpreter::visit(ASTUsingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (axe::StringUtils::contains(built_in_libs, libname)) {
		register_built_in_lib(libname);
	}

	auto program = programs[libname];

	// add lib to current program
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::StringUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		program_nmspaces[get_current_namespace()].push_back(default_namespace);
		if (!program->alias.empty()) {
			scopes[program->alias].push_back(new InterpreterScope());
		}
		current_this_name.push(current_program->name);
		start();
		current_this_name.pop();
		current_program = prev_program;
	}
}

void Interpreter::visit(ASTAsNamespaceNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	program_nmspaces[get_namespace(current_program->alias)].push_back(astnode->nmspace);
}

void Interpreter::visit(ASTEnumNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		scopes[nmspace].back()->declare_variable(astnode->identifiers[i],
			new Variable(Type::T_INT, Type::T_UNDEFINED, std::vector<ASTExprNode*>(),
				"", "", new Value(cp_int(i))));
	}
}

void Interpreter::visit(ASTDeclarationNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		current_expression_value = new Value(Type::T_UNDEFINED);
	}

	Value* new_value;

	if (current_expression_value->use_ref) {
		new_value = current_expression_value;
	}
	else {
		new_value = new Value(current_expression_value);
	}

	auto& astnode_type_name = astnode->type_name.empty() ? new_value->type_name : astnode->type_name;

	auto new_var = new Variable(astnode->type,
		astnode->array_type, astnode->dim,
		astnode_type_name, astnode->type_name_space,
		new_value);

	if ((!TypeDefinition::is_any_or_match_type(new_var, *new_var, nullptr, *new_value, evaluate_access_vector_ptr) ||
		is_array(new_var->type) && !is_any(new_var->array_type)
		&& !TypeDefinition::match_type(*new_var, *new_value, evaluate_access_vector_ptr, false, true))
		&& astnode->expr && !is_undefined(new_value->type)) {
		ExceptionHandler::throw_declaration_type_err(astnode->identifier, *new_var, *new_value, evaluate_access_vector_ptr);
	}

	check_build_array(new_value, astnode->dim);

	normalize_type(new_var, new_value);

	scopes[nmspace].back()->declare_variable(astnode->identifier, new_var);
}

void Interpreter::visit(ASTUnpackedDeclarationNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void Interpreter::visit(ASTAssignmentNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	InterpreterScope* astscope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		astscope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	Variable* variable = astscope->find_declared_variable(astnode->identifier);
	Value* value = access_value(astscope, variable->get_value(), astnode->identifier_vector);

	astnode->expr->accept(this);

	auto ptr_value = current_expression_value;
	Value* new_value = nullptr;
	if (ptr_value->use_ref) {
		new_value = ptr_value;
	}
	else {
		new_value = new Value(ptr_value);
	}

	auto arr = new_value->get_arr();

	std::vector<unsigned int> dim;
	if (variable->dim.size() > 0) {
		dim = evaluate_access_vector(variable->dim);
	}

	check_build_array(new_value, variable->dim);

	if (astnode->op == "="
		&& astnode->identifier_vector.size() == 1
		&& astnode->identifier_vector[0].access_vector.size() == 0
		&& !has_string_access) {
		if (!TypeDefinition::is_any_or_match_type(variable, *variable, nullptr, *ptr_value, evaluate_access_vector_ptr) ||
			is_array(variable->type) && !is_any(variable->array_type)
			&& !TypeDefinition::match_type(*variable, *ptr_value, evaluate_access_vector_ptr, false, true)) {
			ExceptionHandler::throw_mismatched_type_err(*variable, *ptr_value, evaluate_access_vector_ptr);
		}

		normalize_type(variable, new_value);

		variable->set_value(new_value);
	}
	else {
		if (astnode->identifier_vector.size() == 1 && astnode->identifier_vector[0].access_vector.size() == 0) {
			variable->set_value(new Value(variable->get_value()));
			value = variable->get_value();
		}

		cp_int pos = 0;
		if (has_string_access) {
			astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1]->accept(this);
			pos = current_expression_value->get_i();
		}

		if (astnode->op == "=") {
			if (is_string(variable->type) && is_char(new_value->type)
				&& new_value->use_ref && new_value->ref && !is_any(new_value->ref->type)) {
				throw std::runtime_error("cannot reference char to string in function call");
			}
			else if (is_float(variable->type) && is_int(new_value->type)
				&& new_value->use_ref && new_value->ref && !is_any(new_value->ref->type)) {
				throw std::runtime_error("cannot reference int to float in function call");
			}
			set_value(astscope, astnode->identifier_vector, new_value);
		}
		else {
			normalize_type(variable, new_value);
			do_operation(astnode->op, value, new_value, false, pos);
		}
	}
}

void Interpreter::visit(ASTReturnNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	if (astnode->expr) {
		auto& curr_func_ret_type = current_function_return_type.top();
		InterpreterScope* func_scope = get_inner_most_function_scope(nmspace, current_function_call_identifier_vector.top()[0].identifier, &current_function_signature.top());

		astnode->expr->accept(this);
		Value* value = access_value(func_scope, current_expression_value, current_function_call_identifier_vector.top());

		if (value->type == Type::T_STRING && current_function_call_identifier_vector.top().back().access_vector.size() > 0 && has_string_access) {
			has_string_access = false;
			std::string str = value->get_s();
			current_function_call_identifier_vector.top().back().access_vector[current_function_call_identifier_vector.top().back().access_vector.size() - 1]->accept(this);
			auto pos = value->get_i();

			auto char_value = new Value(Type::T_CHAR);
			char_value->set(cp_char(str[pos]));
			value = char_value;
		}

		if (!TypeDefinition::is_any_or_match_type(
			&curr_func_ret_type, curr_func_ret_type,
			nullptr, *value, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_return_type_err(current_this_name.top(),
				curr_func_ret_type, *value, evaluate_access_vector_ptr);
		}

		if (value->use_ref) {
			current_expression_value = value;
		}
		else {
			current_expression_value = new Value(value);
		}
	}
	else {
		current_expression_value = new Value(Type::T_UNDEFINED);
	}

	for (long long i = scopes[nmspace].size() - 1; i >= 0; --i) {
		if (!scopes[nmspace][i]->get_name().empty()) {
			return_from_function_name = scopes[nmspace][i]->get_name();
			return_from_function = true;
			break;
		}
	}
}

void Interpreter::visit(ASTFunctionCallNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::string nmspace = get_namespace(astnode->nmspace);
	std::string identifier = astnode->identifier;
	std::vector<Identifier> identifier_vector = astnode->identifier_vector;
	bool strict = true;
	std::vector<TypeDefinition> signature;
	std::vector<Value*> function_arguments;

	for (auto& param : astnode->parameters) {
		param->accept(this);

		signature.push_back(*current_expression_value);

		Value* pvalue = nullptr;
		if (current_expression_value->use_ref) {
			pvalue = current_expression_value;
		}
		else {
			pvalue = new Value(current_expression_value);
		}

		function_arguments.push_back(pvalue);
	}

	InterpreterScope* func_scope;
	try {
		func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
	}
	catch (...) {
		try {
			strict = false;
			func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
		}
		catch (...) {
			try {
				auto var_scope = get_inner_most_variable_scope(nmspace, identifier);
				auto var = var_scope->find_declared_variable(identifier);
				nmspace = var->value->get_fun().first;
				identifier = var->value->get_fun().second;
				identifier_vector = std::vector<Identifier>{ Identifier(identifier) };
				func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
			}
			catch (...) {
				std::string func_name = identifier + "(";
				for (const auto& param : signature) {
					func_name += type_str(param.type) + ", ";
				}
				if (signature.size() > 0) {
					func_name.pop_back();
					func_name.pop_back();
				}
				func_name += ")";

				throw std::runtime_error("function '" + func_name + "' was never declared");
			}
		}
	}
	auto declfun = func_scope->find_declared_function(identifier, &signature, evaluate_access_vector_ptr, strict);

	current_function_defined_parameters.push(std::get<0>(declfun));

	current_this_name.push(identifier);
	current_function_signature.push(signature);
	current_function_call_identifier_vector.push(identifier_vector);
	current_function_nmspace.push(nmspace);
	current_function_return_type.push(std::get<2>(declfun));
	current_function_calling_arguments.push(function_arguments);

	auto block = std::get<1>(declfun);
	if (block) {
		function_call_name = identifier;
		block->accept(this);
	}
	else {
		call_builtin_function(identifier);
	}

	current_function_return_type.pop();
	current_function_call_identifier_vector.pop();
	current_function_signature.pop();
	current_function_nmspace.pop();
	current_this_name.pop();
}

void Interpreter::visit(ASTFunctionDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	interpreter_parameter_list_t params;
	for (size_t i = 0; i < astnode->parameters.size(); ++i) {
		interpreter_parameter_t param = std::make_tuple(astnode->variable_names[i], astnode->signature[i], astnode->parameters[i].default_value, astnode->parameters[i].is_rest);
		params.push_back(param);
	}

	if (astnode->identifier != "") {
		scopes[nmspace].back()->declare_function(astnode->identifier, params, astnode->block, *astnode);
	}
}

void Interpreter::visit(ASTFunctionExpression* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	auto& fun = astnode->fun;

	interpreter_parameter_list_t params;
	for (size_t i = 0; i < fun->parameters.size(); ++i) {
		interpreter_parameter_t param = std::make_tuple(fun->variable_names[i], fun->signature[i], fun->parameters[i].default_value, fun->parameters[i].is_rest);
		params.push_back(param);
	}

	std::string identifier = "__unnamed_function_" + axe::AxeUUID::generate();

	scopes[nmspace].back()->declare_function(identifier, params, fun->block, *fun);

	auto value = new Value(Type::T_FUNCTION);
	value->set(cp_function(nmspace, identifier));
	current_expression_value = value;
}

void Interpreter::visit(ASTBlockNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	scopes[nmspace].push_back(new InterpreterScope(function_call_name));
	function_call_name = "";

	declare_function_block_parameters(nmspace);

	for (auto& stmt : astnode->statements) {
		stmt->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block && (is_loop)) {
			break;
		}

		if (break_block && (is_loop || is_switch)) {
			break;
		}

		if (return_from_function) {
			if (!return_from_function_name.empty() && return_from_function_name == scopes[nmspace].back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	scopes[nmspace].pop_back();
}

void Interpreter::visit(ASTExitNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->exit_code->accept(this);
	if (!is_int(current_expression_value->type)) {
		throw std::runtime_error("expected int value");
	}
	exit_from_program = true;
}

void Interpreter::visit(ASTContinueNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	continue_block = true;
}

void Interpreter::visit(ASTBreakNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	break_block = true;
}

void Interpreter::visit(ASTSwitchNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_switch;

	scopes[nmspace].push_back(new InterpreterScope(""));

	long long pos = -1;
	if (astnode->case_blocks.size() > 0) {
		astnode->condition->accept(this);
		TypeDefinition cond_type = *current_expression_value;
		for (const auto& expr : astnode->case_blocks) {
			expr.first->accept(this);
			break;
		}
		TypeDefinition case_type = *current_expression_value;

		if (!TypeDefinition::match_type(cond_type, case_type, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_mismatched_type_err(cond_type, case_type, evaluate_access_vector_ptr);
		}
	}

	try {
		auto hash = astnode->condition->hash(this);
		pos = astnode->parsed_case_blocks.at(hash);
	}
	catch (...) {
		pos = astnode->default_block;
	}

	for (long long i = pos; i < astnode->statements.size(); ++i) {
		astnode->statements.at(i)->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block) {
			continue_block = false;
			continue;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (return_from_function) {
			if (!return_from_function_name.empty() && return_from_function_name == scopes[nmspace].back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	scopes[nmspace].pop_back();
	--is_switch;
}

void Interpreter::visit(ASTElseIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	executed_elif = false;

	astnode->condition->accept(this);

	if (!is_bool(current_expression_value->type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	bool result = current_expression_value->get_b();

	if (result) {
		astnode->block->accept(this);
		executed_elif = true;
	}
}

void Interpreter::visit(ASTIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (!is_bool(current_expression_value->type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	bool result = current_expression_value->get_b();

	if (result) {
		astnode->if_block->accept(this);
	}
	else {
		for (auto& elif : astnode->else_ifs) {
			elif->accept(this);
			if (executed_elif) {
				break;
			}
		}
		if (astnode->else_block && !executed_elif) {
			astnode->else_block->accept(this);
		}
	}

	executed_elif = false;
}

void Interpreter::visit(ASTForNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_loop;
	scopes[nmspace].push_back(new InterpreterScope(""));

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);

		if (!is_bool(current_expression_value->type)) {
			ExceptionHandler::throw_condition_type_err();
		}
	}
	else {
		current_expression_value = new Value(Type::T_BOOL);
		current_expression_value->set(true);
	}

	bool result = current_expression_value->get_b();

	while (result) {
		astnode->block->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block) {
			continue_block = false;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (return_from_function) {
			break;
		}

		if (astnode->dci[2]) {
			astnode->dci[2]->accept(this);
		}

		if (astnode->dci[1]) {
			astnode->dci[1]->accept(this);

			if (!is_bool(current_expression_value->type)) {
				ExceptionHandler::throw_condition_type_err();
			}
		}
		else {
			current_expression_value = new Value(Type::T_BOOL);
			current_expression_value->set(true);
		}

		result = current_expression_value->get_b();
	}

	scopes[nmspace].pop_back();
	--is_loop;
}

void Interpreter::visit(ASTForEachNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_loop;

	astnode->collection->accept(this);

	scopes[nmspace].push_back(new InterpreterScope(""));

	auto itdecl = static_cast<ASTDeclarationNode*>(astnode->itdecl);

	switch (current_expression_value->type) {
	case Type::T_ARRAY: {
		const auto& colletion = current_expression_value->get_arr();
		for (size_t i = 0; i < colletion.second; ++i) {
			auto val = colletion.first[i];

			astnode->itdecl->accept(this);

			set_value(
				scopes[nmspace].back(),
				std::vector<Identifier>{Identifier(itdecl->identifier)},
				val);

			astnode->block->accept(this);

			if (exit_from_program) {
				return;
			}

			if (continue_block) {
				continue_block = false;
				continue;
			}

			if (break_block) {
				break_block = false;
				break;
			}

			if (return_from_function) {
				break;
			}
		}
		break;
	}
	case Type::T_STRING: {
		const auto& colletion = current_expression_value->get_s();
		for (auto val : colletion) {
			astnode->itdecl->accept(this);

			set_value(
				scopes[nmspace].back(),
				std::vector<Identifier>{Identifier(itdecl->identifier)},
				new Value(cp_char(val)));

			astnode->block->accept(this);

			if (exit_from_program) {
				return;
			}

			if (continue_block) {
				continue_block = false;
				continue;
			}

			if (break_block) {
				break_block = false;
				break;
			}

			if (return_from_function) {
				break;
			}
		}
		break;
	}
	case Type::T_STRUCT: {
		const auto& colletion = current_expression_value->get_str();
		for (const auto& val : colletion) {
			astnode->itdecl->accept(this);

			if (itdecl) {
				if (!is_any(current_expression_value->ref->type)
					&& current_expression_value->ref->type_name_space != "cp"
					&& current_expression_value->ref->type_name != "Pair") {
					throw std::runtime_error("invalid struct '"
						+ current_expression_value->ref->type_name
						+ "' declaration in foreach loop");
				}

				auto str = cp_struct();
				str["key"] = new Value(cp_string(val.first));
				str["value"] = val.second;
				auto str_value = new Value(str, "cp", "Pair");

				set_value(
					scopes[nmspace].back(),
					std::vector<Identifier>{Identifier(itdecl->identifier)},
					str_value);
			}
			else if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->itdecl)) {
				if (idnode->declarations.size() != 2) {
					throw std::runtime_error("invalid number of values");
				}

				InterpreterScope* back_scope = scopes[nmspace].back();
				auto decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
				decl_key->set_value(new Value(cp_string(val.first)));

				back_scope = scopes[nmspace].back();
				auto decl_val = back_scope->find_declared_variable(idnode->declarations[1]->identifier);
				decl_val->set_value(val.second);
			}

			astnode->block->accept(this);

			if (exit_from_program) {
				return;
			}

			if (continue_block) {
				continue_block = false;
				continue;
			}

			if (break_block) {
				break_block = false;
				break;
			}

			if (return_from_function) {
				break;
			}
		}
		break;
	}
	default:
		throw std::exception("invalid foreach iterable type");
	}

	scopes[nmspace].pop_back();

	--is_loop;
}

void Interpreter::visit(ASTTryCatchNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	try {
		astnode->try_block->accept(this);
	}
	catch (std::exception ex) {
		scopes[nmspace].push_back(new InterpreterScope(""));

		astnode->decl->accept(this);

		if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->decl)) {
			if (idnode->declarations.size() != 1) {
				throw std::runtime_error("invalid number of values");
			}

			InterpreterScope* back_scope = scopes[nmspace].back();
			auto decl_val = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
			decl_val->set_value(new Value(cp_string(ex.what())));
		}
		else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
			if (!is_any(current_expression_value->ref->type)
				&& current_expression_value->ref->type_name_space != "cp"
				&& current_expression_value->ref->type_name != "Pair") {
				throw std::runtime_error("invalid struct '"
					+ current_expression_value->ref->type_name
					+ "' declaration in foreach loop");
			}

			Value* value = new Value(Type::T_STRUCT);
			value->get_str() = cp_struct();
			value->get_str()["error"] = new Value(cp_string(ex.what()));
			value->type_name_space = "cp";
			value->type_name = "Exception";

			current_expression_value->ref->set_value(value);
		}
		else if (!dynamic_cast<ASTReticencesNode*>(astnode->decl)) {
			throw std::runtime_error("expected declaration");
		}

		astnode->catch_block->accept(this);
		scopes[nmspace].pop_back();
	}
}

void Interpreter::visit(parser::ASTThrowNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->error->accept(this);

	if (is_struct(current_expression_value->type)
		&& current_expression_value->type_name == "Exception") {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}

		throw std::exception(current_expression_value->get_str()["error"]->get_s().c_str());
	}
	else if (is_string(current_expression_value->type)) {
		throw std::runtime_error(current_expression_value->get_s());
	}
	else {
		throw std::runtime_error("expected Exception cp::Exception struct or string in throw");
	}

}

void Interpreter::visit(parser::ASTReticencesNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_UNDEFINED);
	value->set_undefined();
	current_expression_value = value;
}

void Interpreter::visit(ASTWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	++is_loop;

	astnode->condition->accept(this);

	if (!is_bool(current_expression_value->type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	bool result = current_expression_value->get_b();

	while (result) {
		astnode->block->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block) {
			continue_block = false;
			//continue;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (return_from_function) {
			break;
		}

		astnode->condition->accept(this);

		if (!is_bool(current_expression_value->type)) {
			ExceptionHandler::throw_condition_type_err();
		}

		result = current_expression_value->get_b();
	}

	--is_loop;
}

void Interpreter::visit(ASTDoWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	++is_loop;

	bool result = false;

	do {
		astnode->block->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block) {
			continue_block = false;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (return_from_function) {
			break;
		}

		astnode->condition->accept(this);

		if (!is_bool(current_expression_value->type)) {
			ExceptionHandler::throw_condition_type_err();
		}

		result = current_expression_value->get_b();
	} while (result);

	--is_loop;
}

void Interpreter::visit(ASTStructDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	scopes[get_namespace()].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void Interpreter::visit(ASTLiteralNode<cp_bool>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_BOOL);
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_int>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_INT);
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_float>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_FLOAT);
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_char>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_CHAR);
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_string>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_STRING);
	value->set(astnode->val);
	current_expression_value = value;
}

std::vector<ASTExprNode*> current_expression_array_dim;
int current_expression_array_dim_max;
Type current_expression_array_type;
bool is_max;

void Interpreter::visit(ASTArrayConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_ARRAY);
	Type arr_t = Type::T_ANY;
	cp_array arr = cp_array(new Value * [astnode->values.size()], astnode->values.size());

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
		const auto expr = astnode->values[i];

		expr->accept(this);

		if (is_undefined(current_expression_array_type.type) || is_array(current_expression_array_type.type)) {
			current_expression_array_type = *current_expression_value;
		}
		else {
			if (!match_type(current_expression_array_type.type, current_expression_value->type)
				&& !is_any(current_expression_value->type) && !is_void(current_expression_value->type)
				&& !is_array(current_expression_value->type)) {
				current_expression_array_type = TypeDefinition::get_basic(Type::T_ANY);
			}
		}

		Value* arr_value = nullptr;
		if (current_expression_value->use_ref) {
			arr_value = current_expression_value;
		}
		else {
			arr_value = new Value(current_expression_value);
		}
		arr.first[i] = arr_value;
	}

	if (!is_max) {
		((ASTLiteralNode<cp_int>*)current_expression_array_dim.back())->val = arr.second;
	}

	is_max = true;

	value->set(arr, arr_t, current_expression_array_dim, current_expression_array_type.type_name, current_expression_array_type.type_name_space);

	current_expression_value = value;
	current_expression_value->array_type = current_expression_array_type.type;
	current_expression_value->type_name = current_expression_array_type.type_name;
	current_expression_value->type_name_space = current_expression_array_type.type_name_space;
	--current_expression_array_dim_max;
	size_t stay = current_expression_array_dim.size() - current_expression_array_dim_max;
	std::vector<ASTExprNode*> current_expression_array_dim_aux;
	size_t curr_dim_i = current_expression_array_dim.size() - 1;
	for (size_t i = 0; i < stay; ++i) {
		current_expression_array_dim_aux.emplace(current_expression_array_dim_aux.begin(), current_expression_array_dim.at(curr_dim_i));
		--curr_dim_i;
	}
	current_expression_value->dim = current_expression_array_dim_aux;

	if (current_expression_array_dim_max == 0) {
		if (is_undefined(current_expression_value->array_type)) {
			current_expression_value->array_type = Type::T_ANY;
		}
		current_expression_array_dim.clear();
	}
}

void Interpreter::visit(ASTStructConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	InterpreterScope* curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error("error trying to find struct definition");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);

	auto value = new Value(Type::T_STRUCT);

	auto str = cp_struct();

	for (auto& expr : astnode->values) {
		if (type_struct.variables.find(expr.first) == type_struct.variables.end()) {
			ExceptionHandler::throw_struct_member_err(astnode->nmspace, astnode->type_name, expr.first);
		}
		VariableDefinition var_type_struct = type_struct.variables[expr.first];

		expr.second->accept(this);

		Value* str_value = current_expression_value;

		if (!TypeDefinition::is_any_or_match_type(&var_type_struct, var_type_struct, nullptr, *current_expression_value, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_struct_type_err(astnode->nmspace, astnode->type_name, var_type_struct, evaluate_access_vector_ptr);
		}

		if (!current_expression_value->use_ref) {
			str_value = new Value(str_value);
		}

		std::vector<unsigned int> dim;
		if (var_type_struct.dim.size() > 0) {
			dim = evaluate_access_vector(var_type_struct.dim);
		}

		check_build_array(str_value, var_type_struct.dim);

		if (!is_any(var_type_struct.type) && !is_void(str_value->type)) {
			str_value->type = var_type_struct.type;
			str_value->array_type = var_type_struct.array_type;
			str_value->type_name = var_type_struct.type_name;
			str_value->type_name_space = var_type_struct.type_name_space;
		}

		str[expr.first] = str_value;
	}

	// declare rest values as null
	for (auto& struct_var_def : type_struct.variables) {
		if (str.find(struct_var_def.first) == str.end()) {
			Value* str_value = new Value(struct_var_def.second.type);
			str_value->set_null();
			str[struct_var_def.first] = str_value;
		}
	}

	value->set(str, astnode->type_name, astnode->nmspace);
	current_expression_value = value;
}

void Interpreter::visit(ASTIdentifierNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace(astnode->nmspace);
	InterpreterScope* id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (...) {
		const auto& dim = astnode->identifier_vector[0].access_vector;
		auto type = Type::T_UNDEFINED;
		auto expression_value = new Value(Type::T_UNDEFINED);

		if (astnode->identifier == "bool") {
			type = Type::T_BOOL;
		}
		else if (astnode->identifier == "int") {
			type = Type::T_INT;
		}
		else if (astnode->identifier == "float") {
			type = Type::T_FLOAT;
		}
		else if (astnode->identifier == "char") {
			type = Type::T_CHAR;
		}
		else if (astnode->identifier == "string") {
			type = Type::T_STRING;
		}

		if (is_undefined(type)) {
			InterpreterScope* curr_scope;
			try {
				curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->identifier);
			}
			catch (...) {
				try {
					curr_scope = get_inner_most_function_scope(nmspace, astnode->identifier, nullptr);
					auto fun = cp_function();
					fun.first = nmspace;
					fun.second = astnode->identifier;
					current_expression_value = new Value(Type::T_FUNCTION);
					current_expression_value->set(fun);
					return;
				}
				catch (...) {
					throw std::runtime_error("identifier '" + astnode->identifier + "' was not declared");
				}
			}
			type = Type::T_STRUCT;
			auto str = cp_struct();
			expression_value->set(str, astnode->identifier, nmspace);
		}

		expression_value->set_type(type);

		if (dim.size() > 0) {
			cp_array arr = build_array(dim, expression_value, dim.size() - 1);

			current_expression_value = new Value(arr, type, dim);
		}
		else {
			current_expression_value = new Value(expression_value);
		}

		return;
	}

	Variable* variable = id_scope->find_declared_variable(astnode->identifier);
	auto sub_val = access_value(id_scope, variable->get_value(), astnode->identifier_vector);
	sub_val->reset_ref();

	current_expression_value = sub_val;

	if (current_expression_value->type == Type::T_STRING && astnode->identifier_vector.back().access_vector.size() > 0 && has_string_access) {
		has_string_access = false;
		auto str = current_expression_value->get_s();
		astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1]->accept(this);
		auto pos = current_expression_value->get_i();

		auto char_value = new Value(Type::T_CHAR);
		char_value->set(cp_char(str[pos]));
		current_expression_value = char_value;
	}
}

void Interpreter::visit(ASTBinaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->left->accept(this);
	Value* l_value = nullptr;
	if (current_expression_value->use_ref) {
		l_value = current_expression_value;
	}
	else {
		l_value = new Value(current_expression_value);
	}

	if (is_bool(current_expression_value->type) && astnode->op == "and" && !current_expression_value->get_b()) {
		return;
	}

	astnode->right->accept(this);
	Value* r_value = nullptr;
	if (current_expression_value->use_ref) {
		r_value = current_expression_value;
	}
	else {
		r_value = new Value(current_expression_value);
	}

	current_expression_value = do_operation(astnode->op, l_value, r_value, true, 0);
}

void Interpreter::visit(ASTTernaryNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);
	if (current_expression_value->get_b()) {
		astnode->value_if_true->accept(this);
	}
	else {
		astnode->value_if_false->accept(this);
	}
}

void Interpreter::visit(ASTInNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->value->accept(this);
	Value expr_val = Value(current_expression_value);
	astnode->collection->accept(this);
	bool res = false;

	if (is_array(current_expression_value->type)) {
		cp_array expr_col = current_expression_value->get_arr();

		for (size_t i = 0; i < expr_col.second; ++i) {
			res = equals_value(&expr_val, expr_col.first[i]);
			if (res) {
				break;
			}
		}
	}
	else {
		const auto& expr_col = current_expression_value->get_s();

		if (is_char(expr_val.type)) {
			res = current_expression_value->get_s().find(expr_val.get_c()) != std::string::npos;
		}
		else {
			res = current_expression_value->get_s().find(expr_val.get_s()) != std::string::npos;
		}
	}

	auto value = new Value(Type::T_BOOL);
	value->set(res);
	current_expression_value = value;
}

void Interpreter::visit(ASTUnaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	bool has_assign = false;

	astnode->expr->accept(this);

	if (astnode->unary_op == "ref" || astnode->unary_op == "unref") {
		if (astnode->unary_op == "unref") {
			current_expression_value->use_ref = false;
		}
		else if (astnode->unary_op == "ref") {
			current_expression_value->use_ref = true;
		}
	}
	else {
		if (!current_expression_value->use_ref) {
			current_expression_value = new Value(current_expression_value);
		}

		switch (current_expression_value->type) {
		case Type::T_INT:
			if (astnode->unary_op == "-") {
				current_expression_value->set(cp_int(-current_expression_value->get_i()));
			}
			else if (astnode->unary_op == "--") {
				current_expression_value->set(cp_int(current_expression_value->get_i() - 1));
				has_assign = true;
			}
			else if (astnode->unary_op == "++") {
				current_expression_value->set(cp_int(current_expression_value->get_i() + 1));
				has_assign = true;
			}
			else if (astnode->unary_op == "~") {
				current_expression_value->set(cp_int(~current_expression_value->get_i()));
			}
			break;
		case Type::T_FLOAT:
			if (astnode->unary_op == "-") {
				current_expression_value->set(cp_float(-current_expression_value->get_f()));
			}
			else if (astnode->unary_op == "--") {
				current_expression_value->set(cp_float(current_expression_value->get_f() - 1));
				has_assign = true;
			}
			else if (astnode->unary_op == "++") {
				current_expression_value->set(cp_float(current_expression_value->get_f() + 1));
				has_assign = true;
			}
			break;
		case Type::T_BOOL:
			current_expression_value->set(cp_bool(!current_expression_value->get_b()));
			break;
		default:
			throw std::runtime_error("incompatible unary operator '" + astnode->unary_op +
				"' in front of " + type_str(current_expression_value->type) + " expression");
		}
	}

	if (has_assign) {
		const auto id = dynamic_cast<ASTIdentifierNode*>(astnode->expr);

		if (!id) {
			throw std::runtime_error("error unary assign");
		}

		const std::string& nmspace = get_namespace(id->nmspace);
		InterpreterScope* id_scope;
		try {
			id_scope = get_inner_most_variable_scope(nmspace, id->identifier);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}

		set_value(id_scope, id->identifier_vector, current_expression_value);
	}
}

void Interpreter::visit(ASTTypeParseNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	Value* new_value = new Value();

	switch (astnode->type) {
	case Type::T_BOOL:
		switch (current_expression_value->type) {
		case Type::T_BOOL:
			new_value->copy_from(current_expression_value);
			break;
		case Type::T_INT:
			new_value->set(cp_bool(current_expression_value->get_i() != 0));
			break;
		case Type::T_FLOAT:
			new_value->set(cp_bool(current_expression_value->get_f() != .0));
			break;
		case Type::T_CHAR:
			new_value->set(cp_bool(current_expression_value->get_c() != '\0'));
			break;
		case Type::T_STRING:
			new_value->set(cp_bool(!current_expression_value->get_s().empty()));
			break;
		}
		break;

	case Type::T_INT:
		switch (current_expression_value->type) {
		case Type::T_BOOL:
			new_value->set(cp_int(current_expression_value->get_b()));
			break;
		case Type::T_INT:
			new_value->copy_from(current_expression_value);
			break;
		case Type::T_FLOAT:
			new_value->set(cp_int(current_expression_value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->set(cp_int(current_expression_value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(cp_int(std::stoll(current_expression_value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + current_expression_value->get_s() + "' is not a valid value to parse int");
			}
			break;
		}
		break;

	case Type::T_FLOAT:
		switch (current_expression_value->type) {
		case Type::T_BOOL:
			new_value->set(cp_float(current_expression_value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(cp_float(current_expression_value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->copy_from(current_expression_value);
			break;
		case Type::T_CHAR:
			new_value->set(cp_float(current_expression_value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(cp_float(std::stold(current_expression_value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + current_expression_value->get_s() + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case Type::T_CHAR:
		switch (current_expression_value->type) {
		case Type::T_BOOL:
			new_value->set(cp_char(current_expression_value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(cp_char(current_expression_value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->set(cp_char(current_expression_value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->copy_from(current_expression_value);
			break;
		case Type::T_STRING:
			if (new_value->get_s().size() > 1) {
				throw std::runtime_error("'" + current_expression_value->get_s() + "' is not a valid value to parse char");
			}
			else {
				new_value->set(cp_char(current_expression_value->get_s()[0]));
			}
			break;
		}
		break;

	case Type::T_STRING:
		new_value->set(cp_string(parse_value_to_string(current_expression_value)));

	}

	new_value->set_type(astnode->type);

	current_expression_value = new_value;
}

void Interpreter::visit(ASTNullNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_VOID);
	current_expression_value = value;
}

void Interpreter::visit(ASTThisNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = new Value(Type::T_STRING);
	value->set(cp_string(current_this_name.top()));
	current_expression_value = value;
}

void Interpreter::visit(ASTTypingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (astnode->image == "is_any") {
		auto value = new Value(Type::T_BOOL);
		value->set(cp_bool(
			(current_expression_value->ref
				&& (is_any(current_expression_value->ref->type))
				|| is_any(current_expression_value->ref->array_type))));
		current_expression_value = value;
		return;
	}
	else if (astnode->image == "is_array") {
		auto value = new Value(Type::T_BOOL);
		value->set(cp_bool(is_array(current_expression_value->type) || current_expression_value->dim.size() > 0));
		current_expression_value = value;
		return;
	}
	else if (astnode->image == "is_struct") {
		auto value = new Value(Type::T_BOOL);
		value->set(cp_bool(is_struct(current_expression_value->type) || is_struct(current_expression_value->array_type)));
		current_expression_value = value;
		return;
	}

	auto curr_value = current_expression_value;
	auto dim = std::vector<unsigned int>();
	auto type = is_void(curr_value->type) ? curr_value->type : curr_value->type;
	std::string str_type = "";

	if (is_array(type)) {
		dim = calculate_array_dim_size(curr_value->get_arr());
		type = curr_value->array_type;
	}

	str_type = type_str(type);

	if (is_struct(type)) {
		if (dim.size() > 0) {
			auto arr = curr_value->get_arr().first[0];
			for (size_t i = 0; i < dim.size() - 1; ++i) {
				arr = arr->get_arr().first[0];
			}
			str_type = arr->type_name;
		}
		else {
			str_type = curr_value->type_name;
		}
	}

	if (dim.size() > 0) {
		for (size_t i = 0; i < dim.size(); ++i) {
			str_type += "[" + std::to_string(dim[i]) + "]";
		}
	}

	if (is_struct(type) && !curr_value->type_name_space.empty()) {
		str_type = curr_value->type_name_space + "::" + str_type;
	}

	if (astnode->image == "typeid") {
		auto value = new Value(Type::T_INT);
		value->set(cp_int(axe::StringUtils::hashcode(str_type)));
		current_expression_value = value;
	}
	else {
		auto value = new Value(Type::T_STRING);
		value->set(cp_string(str_type));
		current_expression_value = value;
	}
}

cp_bool Interpreter::equals_value(const Value* lval, const Value* rval) {
	if (lval->use_ref) {
		return lval == rval;
	}

	switch (lval->type) {
	case Type::T_VOID:
		return is_void(rval->type);
	case Type::T_BOOL:
		return lval->get_b() == rval->get_b();
	case Type::T_INT:
		return lval->get_i() == rval->get_i();
	case Type::T_FLOAT:
		return lval->get_f() == rval->get_f();
	case Type::T_CHAR:
		return lval->get_c() == rval->get_c();
	case Type::T_STRING:
		return lval->get_s() == rval->get_s();
	case Type::T_ARRAY:
		return equals_array(lval->get_arr(), rval->get_arr());
	case Type::T_STRUCT:
		return equals_struct(lval->get_str(), rval->get_str());
	}
	return false;
}

cp_bool Interpreter::equals_struct(const cp_struct& lstr, const cp_struct& rstr) {
	if (lstr.size() != rstr.size()) {
		return false;
	}

	for (auto& lval : lstr) {
		if (rstr.find(lval.first) == rstr.end()) {
			return false;
		}
		if (!equals_value(lval.second, rstr.at(lval.first))) {
			return false;
		}
	}

	return true;
}

cp_bool Interpreter::equals_array(const cp_array& larr, const cp_array& rarr) {
	if (larr.second != rarr.second) {
		return false;
	}

	for (size_t i = 0; i < larr.second; ++i) {
		if (!equals_value(larr.first[i], rarr.first[i])) {
			return false;
		}
	}

	return true;
}

InterpreterScope* Interpreter::get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_current_namespace()]) {
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
			throw std::runtime_error("something went wrong searching '" + identifier + "' variable");
		}
	}
	return scopes[nmspace][i];
}

InterpreterScope* Interpreter::get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			bool found = false;
			for (const auto& prgnmspace : program_nmspaces[get_current_namespace()]) {
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
			throw std::runtime_error("something went wrong searching '" + identifier + "' struct");
		}
	}
	return scopes[nmspace][i];
}

InterpreterScope* Interpreter::get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>* signature, bool strict) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_current_namespace()]) {
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
			throw std::runtime_error("something went wrong searching '" + identifier + "' fuction");
		}
	}
	return scopes[nmspace][i];
}

InterpreterScope* Interpreter::get_inner_most_functions_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function_name(identifier); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_current_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function_name(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' fuction");
		}
	}
	return scopes[nmspace][i];
}

Value* Interpreter::set_value(InterpreterScope* scope, const std::vector<parser::Identifier>& identifier_vector, Value* new_value) {
	auto var = scope->find_declared_variable(identifier_vector[0].identifier);

	if (identifier_vector.size() == 1 && identifier_vector[0].access_vector.size() == 0) {
		var->set_value(new_value);
		return var->get_value();
	}

	Value* before_value = nullptr;
	Value* value = var->get_value();
	size_t i = 0;

	while (i < identifier_vector.size()) {
		before_value = value;

		auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

		if (access_vector.size() > 0) {
			auto current_val = value->arr;
			size_t s = 0;
			size_t access_pos = 0;

			for (s = 0; s < access_vector.size() - 1; ++s) {
				access_pos = access_vector.at(s);

				// break if it is a string, and the string access will be handled in identifier node evaluation
				if (is_string(current_val->first[access_pos]->type)) {
					current_val->first[access_pos]->get_s()[access_vector.at(s + 1)] = new_value->get_c();
					return current_val->first[access_pos];
				}
				if (access_pos >= current_val->second) {
					throw std::runtime_error("invalid array position access");
				}
				current_val = current_val->first[access_pos]->arr;
			}
			if (is_string(value->type)) {
				value->get_s()[access_vector.at(s)] = new_value->get_c();
				return value;
			}
			access_pos = access_vector.at(s);
			if (i == identifier_vector.size() - 1) {
				current_val->first[access_pos] = new_value;
			}
			else {
				value = current_val->first[access_pos];
			}
		}

		++i;

		if (i < identifier_vector.size()) {
			if (is_void(value->type)) {
				std::stringstream ss;
				for (size_t si = 0; si <= i; ++si) {
					ss << identifier_vector[si].identifier;
					if (si < i) {
						ss << '.';
					}
				}
				throw std::runtime_error("cannot reach '" + ss.str() + "', previous '" + identifier_vector[i - 1].identifier + "' value is null");
			}

			if (i == identifier_vector.size() - 1 && identifier_vector[i].access_vector.size() == 0) {
				value->str->at(identifier_vector[i].identifier) = new_value;
			}
			else {
				value = value->get_str()[identifier_vector[i].identifier];
			}
		}
	}

	return value;
}

Value* Interpreter::access_value(const InterpreterScope* scope, Value* value, const std::vector<Identifier>& identifier_vector, size_t i) {
	Value* next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		auto current_Val = next_value->arr;
		size_t s = 0;
		size_t access_pos = 0;

		for (s = 0; s < access_vector.size() - 1; ++s) {
			access_pos = access_vector.at(s);
			// break if it is a string, and the string access will be handled in identifier node evaluation
			if (current_Val->first[access_pos]->type == Type::T_STRING) {
				has_string_access = true;
				break;
			}
			if (access_pos >= current_Val->second) {
				throw std::runtime_error("invalid array position access");
			}
			current_Val = current_Val->first[access_pos]->arr;
		}
		if (is_string(next_value->type)) {
			has_string_access = true;
			return next_value;
		}
		access_pos = access_vector.at(s);
		next_value = current_Val->first[access_pos];
	}

	++i;

	if (i < identifier_vector.size()) {
		if (is_void(next_value->type)) {
			std::stringstream ss;
			for (size_t si = 0; si <= i; ++si) {
				ss << identifier_vector[si].identifier;
				if (si < i) {
					ss << '.';
				}
			}
			throw std::runtime_error("cannot reach '" + ss.str() + "', previous '" + identifier_vector[i - 1].identifier + "' value is null");
		}

		next_value = next_value->get_str()[identifier_vector[i].identifier];

		if (identifier_vector[i].access_vector.size() > 0 || i < identifier_vector.size()) {
			return access_value(scope, next_value, identifier_vector, i);
		}
	}

	return next_value;
}

void Interpreter::check_build_array(Value* new_value, std::vector<ASTExprNode*> dim) {
	if (is_array(new_value->type) && dim.size() > 0) {
		auto arr = new_value->get_arr();

		cp_array* rarr = new cp_array();

		if (arr.second == 1) {
			*rarr = build_array(dim, arr.first[0], dim.size() - 1);
		}
		else if (arr.second == 0) {
			*rarr = build_undefined_array(dim, dim.size() - 1);
		}
		else {
			delete rarr;
			rarr = nullptr;
		}

		if (rarr) {
			new_value->set(*rarr, current_expression_array_type.type,
				current_expression_array_type.dim,
				current_expression_array_type.type_name,
				current_expression_array_type.type_name_space);
		}
	}
}

cp_array Interpreter::build_array(const std::vector<ASTExprNode*>& dim, Value* init_value, long long i) {
	Value** raw_arr;

	if (dim.size() - 1 == i) {
		current_expression_array_type = TypeDefinition();
	}

	size_t size = 0;
	if (dim.size() == 0) {
		size = 1;
	}
	else {
		auto crr_acc = dim[i];
		crr_acc->accept(this);
		size = current_expression_value->get_i();
	}

	raw_arr = new Value * [size];

	for (size_t j = 0; j < size; ++j) {
		auto val = new Value(init_value);

		if (is_undefined(current_expression_array_type.type) || is_array(current_expression_array_type.type)) {
			current_expression_array_type = *val;
		}

		raw_arr[j] = val;
	}

	auto arr = cp_array(raw_arr, size);

	--i;

	if (i >= 0) {
		size_t stay = dim.size() - i - 1;
		std::vector<ASTExprNode*> curr_arr_dim;
		size_t curr_dim_i = dim.size() - 1;
		for (size_t i = 0; i < stay; ++i) {
			curr_arr_dim.emplace(curr_arr_dim.begin(), dim.at(curr_dim_i));
			--curr_dim_i;
		}

		auto val = new Value(arr, current_expression_array_type.array_type, curr_arr_dim,
			current_expression_array_type.type_name, current_expression_array_type.type_name_space);

		return build_array(dim, val, i);
	}

	return arr;
}

cp_array Interpreter::build_undefined_array(const std::vector<ASTExprNode*>& dim, long long i) {
	Value** raw_arr;

	if (dim.size() - 1 == i) {
		current_expression_array_type = TypeDefinition();
	}

	size_t size = 0;
	if (dim.size() == 0) {
		size = 1;
	}
	else {
		auto crr_acc = dim[i];
		crr_acc->accept(this);
		size = current_expression_value->get_i();
	}

	raw_arr = new Value * [size] { nullptr };

	auto arr = cp_array(raw_arr, size);

	--i;

	if (i >= 0) {
		return build_undefined_array(dim, i);
	}

	return arr;
}

std::vector<unsigned int> Interpreter::calculate_array_dim_size(const cp_array& arr) {
	auto dim = std::vector<unsigned int>();

	dim.push_back(arr.second);

	if (is_array(arr.first[0]->type)) {
		auto dim2 = calculate_array_dim_size(arr.first[0]->get_arr());
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

std::vector<unsigned int> Interpreter::evaluate_access_vector(const std::vector<ASTExprNode*>& expr_access_vector) {
	auto access_vector = std::vector<unsigned int>();
	for (const auto& expr : expr_access_vector) {
		unsigned int val = 0;
		if (expr) {
			expr->accept(this);
			if (!is_int(current_expression_value->type)) {
				throw std::runtime_error("array index access must be a integer value");
			}
			val = current_expression_value->get_i();
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

std::string Interpreter::parse_value_to_string(const Value* value) {
	std::string str = "";
	if (print_level == 0) {
		printed.clear();
	}
	++print_level;
	switch (value->type) {
	case Type::T_VOID:
		str = "null";
		break;
	case Type::T_BOOL:
		str = ((value->get_b()) ? "true" : "false");
		break;
	case Type::T_INT:
		str = std::to_string(value->get_i());
		break;
	case Type::T_FLOAT:
		str = std::to_string(value->get_f());
		break;
	case Type::T_CHAR:
		str = cp_string(std::string{ value->get_c() });
		break;
	case Type::T_STRING:
		str = value->get_s();
		break;
	case Type::T_STRUCT: {
		if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
			std::stringstream s = std::stringstream();
			if (!value->type_name_space.empty()) {
				s << value->type_name_space << "::";
			}
			s << value->type_name;
			s << "<" << value << ">{...}";
			str = s.str();
		}
		else {
			printed.push_back(reinterpret_cast<uintptr_t>(value));
			str = parse_struct_to_string(value);
		}
		break;
	}
	case Type::T_ARRAY:
		str = parse_array_to_string(value->get_arr());
		break;
	case Type::T_FUNCTION: {
		auto funcs = get_inner_most_functions_scope(value->get_fun().first, value->get_fun().second)->find_declared_functions(value->get_fun().second);
		for (auto& it = funcs.first; it != funcs.second; ++it) {
			auto& func_name = it->first;
			auto& func_sig = std::get<0>(it->second);

			std::string func_decl = func_name + "(";
			for (auto param : func_sig) {
				func_decl += type_str(std::get<1>(param).type) + ", ";
			}
			if (func_sig.size() > 0) {
				func_decl.pop_back();
				func_decl.pop_back();
			}
			func_decl += ")";

			str = func_decl;
		}
		break;
	}
	case Type::T_UNDEFINED:
		throw std::runtime_error("undefined expression");
	default:
		throw std::runtime_error("can't determine value type on parsing");
	}
	--print_level;
	return str;
}

std::string Interpreter::parse_array_to_string(const cp_array& arr_value) {
	std::stringstream s = std::stringstream();
	s << "[";
	for (auto i = 0; i < arr_value.second; ++i) {
		s << parse_value_to_string(arr_value.first[i]);
		if (i < arr_value.second - 1) {
			s << ",";
		}
	}
	s << "]";
	return s.str();
}

std::string Interpreter::parse_struct_to_string(const Value* value) {
	auto str_value = value->get_str();
	std::stringstream s = std::stringstream();
	if (!value->type_name_space.empty()) {
		s << value->type_name_space << "::";
	}
	s << value->type_name << "<" << value << ">{";
	for (auto const& [key, val] : str_value) {
		if (key != modules::Module::INSTANCE_ID_NAME) {
			s << key + ":";
			s << parse_value_to_string(val);
			s << ",";
		}
	}
	if (s.str() != "{") {
		s.seekp(-1, std::ios_base::end);
	}
	s << "}";
	return s.str();
}

Value* Interpreter::do_operation(const std::string& op, Value* lval, Value* rval, bool is_expr, cp_int str_pos) {
	Type l_var_type = lval->ref ? lval->ref->type : lval->type;
	Type l_var_array_type = lval->ref ? lval->ref->array_type : lval->array_type;
	Type l_type = is_undefined(lval->type) ? l_var_type : lval->type;
	Type r_var_type = rval->ref ? rval->ref->type : rval->type;
	Type r_var_array_type = rval->ref ? rval->ref->array_type : rval->array_type;
	Type r_type = rval->type;
	Value* res_value = nullptr;

	if (is_void(r_type) && op == "=") {
		lval->set_null();
		return lval;
	}

	if (is_void(l_type) && op == "=") {
		lval->copy_from(rval);
		return lval;
	}

	if ((is_void(l_type) || is_void(r_type))
		&& Token::is_equality_op(op)) {
		return new Value((cp_bool)((op == "==") ?
			match_type(l_type, r_type)
			: !match_type(l_type, r_type)));
	}

	if (lval->use_ref
		&& Token::is_equality_op(op)) {
		return new Value((cp_bool)((op == "==") ?
			lval == rval
			: lval != rval));
	}

	switch (r_type) {
	case Type::T_BOOL: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_b());
			break;
		}

		if (!is_bool(l_type)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		if (op == "=") {
			lval->set(rval->get_b());
		}
		else if (op == "and") {
			res_value = new Value((cp_bool)(lval->get_b() && rval->get_b()));
		}
		else if (op == "or") {
			res_value = new Value((cp_bool)(lval->get_b() || rval->get_b()));
		}
		else if (op == "==") {
			res_value = new Value((cp_bool)(lval->get_b() == rval->get_b()));
		}
		else if (op == "!=") {
			res_value = new Value((cp_bool)(lval->get_b() != rval->get_b()));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_INT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_i());
			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& op == "<=>") {
			res_value = new Value((cp_int)(do_spaceship_operation(op, lval, rval)));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_relational_op(op)) {
			res_value = new Value(do_relational_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_equality_op(op)) {
			cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
			cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

			res_value = new Value((cp_bool)(op == "==" ?
				l == r : l != r));

			break;
		}

		if (is_float(l_type) && (is_any(l_var_type) || is_expr)) {
			lval->set(do_operation(lval->get_f(), cp_float(rval->get_i()), op));
		}
		else if (is_int(l_type) && is_any(l_var_type)
			&& (op == "/=" || op == "/%=" || op == "/" || op == "/%")) {
			lval->set(do_operation(cp_float(lval->get_i()), cp_float(rval->get_i()), op));
		}
		else if (is_int(l_type)) {
			lval->set(do_operation(lval->get_i(), rval->get_i(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_FLOAT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_f());
			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& op == "<=>") {
			res_value = new Value(do_spaceship_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_relational_op(op)) {
			lval->set(do_relational_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_equality_op(op)) {
			cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
			cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

			res_value = new Value((cp_bool)(op == "==" ?
				l == r : l != r));

			break;
		}

		if (is_float(l_type)) {
			lval->set(do_operation(lval->get_f(), rval->get_f(), op));
		}
		else if (is_int(l_type)) {
			lval->set(do_operation(cp_float(lval->get_i()), rval->get_f(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_CHAR: {
		if (is_any(l_var_type) && op == "=" && !has_string_access) {
			lval->set(rval->get_c());
			break;
		}

		if (is_expr
			&& is_char(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new Value((cp_bool)(op == "==" ?
				lval->get_c() == rval->get_c()
				: lval->get_c() != lval->get_c()));

			break;
		}

		if (is_string(l_type)) {
			if (has_string_access) {
				if (op != "=") {
					ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
				}
				has_string_access = false;
				lval->get_s()[str_pos] = rval->get_c();
				lval->set(lval->get_s());
			}
			else {
				lval->set(do_operation(lval->get_s(), std::string{ rval->get_c() }, op));
			}
		}
		else if (is_char(l_type)) {
			if (op != "=") {
				ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
			}

			lval->set(rval->get_c());
		}
		else if (is_any(l_var_type)) {
			if (op != "=") {
				ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
			}

			lval->set(rval->get_c());
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_STRING: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_s());
			break;
		}

		if (is_expr
			&& is_string(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new Value((cp_bool)(op == "==" ?
				lval->get_s() == rval->get_s()
				: lval->get_s() != rval->get_s()));

			break;
		}

		if (is_string(l_type)) {
			lval->set(do_operation(lval->get_s(), rval->get_s(), op));
		}
		else if (is_expr && is_char(l_type)) {
			lval->set(do_operation(cp_string{ lval->get_c() }, rval->get_s(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_ARRAY: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_arr(), lval->array_type, lval->dim, lval->type_name, lval->type_name_space);
			break;
		}

		if (is_expr
			&& is_array(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new Value((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval)));

			break;
		}

		if (!TypeDefinition::match_type_array(*lval, *rval, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		bool match_arr_t = lval->array_type == rval->array_type;
		if (!match_arr_t && !is_any(l_var_array_type)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(do_operation(lval->get_arr(), rval->get_arr(), op),
			match_arr_t ? lval->array_type : Type::T_ANY, lval->dim,
			lval->type_name, lval->type_name_space);

		break;
	}
	case Type::T_STRUCT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_str(), rval->type_name, lval->type_name_space);
			break;
		}

		if (is_expr
			&& is_struct(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new Value((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval)));

			break;
		}

		if (!is_struct(l_type) || op != "=") {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(rval->get_str(), rval->type_name, rval->type_name_space);

		break;
	}
	case Type::T_FUNCTION: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_str(), rval->type_name, rval->type_name_space);
			break;
		}

		if (!is_function(l_type) || op != "=") {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(rval->get_fun());

		break;
	}
	default:
		throw std::runtime_error("cannot determine type of operation");

	}

	if (!res_value) {
		res_value = lval;
	}

	return res_value;
}

cp_bool Interpreter::do_relational_operation(const std::string& op, Value* lval, Value* rval) {
	cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
	cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

	if (op == "<") {
		return l < r;
	}
	else if (op == ">") {
		return l > r;
	}
	else if (op == "<=") {
		return l <= r;
	}
	else if (op == ">=") {
		return l >= r;
	}
	ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
}

cp_int Interpreter::do_spaceship_operation(const std::string& op, Value* lval, Value* rval) {
	cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
	cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

	auto res = l <=> r;
	if (res == std::strong_ordering::less) {
		return cp_int(-1);
	}
	else if (res == std::strong_ordering::equal) {
		return cp_int(0);
	}
	else if (res == std::strong_ordering::greater) {
		return cp_int(1);
	}
}

cp_int Interpreter::do_operation(cp_int lval, cp_int rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (rval == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (rval == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return lval % rval;
	}
	else if (op == "/%=" || op == "/%") {
		if (rval == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return cp_int(std::floor(lval / rval));
	}
	else if (op == "**=" || op == "**") {
		return cp_int(std::pow(lval, rval));
	}
	else if (op == ">>=" || op == ">>") {
		return lval >> rval;
	}
	else if (op == "<<=" || op == "<<") {
		return lval << rval;
	}
	else if (op == "|=" || op == "|") {
		return lval | rval;
	}
	else if (op == "&=" || op == "&") {
		return lval & rval;
	}
	else if (op == "^=" || op == "^") {
		return lval ^ rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'int' and 'int'");
}

cp_float Interpreter::do_operation(cp_float lval, cp_float rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (int(rval) == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (int(rval) == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return std::fmod(lval, rval);
	}
	else if (op == "/%=" || op == "/%") {
		if (int(rval) == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return std::floor(lval / rval);
	}
	else if (op == "**=" || op == "**") {
		return cp_int(std::pow(lval, rval));
	}
	throw std::runtime_error("invalid '" + op + "' operator");
}

cp_string Interpreter::do_operation(cp_string lval, cp_string rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'string' and 'string'");
}

cp_array Interpreter::do_operation(cp_array lval, cp_array rval, const std::string& op) {
	if (op == "=") {
		Value** result = new Value * [rval.second];
		for (size_t i = 0; i < rval.second; ++i) {
			result[i] = rval.first[i];
		}
		return cp_array(result, rval.second);
	}
	else if (op == "+=" || op == "+") {
		auto size = lval.second + rval.second;
		Value** result = new Value * [size];

		std::sort(lval.first, lval.first + lval.second, [](const Value* a, const Value* b) {
			return a->value_hash() < b->value_hash();
			});

		std::sort(rval.first, rval.first + rval.second, [](const Value* a, const Value* b) {
			return a->value_hash() < b->value_hash();
			});

		std::merge(lval.first, lval.first + lval.second, rval.first, rval.first + rval.second, result, [](const Value* a, const Value* b) {
			return a->value_hash() < b->value_hash();
			});

		return cp_array(result, size);
	}

	throw std::runtime_error("invalid '" + op + "' operator for types 'array' and 'array'");
}

void Interpreter::normalize_type(Variable* var, Value* val) {
	if (is_string(var->type) && is_char(val->type)) {
		val->type = var->type;
		val->set(cp_string{ val->get_c() });
	}
	else if (is_float(var->type) && is_int(val->type)) {
		val->type = var->type;
		val->set(cp_float(val->get_i()));
	}
}

long long Interpreter::hash(ASTExprNode* astnode) {
	astnode->accept(this);
	switch (current_expression_value->type) {
	case Type::T_BOOL:
		return static_cast<long long>(current_expression_value->get_b());
	case Type::T_INT:
		return static_cast<long long>(current_expression_value->get_i());
	case Type::T_FLOAT:
		return static_cast<long long>(current_expression_value->get_f());
	case Type::T_CHAR:
		return static_cast<long long>(current_expression_value->get_c());
	case Type::T_STRING:
		return axe::StringUtils::hashcode(current_expression_value->get_s());
	default:
		throw std::runtime_error("cannot determine type");
	}
}

long long Interpreter::hash(ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(ASTLiteralNode<cp_int>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(ASTLiteralNode<cp_float>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(ASTLiteralNode<cp_char>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(ASTLiteralNode<cp_string>* astnode) {
	return axe::StringUtils::hashcode(astnode->val);
}

long long Interpreter::hash(ASTIdentifierNode* astnode) {
	std::string nmspace = get_namespace(astnode->nmspace);

	InterpreterScope* id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	Variable* variable = id_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	auto value = access_value(id_scope, variable->get_value(), astnode->identifier_vector);

	switch (value->type) {
	case Type::T_BOOL:
		return static_cast<long long>(value->get_b());
	case Type::T_INT:
		return static_cast<long long>(value->get_i());
	case Type::T_FLOAT:
		return static_cast<long long>(value->get_f());
	case Type::T_CHAR:
		return static_cast<long long>(value->get_c());
	case Type::T_STRING:
		return axe::StringUtils::hashcode(value->get_s());
	default:
		throw std::runtime_error("cannot determine type");
	}
}

void Interpreter::declare_function_block_parameters(const std::string& nmspace) {
	auto curr_scope = scopes[nmspace].back();
	auto rest_name = std::string();
	auto vec = std::vector<Value*>();
	size_t i;

	if (current_function_calling_arguments.size() == 0 || current_function_defined_parameters.size() == 0) {
		return;
	}

	// adds function arguments
	for (i = 0; i < current_function_calling_arguments.top().size(); ++i) {
		auto current_function_calling_argument = current_function_calling_arguments.top()[i];

		if (current_function_defined_parameters.top().size() > i
			&& is_string(std::get<1>(current_function_defined_parameters.top()[i]).type) && is_char(current_function_calling_argument->type)) {
			if (current_function_calling_argument->use_ref
				&& current_function_calling_argument->ref
				&& !is_any(current_function_calling_argument->ref->type)) {
				throw std::runtime_error("cannot reference char to string in function call");
			}
			current_function_calling_argument->type = std::get<1>(current_function_defined_parameters.top()[i]).type;
			current_function_calling_argument->set(cp_string{ current_function_calling_argument->get_c() });
		}
		else if (current_function_defined_parameters.top().size() > i
			&& is_float(std::get<1>(current_function_defined_parameters.top()[i]).type) && is_int(current_function_calling_argument->type)) {
			if (current_function_calling_argument->use_ref
				&& current_function_calling_argument->ref
				&& !is_any(current_function_calling_argument->ref->type)) {
				throw std::runtime_error("cannot reference int to float in function call");
			}
			current_function_calling_argument->type = std::get<1>(current_function_defined_parameters.top()[i]).type;
			current_function_calling_argument->set(cp_float(current_function_calling_argument->get_i()));
		}

		// is reference : not reference
		Value* current_value = nullptr;
		if (current_function_calling_argument->use_ref) {
			current_value = current_function_calling_argument;
		}
		else {
			current_value = new Value(current_function_calling_argument);
			current_value->ref = nullptr;
		}

		if (i >= current_function_defined_parameters.top().size()) {
			vec.push_back(current_value);
		}
		else {
			const auto& pname = std::get<0>(current_function_defined_parameters.top()[i]);

			if (is_function(current_function_calling_argument->type)) {
				auto funcs = get_inner_most_functions_scope(current_function_calling_argument->get_fun().first,
					current_function_calling_argument->get_fun().second)->find_declared_functions(current_function_calling_argument->get_fun().second);
				for (auto& it = funcs.first; it != funcs.second; ++it) {
					auto& func_params = std::get<0>(it->second);
					auto& func_block = std::get<1>(it->second);
					curr_scope->declare_function(pname, func_params, func_block, std::get<2>(it->second));
				}
			}
			else {
				if (current_value->use_ref && current_value->ref) {
					curr_scope->declare_variable(pname, current_value->ref);
				}
				else {
					curr_scope->declare_variable(pname, new Variable(current_value));
				}
			}

			// is rest
			if (std::get<3>(current_function_defined_parameters.top()[i])) {
				rest_name = pname;
				// if is last parameter and is array
				if (current_function_defined_parameters.top().size() - 1 == i
					&& is_array(current_value->type)) {
					for (size_t i = 0; i < vec.size(); ++i) {
						vec.push_back(current_value->get_arr().first[i]);
					}
				}
				else {
					vec.push_back(current_value);
				}
			}
		}
	}

	// adds default values
	for (; i < current_function_defined_parameters.top().size(); ++i) {
		if (std::get<3>(current_function_defined_parameters.top()[i])) {
			break;
		}

		const auto& pname = std::get<0>(current_function_defined_parameters.top()[i]);
		std::get<2>(current_function_defined_parameters.top()[i])->accept(this);

		if (is_function(current_expression_value->type)) {
			auto funcs = get_inner_most_functions_scope(current_expression_value->get_fun().first,
				current_expression_value->get_fun().second)->find_declared_functions(current_expression_value->get_fun().second);
			for (auto& it = funcs.first; it != funcs.second; ++it) {
				auto& func_params = std::get<0>(it->second);
				auto& func_block = std::get<1>(it->second);
				curr_scope->declare_function(pname, func_params, func_block, std::get<2>(it->second));
			}
		}
		else {
			curr_scope->declare_variable(pname, new Variable(new Value(current_expression_value)));
		}
	}

	if (vec.size() > 0) {
		auto rest = new Value(Type::T_ANY, std::vector<ASTExprNode*>());
		auto rarr = new Value * [vec.size()];
		for (size_t i = 0; i < vec.size(); ++i) {
			rarr[i] = vec[i];
		}
		auto arr = cp_array(rarr, vec.size());
		rest->set(arr, Type::T_ANY, std::vector<ASTExprNode*>());
		curr_scope->declare_variable(rest_name, new Variable(new Value(rest)));
	}

	current_function_defined_parameters.pop();
	current_function_calling_arguments.pop();
}

void Interpreter::call_builtin_function(const std::string& identifier) {
	auto vec = std::vector<Value*>();

	for (size_t i = 0; i < current_function_calling_arguments.top().size(); ++i) {
		// is reference : not reference
		Value* current_value = nullptr;
		if (current_function_calling_arguments.top()[i]->use_ref) {
			current_value = current_function_calling_arguments.top()[i];
		}
		else {
			current_value = new Value(current_function_calling_arguments.top()[i]);
			current_value->ref = nullptr;
		}

		if (i >= current_function_defined_parameters.top().size()) {
			vec.push_back(current_value);
		}
		else {
			if (std::get<3>(current_function_defined_parameters.top()[i])) {
				vec.push_back(current_value);
			}
			else {
				builtin_arguments.push_back(current_value);
			}
		}
	}

	if (vec.size() > 0) {
		auto rest = new Value(Type::T_ANY, std::vector<ASTExprNode*>());
		auto rarr = new Value * [vec.size()];
		for (size_t i = 0; i < vec.size(); ++i) {
			rarr[i] = vec[i];
		}
		auto arr = cp_array(rarr, vec.size());
		rest->set(arr, Type::T_ANY, std::vector<ASTExprNode*>());
		builtin_arguments.push_back(rest);
	}

	current_function_defined_parameters.pop();
	current_function_calling_arguments.pop();

	builtin_functions[identifier]();
	builtin_arguments.clear();
}

void Interpreter::register_built_in_functions() {
	interpreter_parameter_list_t params;

	builtin_functions["print"] = [this]() {
		current_expression_value = new Value(Type::T_UNDEFINED);
		if (builtin_arguments.size() == 0) {
			return;
		}
		for (size_t i = 0; i < builtin_arguments[0]->get_arr().second; ++i) {
			std::cout << parse_value_to_string(builtin_arguments[0]->get_arr().first[i]);
		}
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_function("print", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));

	builtin_functions["println"] = [this]() {
		builtin_functions["print"]();
		std::cout << std::endl;
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_function("println", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));


	builtin_functions["read"] = [this]() {
		if (builtin_arguments.size() > 0) {
			builtin_functions["print"]();
		}
		std::string line;
		std::getline(std::cin, line);
		current_expression_value = new Value(Type::T_STRING);
		current_expression_value->set(cp_string(std::move(line)));
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_function("read", params, nullptr, TypeDefinition::get_basic(Type::T_STRING));


	builtin_functions["readch"] = [this]() {
		while (!_kbhit());
		char ch = _getch();
		current_expression_value = new Value(Type::T_CHAR);
		current_expression_value->set(cp_char(ch));
		};
	params.clear();
	scopes[default_namespace].back()->declare_function("readch", params, nullptr, TypeDefinition::get_basic(Type::T_CHAR));


	builtin_functions["len"] = [this]() {
		auto& curr_val = builtin_arguments[0];
		auto val = new Value(Type::T_INT);

		if (is_array(curr_val->type)) {
			val->set(cp_int(curr_val->get_arr().second));
		}
		else {
			val->set(cp_int(curr_val->get_s().size()));
		}

		current_expression_value = val;
		};
	params.clear();
	params.push_back(std::make_tuple("arr", TypeDefinition::get_array(Type::T_ANY), nullptr, false));
	scopes[default_namespace].back()->declare_function("len", params, nullptr, TypeDefinition::get_basic(Type::T_INT));
	params.clear();
	params.push_back(std::make_tuple("str", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	scopes[default_namespace].back()->declare_function("len", params, nullptr, TypeDefinition::get_basic(Type::T_INT));


	builtin_functions["equals"] = [this]() {
		auto& rval = builtin_arguments[0];
		auto& lval = builtin_arguments[1];
		auto res = new Value(Type::T_BOOL);

		res->set(equals_struct(rval->get_str(), lval->get_str()));

		current_expression_value = res;
		};
	params.clear();
	params.push_back(std::make_tuple("lval", TypeDefinition::get_basic(Type::T_ANY), nullptr, false));
	params.push_back(std::make_tuple("rval", TypeDefinition::get_basic(Type::T_ANY), nullptr, false));
	scopes[default_namespace].back()->declare_function("equals", params, nullptr, TypeDefinition::get_basic(Type::T_BOOL));


	builtin_functions["system"] = [this]() {
		current_expression_value = new Value(Type::T_UNDEFINED);
		system(builtin_arguments[0]->get_s().c_str());
		};
	params.clear();
	params.push_back(std::make_tuple("cmd", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	scopes[default_namespace].back()->declare_function("system", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));
}

void Interpreter::register_built_in_lib(const std::string& libname) {
	if (built_in_libs[0] == libname) {
		cpgraphics = std::unique_ptr<modules::Graphics>(new modules::Graphics());
		cpgraphics->register_functions(this);
	}

	if (built_in_libs[1] == libname) {
		cpfiles = std::unique_ptr<modules::Files>(new modules::Files());
		cpfiles->register_functions(this);
	}

	if (built_in_libs[2] == libname) {
		cpconsole = std::unique_ptr<modules::Console>(new modules::Console());
		cpconsole->register_functions(this);
	}
}

const std::string& Interpreter::get_namespace(const std::string& nmspace) const {
	return get_namespace(current_program, nmspace);
}

const std::string& Interpreter::get_namespace(const ASTProgramNode* program, const std::string& nmspace) const {
	return nmspace.empty() ? (
		current_function_nmspace.size() == 0 ? (
			program->alias.empty() ? default_namespace : program->alias
			) : current_function_nmspace.top()
		) : nmspace;
}

const std::string& Interpreter::get_current_namespace() {
	return current_function_nmspace.size() == 0 ? (
		current_program->alias.empty() ? default_namespace : current_program->alias
		) : current_function_nmspace.top();
}

void Interpreter::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Interpreter::msg_header() {
	return "(IERR) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
