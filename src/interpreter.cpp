#include <iostream>
#include <algorithm> 
#include <cmath>
#include <compare>
#include <functional>

#include "interpreter.hpp"
#include "exception_handler.hpp"
#include "token.hpp"
#include "builtin.hpp"
#include "garbage_collector.hpp"

#include "vendor/axeutils.hpp"
#include "vendor/axewatch.hpp"
#include "vendor/axeuuid.hpp"

using namespace lexer;

Interpreter::Interpreter(std::shared_ptr<Scope> global_scope, std::shared_ptr<ASTProgramNode> main_program,
	const std::map<std::string, std::shared_ptr<ASTProgramNode>>& programs, const std::vector<std::string>& args)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	current_expression_value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));
	gc.add_ptr_root(&current_expression_value);

	push_namespace(default_namespace);
	scopes[default_namespace].push_back(global_scope);

	built_in_libs["builtin"]->register_functions(this);

	build_args(args);
}

void Interpreter::start() {
	//auto pop = push_namespace(default_namespace);
	current_this_name.push(get_namespace());
	visit(current_program.top());
	current_this_name.pop();
	//pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTProgramNode> astnode) {
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
		&& !std::dynamic_pointer_cast<ASTExprNode>(astnode->statements[0])) {
		current_expression_value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));
	}
}

void Interpreter::visit(std::shared_ptr<ASTUsingNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (built_in_libs.find(libname) != built_in_libs.end()) {
		built_in_libs.find(libname)->second->register_functions(this);
	}

	auto program = programs[libname];

	// add lib to current program
	current_program.top()->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::CollectionUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);

		current_program.push(program);
		auto pop = push_namespace(program->alias);
		auto nmspace = get_namespace();
		if (!program->alias.empty()) {
			scopes[program->alias].push_back(std::make_shared<Scope>());
			if (std::find(program_nmspaces[nmspace].begin(), program_nmspaces[nmspace].end(), default_namespace) == program_nmspaces[nmspace].end()) {
				program_nmspaces[nmspace].push_back(default_namespace);
			}
		}
		current_this_name.push(nmspace);

		start();

		current_program.pop();
		current_this_name.pop();
		pop_namespace(pop);
	}
}

void Interpreter::visit(std::shared_ptr<ASTNamespaceManagerNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	if (astnode->image == "include") {
		if (std::find(program_nmspaces[nmspace].begin(), program_nmspaces[nmspace].end(), astnode->nmspace) == program_nmspaces[nmspace].end()) {
			program_nmspaces[nmspace].push_back(astnode->nmspace);
		}
	}
	else {
		size_t pos = std::distance(program_nmspaces[nmspace].begin(), std::find(program_nmspaces[nmspace].begin(), program_nmspaces[nmspace].end(), astnode->nmspace));
		program_nmspaces[nmspace].erase(program_nmspaces[nmspace].begin() + pos);
	}
}

void Interpreter::visit(std::shared_ptr<ASTEnumNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		auto var = std::make_shared<RuntimeVariable>(astnode->identifiers[i], Type::T_INT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "");
		gc.add_var_root(var);
		var->set_value(alocate_value(new RuntimeValue(cp_int(i))));
		scopes[nmspace].back()->declare_variable(astnode->identifiers[i], var);
	}
}

void Interpreter::visit(std::shared_ptr<ASTDeclarationNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	const auto& nmspace = get_namespace();

	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		current_expression_value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));
	}

	RuntimeValue* new_value;

	if (current_expression_value->use_ref) {
		new_value = current_expression_value;
	}
	else {
		new_value = alocate_value(new RuntimeValue(current_expression_value));
	}

	auto& astnode_type_name = astnode->type_name.empty() ? new_value->type_name : astnode->type_name;

	auto new_var = std::make_shared<RuntimeVariable>(astnode->identifier, astnode->type,
		astnode->array_type, astnode->dim,
		astnode_type_name, astnode->type_name_space);
	gc.add_var_root(new_var);
	new_var->set_value(new_value);

	if ((!TypeDefinition::is_any_or_match_type(*new_var, *new_value, evaluate_access_vector_ptr) ||
		is_array(new_var->type) && !is_any(new_var->array_type)
		&& !TypeDefinition::match_type(*new_var, *new_value, evaluate_access_vector_ptr, false, true))
		&& astnode->expr && !is_undefined(new_value->type)) {
		ExceptionHandler::throw_declaration_type_err(astnode->identifier, *new_var, *new_value, evaluate_access_vector_ptr);
	}

	check_build_array(new_value, astnode->dim);

	normalize_type(new_var, new_value);

	scopes[nmspace].back()->declare_variable(astnode->identifier, new_var);
}

void Interpreter::visit(std::shared_ptr<ASTUnpackedDeclarationNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::shared_ptr<ASTIdentifierNode> var = nullptr;
	if (astnode->expr) {
		var = std::dynamic_pointer_cast<ASTIdentifierNode>(astnode->expr);
	}

	for (auto& declaration : astnode->declarations) {
		if (var) {
			auto ids = var->identifier_vector;
			ids.push_back(Identifier(declaration->identifier));
			auto access_expr = std::make_shared<ASTIdentifierNode>(ids, var->nmspace, declaration->row, declaration->col);
			declaration->expr = access_expr;
		}

		declaration->accept(this);

		if (var) {
			declaration->expr = nullptr;
		}
	}
}

void Interpreter::visit(std::shared_ptr<ASTAssignmentNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto pop = push_namespace(astnode->nmspace);

	std::shared_ptr<Scope> astscope;
	try {
		auto nmspace = get_namespace();
		astscope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	std::shared_ptr<RuntimeVariable> variable = std::dynamic_pointer_cast<RuntimeVariable>(astscope->find_declared_variable(astnode->identifier));
	RuntimeValue* value = access_value(astscope, variable->get_value(), astnode->identifier_vector);

	astnode->expr->accept(this);

	auto ptr_value = current_expression_value;
	RuntimeValue* new_value = nullptr;
	if (ptr_value->use_ref) {
		new_value = ptr_value;
	}
	else {
		new_value = alocate_value(new RuntimeValue(ptr_value));
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
		if (!TypeDefinition::is_any_or_match_type(*variable, *ptr_value, evaluate_access_vector_ptr) ||
			is_array(variable->type) && !is_any(variable->array_type)
			&& !TypeDefinition::match_type(*variable, *ptr_value, evaluate_access_vector_ptr, false, true)) {
			ExceptionHandler::throw_mismatched_type_err(*variable, *ptr_value, evaluate_access_vector_ptr);
		}

		normalize_type(variable, new_value);

		variable->set_value(new_value);
	}
	else {
		if (astnode->identifier_vector.size() == 1 && astnode->identifier_vector[0].access_vector.size() == 0) {
			variable->set_value(alocate_value(new RuntimeValue(variable->get_value())));
			value = variable->get_value();
		}

		cp_int pos = 0;
		if (has_string_access) {
			std::dynamic_pointer_cast<ASTExprNode>(astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1])->accept(this);
			pos = current_expression_value->get_i();
		}

		if (astnode->op == "=") {
			if (is_string(variable->type) && is_char(new_value->type)
				&& new_value->use_ref && new_value->ref.lock() && !is_any(new_value->ref.lock()->type)) {
				throw std::runtime_error("cannot reference char to string in function call");
			}
			else if (is_float(variable->type) && is_int(new_value->type)
				&& new_value->use_ref && new_value->ref.lock() && !is_any(new_value->ref.lock()->type)) {
				throw std::runtime_error("cannot reference int to float in function call");
			}
			set_value(astscope, astnode->identifier_vector, new_value);
		}
		else {
			normalize_type(variable, new_value);
			do_operation(astnode->op, value, new_value, false, pos);
		}
	}
	pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTReturnNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto nmspace = get_namespace();

	if (astnode->expr) {
		TypeDefinition curr_func_ret_type = current_function.top();
		std::shared_ptr<Scope> func_scope = get_inner_most_function_scope(nmspace, current_function_call_identifier_vector.top()[0].identifier, &current_function_signature.top(), evaluate_access_vector_ptr);

		astnode->expr->accept(this);
		RuntimeValue* returned_value = current_expression_value;
		RuntimeValue* value = access_value(func_scope, current_expression_value, current_function_call_identifier_vector.top());

		if (value->type == Type::T_STRING && current_function_call_identifier_vector.top().back().access_vector.size() > 0 && has_string_access) {
			has_string_access = false;
			std::string str = value->get_s();
			std::dynamic_pointer_cast<ASTExprNode>(current_function_call_identifier_vector.top().back().access_vector[current_function_call_identifier_vector.top().back().access_vector.size() - 1])->accept(this);
			auto pos = value->get_i();

			auto char_value = alocate_value(new RuntimeValue(Type::T_CHAR));
			char_value->set(cp_char(str[pos]));
			value = char_value;
		}

		if (!TypeDefinition::is_any_or_match_type(curr_func_ret_type, *returned_value, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_return_type_err(current_function.top().identifier,
				curr_func_ret_type, *returned_value, evaluate_access_vector_ptr);
		}

		if (value->use_ref) {
			current_expression_value = value;
		}
		else {
			current_expression_value = alocate_value(new RuntimeValue(value));
		}
	}
	else {
		current_expression_value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));
	}

	for (long long i = scopes[nmspace].size() - 1; i >= 0; --i) {
		if (!scopes[nmspace][i]->name.empty()) {
			return_from_function_name = scopes[nmspace][i]->name;
			return_from_function = true;
			break;
		}
	}
}

void Interpreter::visit(std::shared_ptr<ASTFunctionCallNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto pop = push_namespace(astnode->nmspace);
	std::string nmspace = get_namespace();
	std::string identifier = astnode->identifier;
	std::vector<Identifier> identifier_vector = astnode->identifier_vector;
	bool strict = true;
	std::vector<TypeDefinition*> signature;
	std::vector<RuntimeValue*> function_arguments;

	if (identifier == "create_collection") {
		int x = 0;
	}

	gc.add_root_container(&function_arguments);

	for (auto& param : astnode->parameters) {
		param->accept(this);

		RuntimeValue* pvalue = nullptr;
		if (current_expression_value->use_ref) {
			pvalue = current_expression_value;
		}
		else {
			pvalue = alocate_value(new RuntimeValue(current_expression_value));
		}

		function_arguments.push_back(pvalue);
		signature.push_back(pvalue);
	}

	std::shared_ptr<Scope> func_scope;
	try {
		func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, evaluate_access_vector_ptr, strict);
	}
	catch (...) {
		try {
			strict = false;
			func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, evaluate_access_vector_ptr, strict);
		}
		catch (...) {
			try {
				auto var_scope = get_inner_most_variable_scope(nmspace, identifier);
				auto var = std::dynamic_pointer_cast<RuntimeVariable>(var_scope->find_declared_variable(identifier));
				nmspace = var->value->get_fun().first;
				identifier = var->value->get_fun().second;
				identifier_vector = std::vector<Identifier>{ Identifier(identifier) };
				func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, evaluate_access_vector_ptr, strict);
			}
			catch (...) {
				std::string func_name = ExceptionHandler::buid_signature(identifier, signature, evaluate_access_vector_ptr);
				throw std::runtime_error("function '" + func_name + "' was never declared");
			}
		}
	}

	auto& declfun = func_scope->find_declared_function(identifier, &signature, evaluate_access_vector_ptr, strict);

	if (!pop) {
		pop = push_namespace(nmspace);
	}

	current_function.push(declfun);
	current_function_defined_parameters.push(declfun.parameters);
	current_this_name.push(identifier);
	current_function_signature.push(signature);
	current_function_call_identifier_vector.push(identifier_vector);
	current_function_calling_arguments.push(function_arguments);

	if (!declfun.block) {
		throw std::runtime_error("'" + astnode->identifier + "' function definition not found");
	}

	function_call_name = identifier;
	declfun.block->accept(this);

	current_function.pop();
	current_function_call_identifier_vector.pop();
	current_function_signature.pop();
	current_this_name.pop();
	gc.remove_root_container(&function_arguments);

	pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTBuiltinFunctionExecuterNode> astnode) {
	std::string nmspace = get_namespace();

	std::shared_ptr<Scope> func_scope = get_inner_most_function_scope(nmspace, current_function_call_identifier_vector.top()[0].identifier, &current_function_signature.top(), evaluate_access_vector_ptr);

	builtin_functions[astnode->identifier]();

	current_expression_value = access_value(func_scope, current_expression_value, current_function_call_identifier_vector.top());

}

void Interpreter::visit(std::shared_ptr<ASTFunctionDefinitionNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto pop = push_namespace(astnode->type_name_space);
	const auto& nmspace = get_namespace();

	try {
		std::shared_ptr<Scope> func_scope = scopes[nmspace].back();
		auto& declfun = func_scope->find_declared_function(astnode->identifier, &astnode->parameters, evaluate_access_vector_ptr, true);
		declfun.block = astnode->block;
	}
	catch (...) {
		auto& block = astnode->block;
		if (!block && builtin_functions.find(astnode->identifier) != builtin_functions.end()) {
			block = std::make_shared<ASTBlockNode>(std::vector<std::shared_ptr<ASTNode>>{
				std::make_shared<ASTBuiltinFunctionExecuterNode>(astnode->identifier, astnode->row, astnode->col)
			}, astnode->row, astnode->col);
		}

		auto f = FunctionDefinition(astnode->identifier, astnode->type, astnode->type_name, astnode->type_name_space,
			astnode->array_type, astnode->dim, astnode->parameters, block, astnode->row, astnode->row);
		scopes[nmspace].back()->declare_function(astnode->identifier, f);
	}
	pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTFunctionExpression> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	auto& fun = astnode->fun;

	fun->identifier = axe::UUID::generate();

	fun->accept(this);

	auto value = alocate_value(new RuntimeValue(Type::T_FUNCTION));
	value->set(cp_function(nmspace, fun->identifier));
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTBlockNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	scopes[nmspace].push_back(std::make_shared<Scope>(function_call_name));
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
			if (!return_from_function_name.empty() && return_from_function_name == scopes[nmspace].back()->name) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	scopes[nmspace].pop_back();
	gc.collect();
}

void Interpreter::visit(std::shared_ptr<ASTExitNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->exit_code->accept(this);
	if (!is_int(current_expression_value->type)) {
		throw std::runtime_error("expected int value");
	}
	exit_from_program = true;
}

void Interpreter::visit(std::shared_ptr<ASTContinueNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	continue_block = true;
}

void Interpreter::visit(std::shared_ptr<ASTBreakNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	break_block = true;
}

void Interpreter::visit(std::shared_ptr<ASTSwitchNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_switch;

	scopes[nmspace].push_back(std::make_shared<Scope>());

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

	long long pos;
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
			if (!return_from_function_name.empty() && return_from_function_name == scopes[nmspace].back()->name) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	scopes[nmspace].pop_back();
	--is_switch;
	gc.collect();
}

void Interpreter::visit(std::shared_ptr<ASTElseIfNode> astnode) {
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

void Interpreter::visit(std::shared_ptr<ASTIfNode> astnode) {
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

void Interpreter::visit(std::shared_ptr<ASTForNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_loop;
	scopes[nmspace].push_back(std::make_shared<Scope>());

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
		current_expression_value = alocate_value(new RuntimeValue(Type::T_BOOL));
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
			current_expression_value = alocate_value(new RuntimeValue(Type::T_BOOL));
			current_expression_value->set(true);
		}

		result = current_expression_value->get_b();
	}

	scopes[nmspace].pop_back();
	--is_loop;
	gc.collect();
}

void Interpreter::visit(std::shared_ptr<ASTForEachNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	++is_loop;

	astnode->collection->accept(this);

	scopes[nmspace].push_back(std::make_shared<Scope>());

	auto itdecl = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->itdecl);

	switch (current_expression_value->type) {
	case Type::T_ARRAY: {
		const auto& colletion = current_expression_value->get_arr();
		for (size_t i = 0; i < colletion.size(); ++i) {
			auto val = colletion[i];

			auto exnode = std::make_shared<ASTValueNode>(val, astnode->row, astnode->col);
			itdecl->expr = exnode;
			itdecl->accept(this);
			itdecl->expr = nullptr;

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

			auto exnode = std::make_shared<ASTValueNode>(alocate_value(new RuntimeValue(cp_char(val))), astnode->row, astnode->col);
			itdecl->expr = exnode;
			itdecl->accept(this);
			itdecl->expr = nullptr;

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
				if (!is_any(current_expression_value->ref.lock()->type)
					&& current_expression_value->ref.lock()->type_name_space != "cp"
					&& current_expression_value->ref.lock()->type_name != "Pair") {
					throw std::runtime_error("invalid struct '"
						+ current_expression_value->ref.lock()->type_name
						+ "' declaration in foreach loop");
				}

				std::map<std::string, std::shared_ptr<ASTExprNode>> values = {
					{ "key", std::make_shared<ASTLiteralNode<cp_string>>(cp_string(val.first), astnode->row, astnode->col) },
					{ "value", std::make_shared<ASTValueNode>(val.second, astnode->row, astnode->col) }
				};
				auto exnode = std::make_shared<ASTStructConstructorNode>("Pair", "cp", values, astnode->row, astnode->col);
				itdecl->expr = exnode;
				itdecl->accept(this);
				itdecl->expr = nullptr;

			}
			else if (const auto idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->itdecl)) {
				if (idnode->declarations.size() != 2) {
					throw std::runtime_error("invalid number of values");
				}

				auto key_node = std::make_shared<ASTLiteralNode<cp_string>>(cp_string(val.first), astnode->row, astnode->col);
				idnode->declarations[0]->expr = key_node;
				auto val_node = std::make_shared<ASTValueNode>(val.second, astnode->row, astnode->col);
				idnode->declarations[1]->expr = val_node;
				idnode->accept(this);
				idnode->declarations[0]->expr = nullptr;
				idnode->declarations[1]->expr = nullptr;
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
	gc.collect();
}

void Interpreter::visit(std::shared_ptr<ASTTryCatchNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	try {
		scopes[nmspace].push_back(std::make_shared<Scope>());

		astnode->try_block->accept(this);

		scopes[nmspace].pop_back();
		gc.collect();
	}
	catch (std::exception ex) {
		scopes[nmspace].pop_back();
		gc.collect();

		scopes[nmspace].push_back(std::make_shared<Scope>());

		if (const auto idnode = std::dynamic_pointer_cast<ASTUnpackedDeclarationNode>(astnode->decl)) {
			if (idnode->declarations.size() != 1) {
				throw std::runtime_error("invalid number of values");
			}
			auto exnode = std::make_shared<ASTLiteralNode<cp_string>>(ex.what(), astnode->row, astnode->col);
			idnode->declarations[0]->expr = exnode;
			idnode->accept(this);
			idnode->declarations[0]->expr = nullptr;
		}
		else if (const auto idnode = std::dynamic_pointer_cast<ASTDeclarationNode>(astnode->decl)) {
			std::map<std::string, std::shared_ptr<ASTExprNode>> values = {
				{ "error", std::make_shared<ASTLiteralNode<cp_string>>(ex.what(), astnode->row, astnode->col) }
			};
			auto exnode = std::make_shared<ASTStructConstructorNode>("Exception", "cp", values, astnode->row, astnode->col);
			idnode->expr = exnode;
			idnode->expr = nullptr;
		}
		else if (!std::dynamic_pointer_cast<ASTReticencesNode>(astnode->decl)) {
			throw std::runtime_error("expected declaration");
		}

		astnode->catch_block->accept(this);
		scopes[nmspace].pop_back();
		gc.collect();
	}
}

void Interpreter::visit(std::shared_ptr<ASTThrowNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->error->accept(this);

	if (is_struct(current_expression_value->type)
		&& current_expression_value->type_name == "Exception") {
		try {
			std::string nmspace = "cp";
			get_inner_most_struct_definition_scope(nmspace, "Exception");
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
		throw std::runtime_error("expected cp::Exception struct or string in throw");
	}

}

void Interpreter::visit(std::shared_ptr<ASTReticencesNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));
	value->set_undefined();
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTWhileNode> astnode) {
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

void Interpreter::visit(std::shared_ptr<ASTDoWhileNode> astnode) {
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

void Interpreter::visit(std::shared_ptr<ASTStructDefinitionNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto str = StructureDefinition(astnode->identifier, astnode->variables, astnode->row, astnode->col);

	scopes[get_namespace()].back()->declare_structure_definition(str);
}

void Interpreter::visit(std::shared_ptr<ASTValueNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	current_expression_value = dynamic_cast<RuntimeValue*>(astnode->value);
}

void Interpreter::visit(std::shared_ptr<ASTLiteralNode<cp_bool>> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_BOOL));
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTLiteralNode<cp_int>> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_INT));
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTLiteralNode<cp_float>> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_FLOAT));
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTLiteralNode<cp_char>> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_CHAR));
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTLiteralNode<cp_string>> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	auto value = alocate_value(new RuntimeValue(Type::T_STRING));
	value->set(astnode->val);
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTArrayConstructorNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	Type arr_t = Type::T_ANY;
	cp_array arr = cp_array(astnode->values.size());

	if (current_expression_array_dim.size() == 0) {
		current_expression_array_type = TypeDefinition();
		current_expression_array_dim_max = 0;
		is_max = false;
	}

	++current_expression_array_dim_max;
	if (!is_max) {
		current_expression_array_dim.push_back(std::make_shared<ASTLiteralNode<cp_int>>(0, astnode->row, astnode->col));
	}

	for (size_t i = 0; i < astnode->values.size(); ++i) {
		const auto& expr = astnode->values[i];

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

		RuntimeValue* arr_value = nullptr;
		if (current_expression_value->use_ref) {
			arr_value = current_expression_value;
		}
		else {
			arr_value = alocate_value(new RuntimeValue(current_expression_value));
		}
		arr[i] = arr_value;
	}

	if (!is_max) {
		std::dynamic_pointer_cast<ASTLiteralNode<cp_int>>(current_expression_array_dim.back())->val = arr.size();
	}

	is_max = true;

	auto value = alocate_value(new RuntimeValue(Type::T_ARRAY));
	value->set(arr, arr_t, current_expression_array_dim, current_expression_array_type.type_name, current_expression_array_type.type_name_space);

	current_expression_value = value;
	current_expression_value->array_type = current_expression_array_type.type;
	current_expression_value->type_name = current_expression_array_type.type_name;
	current_expression_value->type_name_space = current_expression_array_type.type_name_space;
	--current_expression_array_dim_max;
	size_t stay = current_expression_array_dim.size() - current_expression_array_dim_max;
	std::vector<std::shared_ptr<ASTExprNode>> current_expression_array_dim_aux;
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

void Interpreter::visit(std::shared_ptr<ASTStructConstructorNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto pop = push_namespace(astnode->nmspace);
	std::shared_ptr<Scope> curr_scope;
	try {
		auto nmspace = get_namespace();
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error("error trying to find struct definition");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);

	auto str = cp_struct();

	for (auto& expr : astnode->values) {
		if (type_struct.variables.find(expr.first) == type_struct.variables.end()) {
			ExceptionHandler::throw_struct_member_err(astnode->nmspace, astnode->type_name, expr.first);
		}
		VariableDefinition var_type_struct = type_struct.variables[expr.first];

		expr.second->accept(this);

		RuntimeValue* str_value = current_expression_value;

		if (!TypeDefinition::is_any_or_match_type(var_type_struct, *current_expression_value, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_struct_type_err(astnode->nmspace, astnode->type_name, var_type_struct, evaluate_access_vector_ptr);
		}

		if (!current_expression_value->use_ref) {
			str_value = alocate_value(new RuntimeValue(str_value));
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
			RuntimeValue* str_value = alocate_value(new RuntimeValue(struct_var_def.second.type));
			str_value->set_null();
			str[struct_var_def.first] = str_value;
		}
	}

	auto value = alocate_value(new RuntimeValue(Type::T_STRUCT));
	value->set(str, astnode->type_name, astnode->nmspace);
	current_expression_value = value;

	pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTIdentifierNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto pop = push_namespace(astnode->nmspace);
	auto nmspace = get_namespace();
	std::shared_ptr<Scope> id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (...) {
		const auto& dim = astnode->identifier_vector[0].access_vector;
		auto type = Type::T_UNDEFINED;
		auto expression_value = alocate_value(new RuntimeValue(Type::T_UNDEFINED));

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
		else if (astnode->identifier == "function") {
			type = Type::T_FUNCTION;
		}

		if (is_undefined(type)) {
			std::shared_ptr<Scope> curr_scope;
			try {
				curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->identifier);
			}
			catch (...) {
				try {
					curr_scope = get_inner_most_function_scope(nmspace, astnode->identifier, nullptr, evaluate_access_vector_ptr);
					auto fun = cp_function();
					fun.first = nmspace;
					fun.second = astnode->identifier;
					current_expression_value = alocate_value(new RuntimeValue(Type::T_FUNCTION));
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

			current_expression_value = alocate_value(new RuntimeValue(arr, type, dim));
		}
		else {
			current_expression_value = alocate_value(new RuntimeValue(expression_value));
		}

		return;
	}

	auto variable = std::dynamic_pointer_cast<RuntimeVariable>(id_scope->find_declared_variable(astnode->identifier));
	auto sub_val = access_value(id_scope, variable->get_value(), astnode->identifier_vector);
	sub_val->reset_ref();

	current_expression_value = sub_val;

	if (current_expression_value->type == Type::T_STRING && astnode->identifier_vector.back().access_vector.size() > 0 && has_string_access) {
		has_string_access = false;
		auto str = current_expression_value->get_s();
		std::dynamic_pointer_cast<ASTExprNode>(astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1])->accept(this);
		auto pos = current_expression_value->get_i();

		auto char_value = alocate_value(new RuntimeValue(Type::T_CHAR));
		char_value->set(cp_char(str[pos]));
		current_expression_value = char_value;
	}

	pop_namespace(pop);
}

void Interpreter::visit(std::shared_ptr<ASTBinaryExprNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->left->accept(this);
	RuntimeValue* l_value = nullptr;
	if (current_expression_value->use_ref) {
		l_value = current_expression_value;
	}
	else {
		l_value = alocate_value(new RuntimeValue(current_expression_value));
		gc.add_root(l_value);
	}

	if (is_bool(current_expression_value->type) && astnode->op == "and" && !current_expression_value->get_b()) {
		return;
	}

	astnode->right->accept(this);
	RuntimeValue* r_value = nullptr;
	if (current_expression_value->use_ref) {
		r_value = current_expression_value;
	}
	else {
		r_value = alocate_value(new RuntimeValue(current_expression_value));
		gc.add_root(r_value);
	}

	current_expression_value = do_operation(astnode->op, l_value, r_value, true, 0);

	gc.remove_root(l_value);
	gc.remove_root(r_value);
}

void Interpreter::visit(std::shared_ptr<ASTTernaryNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);
	if (current_expression_value->get_b()) {
		astnode->value_if_true->accept(this);
	}
	else {
		astnode->value_if_false->accept(this);
	}
}

void Interpreter::visit(std::shared_ptr<ASTInNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->value->accept(this);
	RuntimeValue expr_val = RuntimeValue(current_expression_value);
	astnode->collection->accept(this);
	bool res = false;

	if (is_array(current_expression_value->type)) {
		cp_array expr_col = current_expression_value->get_arr();

		for (size_t i = 0; i < expr_col.size(); ++i) {
			res = equals_value(&expr_val, expr_col[i]);
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

	auto value = alocate_value(new RuntimeValue(cp_bool(res)));
	current_expression_value = value;
}

void Interpreter::visit(std::shared_ptr<ASTUnaryExprNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	if (astnode->unary_op == "--" || astnode->unary_op == "++") {
		auto expr = std::make_shared<ASTLiteralNode<cp_int>>(1, astnode->row, astnode->col);
		if (const auto id = std::dynamic_pointer_cast<ASTIdentifierNode>(astnode->expr)) {
			auto assign_node = std::make_shared<ASTAssignmentNode>(id->identifier_vector, id->nmspace, std::string{ astnode->unary_op[0] } + "=", expr, astnode->row, astnode->col);
			assign_node->accept(this);
		}
		else {
			auto binex_node = std::make_shared<ASTBinaryExprNode>(std::string{ astnode->unary_op[0] }, astnode->expr, expr, astnode->row, astnode->col);
			binex_node->accept(this);
		}
	}
	else {
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
				current_expression_value = alocate_value(new RuntimeValue(current_expression_value));
			}

			switch (current_expression_value->type) {
			case Type::T_INT:
				if (astnode->unary_op == "-") {
					current_expression_value->set(cp_int(-current_expression_value->get_i()));
				}
				else if (astnode->unary_op == "~") {
					current_expression_value->set(cp_int(~current_expression_value->get_i()));
				}
				else {
					ExceptionHandler::throw_unary_operation_err(astnode->unary_op, *current_expression_value, evaluate_access_vector_ptr);
				}
				break;
			case Type::T_FLOAT:
				if (astnode->unary_op == "-") {
					current_expression_value->set(cp_float(-current_expression_value->get_f()));
				}
				else {
					ExceptionHandler::throw_unary_operation_err(astnode->unary_op, *current_expression_value, evaluate_access_vector_ptr);
				}
				break;
			case Type::T_BOOL:
				if (astnode->unary_op == "not") {
					current_expression_value->set(cp_bool(!current_expression_value->get_b()));
				}
				else {
					ExceptionHandler::throw_unary_operation_err(astnode->unary_op, *current_expression_value, evaluate_access_vector_ptr);
				}
				break;
			default:
				ExceptionHandler::throw_unary_operation_err(astnode->unary_op, *current_expression_value, evaluate_access_vector_ptr);
			}
		}
	}

}

void Interpreter::visit(std::shared_ptr<ASTTypeParseNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	RuntimeValue* new_value = alocate_value(new RuntimeValue());

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

void Interpreter::visit(std::shared_ptr<ASTNullNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression_value = alocate_value(new RuntimeValue(Type::T_VOID));
}

void Interpreter::visit(std::shared_ptr<ASTThisNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression_value = alocate_value(new RuntimeValue(cp_string(current_this_name.top())));
}

void Interpreter::visit(std::shared_ptr<ASTTypingNode> astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (astnode->image == "refid") {
		current_expression_value = alocate_value(new RuntimeValue(cp_int(current_expression_value)));
		return;
	}

	if (astnode->image == "is_any") {
		auto value = alocate_value(new RuntimeValue(Type::T_BOOL));
		value->set(cp_bool(
			(current_expression_value->ref.lock()
				&& (is_any(current_expression_value->ref.lock()->type))
				|| is_any(current_expression_value->ref.lock()->array_type))));
		current_expression_value = value;
		return;
	}
	else if (astnode->image == "is_array") {
		auto value = alocate_value(new RuntimeValue(Type::T_BOOL));
		value->set(cp_bool(is_array(current_expression_value->type) || current_expression_value->dim.size() > 0));
		current_expression_value = value;
		return;
	}
	else if (astnode->image == "is_struct") {
		auto value = alocate_value(new RuntimeValue(Type::T_BOOL));
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
			auto arr = curr_value->get_arr()[0];
			for (size_t i = 0; i < dim.size() - 1; ++i) {
				arr = arr->get_arr()[0];
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
		auto value = alocate_value(new RuntimeValue(Type::T_INT));
		value->set(cp_int(axe::StringUtils::hashcode(str_type)));
		current_expression_value = value;
	}
	else {
		auto value = alocate_value(new RuntimeValue(Type::T_STRING));
		value->set(cp_string(str_type));
		current_expression_value = value;
	}
}

cp_bool Interpreter::equals_value(const RuntimeValue* lval, const RuntimeValue* rval) {
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
	if (larr.size() != rarr.size()) {
		return false;
	}

	for (size_t i = 0; i < larr.size(); ++i) {
		if (!equals_value(larr[i], rarr[i])) {
			return false;
		}
	}

	return true;
}

RuntimeValue* Interpreter::set_value(std::shared_ptr<Scope> scope, const std::vector<parser::Identifier>& identifier_vector, RuntimeValue* new_value) {
	auto var = std::dynamic_pointer_cast<RuntimeVariable>(scope->find_declared_variable(identifier_vector[0].identifier));

	if (identifier_vector.size() == 1 && identifier_vector[0].access_vector.size() == 0) {
		var->set_value(new_value);
		return var->get_value();
	}

	RuntimeValue* before_value = nullptr;
	RuntimeValue* value = var->get_value();
	size_t i = 0;

	while (i < identifier_vector.size()) {
		before_value = value;

		auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

		if (access_vector.size() > 0) {
			auto current_val = value->get_raw_arr();
			size_t s = 0;
			size_t access_pos = 0;

			for (s = 0; s < access_vector.size() - 1; ++s) {
				access_pos = access_vector.at(s);

				// break if it is a string, and the string access will be handled in identifier node evaluation
				if (is_string((*current_val)[access_pos]->type)) {
					(*current_val)[access_pos]->get_s()[access_vector.at(s + 1)] = new_value->get_c();
					return (*current_val)[access_pos];
				}
				if (access_pos >= current_val->size()) {
					throw std::runtime_error("invalid array position access");
				}
				current_val = (*current_val)[access_pos]->get_raw_arr();
			}
			if (is_string(value->type)) {
				value->get_s()[access_vector.at(s)] = new_value->get_c();
				return value;
			}
			access_pos = access_vector.at(s);
			if (i == identifier_vector.size() - 1) {
				(*current_val)[access_pos] = new_value;
			}
			else {
				value = (*current_val)[access_pos];
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
				value->set_sub(identifier_vector[i].identifier, new_value);
			}
			else {
				value = value->get_str()[identifier_vector[i].identifier];
			}
		}
	}

	return value;
}

RuntimeValue* Interpreter::access_value(const std::shared_ptr<Scope> scope, RuntimeValue* value, const std::vector<Identifier>& identifier_vector, size_t i) {
	RuntimeValue* next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		auto current_val = next_value->get_arr();
		size_t s = 0;
		size_t access_pos = 0;

		for (s = 0; s < access_vector.size() - 1; ++s) {
			access_pos = access_vector.at(s);
			// break if it is a string, and the string access will be handled in identifier node evaluation
			if (is_string(current_val[access_pos]->type)) {
				has_string_access = true;
				break;
			}
			if (access_pos >= current_val.size()) {
				throw std::runtime_error("invalid array position access");
			}
			current_val = current_val[access_pos]->get_arr();
		}
		if (is_string(next_value->type)) {
			has_string_access = true;
			return next_value;
		}
		access_pos = access_vector.at(s);
		next_value = current_val[access_pos];
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

void Interpreter::check_build_array(RuntimeValue* new_value, std::vector<std::shared_ptr<ASTExprNode>> dim) {
	if (is_array(new_value->type) && dim.size() > 0) {
		auto arr = new_value->get_arr();
		bool has_array = false;
		cp_array rarr = cp_array();

		if (arr.size() == 1) {
			has_array = true;
			rarr = build_array(dim, arr[0], dim.size() - 1);
		}
		else if (arr.size() == 0) {
			has_array = true;
			rarr = build_undefined_array(dim, dim.size() - 1);
		}

		if (has_array) {
			new_value->set(rarr, current_expression_array_type.type,
				current_expression_array_type.dim,
				current_expression_array_type.type_name,
				current_expression_array_type.type_name_space);
		}
	}
}

cp_array Interpreter::build_array(const std::vector<std::shared_ptr<ASTExprNode>>& dim, RuntimeValue* init_value, long long i) {
	cp_array raw_arr;

	if (dim.size() - 1 == i) {
		current_expression_array_type = TypeDefinition();
	}

	size_t size = 0;
	if (dim.size() == 0) {
		size = 1;
	}
	else {
		auto crr_acc = dim[i];
		std::dynamic_pointer_cast<ASTExprNode>(crr_acc)->accept(this);
		size = current_expression_value->get_i();
	}

	raw_arr = cp_array(size);

	for (size_t j = 0; j < size; ++j) {
		auto val = alocate_value(new RuntimeValue(init_value));

		if (is_undefined(current_expression_array_type.type) || is_array(current_expression_array_type.type)) {
			current_expression_array_type = *val;
		}

		raw_arr[j] = val;
	}

	--i;

	if (i >= 0) {
		size_t stay = dim.size() - i - 1;
		std::vector<std::shared_ptr<ASTExprNode>> curr_arr_dim;
		size_t curr_dim_i = dim.size() - 1;
		for (size_t i = 0; i < stay; ++i) {
			curr_arr_dim.emplace(curr_arr_dim.begin(), dim.at(curr_dim_i));
			--curr_dim_i;
		}

		auto val = alocate_value(new RuntimeValue(raw_arr, current_expression_array_type.array_type, curr_arr_dim,
			current_expression_array_type.type_name, current_expression_array_type.type_name_space));

		return build_array(dim, val, i);
	}

	return raw_arr;
}

cp_array Interpreter::build_undefined_array(const std::vector<std::shared_ptr<ASTExprNode>>& dim, long long i) {
	cp_array raw_arr;

	if (dim.size() - 1 == i) {
		current_expression_array_type = TypeDefinition();
	}

	size_t size = 0;
	if (dim.size() == 0) {
		size = 1;
	}
	else {
		auto crr_acc = dim[i];
		std::dynamic_pointer_cast<ASTExprNode>(crr_acc)->accept(this);
		size = current_expression_value->get_i();
	}

	raw_arr = cp_array(size);

	--i;

	if (i >= 0) {
		return build_undefined_array(dim, i);
	}

	return raw_arr;
}

std::vector<unsigned int> Interpreter::calculate_array_dim_size(const cp_array& arr) {
	auto dim = std::vector<unsigned int>();

	dim.push_back(arr.size());

	if (is_array(arr[0]->type)) {
		auto dim2 = calculate_array_dim_size(arr[0]->get_arr());
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

std::vector<unsigned int> Interpreter::evaluate_access_vector(const std::vector<std::shared_ptr<ASTExprNode>>& expr_access_vector) {
	auto access_vector = std::vector<unsigned int>();
	for (const auto& expr : expr_access_vector) {
		unsigned int val = 0;
		if (expr) {
			std::dynamic_pointer_cast<ASTExprNode>(expr)->accept(this);
			if (!is_int(current_expression_value->type)) {
				throw std::runtime_error("array index access must be a integer value");
			}
			val = current_expression_value->get_i();
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

std::string Interpreter::parse_value_to_string(const RuntimeValue* value) {
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
	case Type::T_FUNCTION:
		str = value->get_fun().first + (value->get_fun().first.empty() ? "" : "::") + value->get_fun().second + "(...)";
		break;
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
	for (auto i = 0; i < arr_value.size(); ++i) {
		bool isc = is_char(arr_value[i]->type);
		bool iss = is_string(arr_value[i]->type);

		if (isc) s << "'";
		else if (iss) s << "\"";

		s << parse_value_to_string(arr_value[i]);

		if (isc) s << "'";
		else if (iss) s << "\"";

		if (i < arr_value.size() - 1) {
			s << ",";
		}
	}
	s << "]";
	return s.str();
}

std::string Interpreter::parse_struct_to_string(const RuntimeValue* value) {
	auto str_value = value->get_str();
	std::stringstream s = std::stringstream();
	if (!value->type_name_space.empty()) {
		s << value->type_name_space << "::";
	}
	s << value->type_name << "<" << value << ">{";
	for (auto const& [key, val] : str_value) {
		//if (key != modules::Module::INSTANCE_ID_NAME) {
			s << key + ":";
			s << parse_value_to_string(val);
			s << ",";
		//}
	}
	if (s.str() != "{") {
		s.seekp(-1, std::ios_base::end);
	}
	s << "}";
	return s.str();
}

RuntimeValue* Interpreter::do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, bool is_expr, cp_int str_pos) {
	Type l_var_type = lval->ref.lock() ? lval->ref.lock()->type : lval->type;
	Type l_var_array_type = lval->ref.lock() ? lval->ref.lock()->array_type : lval->array_type;
	Type l_type = is_undefined(lval->type) ? l_var_type : lval->type;
	Type r_var_type = rval->ref.lock() ? rval->ref.lock()->type : rval->type;
	Type r_var_array_type = rval->ref.lock() ? rval->ref.lock()->array_type : rval->array_type;
	Type r_type = rval->type;
	RuntimeValue* res_value = nullptr;

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
		return alocate_value(new RuntimeValue((cp_bool)((op == "==") ?
			match_type(l_type, r_type)
			: !match_type(l_type, r_type))));
	}

	if (lval->use_ref
		&& Token::is_equality_op(op)) {
		return alocate_value(new RuntimeValue((cp_bool)((op == "==") ?
			lval == rval
			: lval != rval)));
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
			res_value = alocate_value(new RuntimeValue((cp_bool)(lval->get_b() && rval->get_b())));
		}
		else if (op == "or") {
			res_value = alocate_value(new RuntimeValue((cp_bool)(lval->get_b() || rval->get_b())));
		}
		else if (op == "==") {
			res_value = alocate_value(new RuntimeValue((cp_bool)(lval->get_b() == rval->get_b())));
		}
		else if (op == "!=") {
			res_value = alocate_value(new RuntimeValue((cp_bool)(lval->get_b() != rval->get_b())));
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
			res_value = alocate_value(new RuntimeValue((cp_int)(do_spaceship_operation(op, lval, rval))));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_relational_op(op)) {
			res_value = alocate_value(new RuntimeValue(do_relational_operation(op, lval, rval)));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_equality_op(op)) {
			cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
			cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				l == r : l != r)));

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
			res_value = alocate_value(new RuntimeValue(do_spaceship_operation(op, lval, rval)));

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

			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				l == r : l != r)));

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
			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				lval->get_c() == rval->get_c()
				: lval->get_c() != lval->get_c())));

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

			if (lval->get_s().size() > 30) {
				int x = 0;
			}

			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				lval->get_s() == rval->get_s()
				: lval->get_s() != rval->get_s())));

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
			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval))));

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
			res_value = alocate_value(new RuntimeValue((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval))));

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

cp_bool Interpreter::do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
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

cp_int Interpreter::do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
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
		return rval;
	}
	else if (op == "+=" || op == "+") {
		lval.insert(lval.end(), rval.begin(), rval.end());

		return lval;
	}

	throw std::runtime_error("invalid '" + op + "' operator for types 'array' and 'array'");
}

void Interpreter::normalize_type(std::shared_ptr<RuntimeVariable> var, RuntimeValue* val) {
	if (is_string(var->type) && is_char(val->type)) {
		val->type = var->type;
		val->set(cp_string{ val->get_c() });
	}
	else if (is_float(var->type) && is_int(val->type)) {
		val->type = var->type;
		val->set(cp_float(val->get_i()));
	}
}

RuntimeValue* Interpreter::alocate_value(RuntimeValue* value) {
	return dynamic_cast<RuntimeValue*>(gc.allocate(value));
}

long long Interpreter::hash(std::shared_ptr<ASTExprNode> astnode) {
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

long long Interpreter::hash(std::shared_ptr<ASTValueNode> astnode) {
	auto value = dynamic_cast<RuntimeValue*>(astnode->value);
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

long long Interpreter::hash(std::shared_ptr<ASTLiteralNode<cp_bool>> astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(std::shared_ptr<ASTLiteralNode<cp_int>> astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(std::shared_ptr<ASTLiteralNode<cp_float>> astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(std::shared_ptr<ASTLiteralNode<cp_char>> astnode) {
	return static_cast<long long>(astnode->val);
}

long long Interpreter::hash(std::shared_ptr<ASTLiteralNode<cp_string>> astnode) {
	return axe::StringUtils::hashcode(astnode->val);
}

long long Interpreter::hash(std::shared_ptr<ASTIdentifierNode> astnode) {
	auto pop = push_namespace(astnode->nmspace);
	std::string nmspace = get_namespace();

	std::shared_ptr<Scope> id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	auto variable = std::dynamic_pointer_cast<RuntimeVariable>(id_scope->find_declared_variable(astnode->identifier_vector[0].identifier));
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
	pop_namespace(pop);
}

void Interpreter::declare_function_parameter(std::shared_ptr<Scope> scope, const std::string& identifier, RuntimeValue* value) {
	if (is_function(value->type)) {
		auto nmspace = value->get_fun().first;
		auto funcs = get_inner_most_functions_scope(nmspace,
			value->get_fun().second)->find_declared_functions(value->get_fun().second);
		for (auto& it = funcs.first; it != funcs.second; ++it) {
			scope->declare_function(identifier, it->second);
		}
	}
	else {
		if (value->use_ref && value->ref.lock()) {
			scope->declare_variable(identifier, value->ref.lock());
		}
		else {
			auto var = std::make_shared<RuntimeVariable>(identifier, *value);
			gc.add_var_root(var);
			var->set_value(value);
			scope->declare_variable(identifier, var);
		}
	}
}

void Interpreter::declare_function_block_parameters(const std::string& nmspace) {
	auto& curr_scope = scopes[nmspace].back();
	auto rest_name = std::string();
	auto vec = std::vector<RuntimeValue*>();
	size_t i = 0;

	if (current_function_calling_arguments.size() == 0 || current_function_defined_parameters.size() == 0) {
		return;
	}

	// adds function arguments
	for (i = 0; i < current_function_calling_arguments.top().size(); ++i) {
		auto current_function_calling_argument = current_function_calling_arguments.top()[i];

		if (current_function_defined_parameters.top().size() > i
			&& is_string(current_function_defined_parameters.top()[i]->type) && is_char(current_function_calling_argument->type)) {
			if (current_function_calling_argument->use_ref
				&& current_function_calling_argument->ref.lock()
				&& !is_any(current_function_calling_argument->ref.lock()->type)) {
				throw std::runtime_error("cannot reference char to string in function call");
			}
			current_function_calling_argument->type = current_function_defined_parameters.top()[i]->type;
			current_function_calling_argument->set(cp_string{ current_function_calling_argument->get_c() });
		}
		else if (current_function_defined_parameters.top().size() > i
			&& is_float(current_function_defined_parameters.top()[i]->type) && is_int(current_function_calling_argument->type)) {
			if (current_function_calling_argument->use_ref
				&& current_function_calling_argument->ref.lock()
				&& !is_any(current_function_calling_argument->ref.lock()->type)) {
				throw std::runtime_error("cannot reference int to float in function call");
			}
			current_function_calling_argument->type = current_function_defined_parameters.top()[i]->type;
			current_function_calling_argument->set(cp_float(current_function_calling_argument->get_i()));
		}

		// is reference : not reference
		RuntimeValue* current_value = nullptr;
		if (current_function_calling_argument->use_ref) {
			current_value = current_function_calling_argument;
		}
		else {
			current_value = alocate_value(new RuntimeValue(current_function_calling_argument));
			current_value->ref.reset();
		}

		if (i >= current_function_defined_parameters.top().size()) {
			vec.push_back(current_value);
		}
		else {
			if (const auto decl = dynamic_cast<VariableDefinition*>(current_function_defined_parameters.top()[i])) {
				declare_function_parameter(curr_scope, decl->identifier, current_value);

				// is rest
				if (decl->is_rest) {
					rest_name = decl->identifier;
					// if is last parameter and is array
					if (current_function_defined_parameters.top().size() - 1 == i
						&& is_array(current_value->type)) {
						for (size_t i = 0; i < vec.size(); ++i) {
							vec.push_back(current_value->get_arr()[i]);
						}
					}
					else {
						vec.push_back(current_value);
					}
				}
			}
			else if (const auto decls = dynamic_cast<UnpackedVariableDefinition*>(current_function_defined_parameters.top()[i])) {
				for (auto& decl : decls->variables) {
					auto sub_value = alocate_value(new RuntimeValue(current_value->get_str()[decl.identifier]));
					declare_function_parameter(curr_scope, decl.identifier, sub_value);
				}
			}
		}
	}

	// adds default values
	for (; i < current_function_defined_parameters.top().size(); ++i) {
		if (const auto decl = dynamic_cast<VariableDefinition*>(current_function_defined_parameters.top()[i])) {
			if (decl->is_rest) {
				break;
			}

			std::dynamic_pointer_cast<ASTExprNode>(decl->default_value)->accept(this);
			auto current_value = alocate_value(new RuntimeValue(current_expression_value));

			declare_function_parameter(curr_scope, decl->identifier, current_value);
		}
	}

	if (vec.size() > 0) {
		auto arr = cp_array(vec.size());
		for (size_t i = 0; i < vec.size(); ++i) {
			arr[i] = vec[i];
		}
		auto rest = alocate_value(new RuntimeValue(arr, Type::T_ANY, std::vector<std::shared_ptr<ASTExprNode>>()));
		auto var = std::make_shared<RuntimeVariable>(rest_name, *rest);
		gc.add_var_root(var);
		var->set_value(rest);
		curr_scope->declare_variable(rest_name, var);
	}

	current_function_defined_parameters.pop();
	current_function_calling_arguments.pop();
}

void Interpreter::build_args(const std::vector<std::string>& args) {
	auto dim = std::vector<std::shared_ptr<ASTExprNode>>{ std::make_shared<ASTLiteralNode<cp_int>>(cp_int(args.size()), 0, 0) };

	auto var = std::make_shared<RuntimeVariable>("cpargs", Type::T_ARRAY, Type::T_STRING, dim, "", "");
	gc.add_var_root(var);

	auto arr = cp_array();
	for (size_t i = 0; i < args.size(); ++i) {
		arr.push_back(alocate_value(new RuntimeValue(args[i])));
	}

	var->set_value(alocate_value(new RuntimeValue(arr, Type::T_STRING, dim)));

	scopes[default_namespace].back()->declare_variable("cpargs", var);
}

void Interpreter::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Interpreter::msg_header() {
	return "(IERR) " + current_program.top()->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
