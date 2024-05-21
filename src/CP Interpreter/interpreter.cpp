#include <iostream>
#include <cmath>

#include "interpreter.hpp"
#include "util.hpp"
#include "watch.h"


using namespace visitor;
using namespace parser;


Interpreter::Interpreter(InterpreterScope* global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: current_expression_value(Value(parser::Type::T_UNDEF)), is_function_context(false),
	Visitor(programs, main_program, main_program ? main_program->name : "main") {
	scopes["main"].push_back(global_scope);
}

std::string Interpreter::get_namespace(std::string nmspace) {
	return get_namespace(current_program, nmspace);
}

std::string Interpreter::get_namespace(ASTProgramNode* program, std::string nmspace) {
	return nmspace.empty() ? (current_function_nmspace.empty() ? (program->alias.empty() ? "main" : program->alias) : current_function_nmspace) : nmspace;
}

void Interpreter::start() {
	visit(current_program);
}

void Interpreter::visit(parser::ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void Interpreter::visit(parser::ASTUsingNode* astnode) {
	std::string libname = axe::Util::join(astnode->library, ".");

	auto program = programs[libname];

	// add lib to current program
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::Util::contains(libs, libname)) {
		libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		if (!program->alias.empty()) {
			scopes[program->alias].push_back(new InterpreterScope());
		}
		start();
		current_program = prev_program;
	}

}


Value* Interpreter::access_value(InterpreterScope* scope, Value* value, std::vector<parser::Identifier> identifier_vector, size_t i) {
	Value* next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		cp_array* current_Val = &next_value->arr;
		size_t s = 0;
		size_t access_pos = 0;

		for (s = 0; s < access_vector.size() - 1; ++s) {
			// check array position access
			access_pos = access_vector.at(s);
			// break if it is a string, and the string access will be handled in identifier node evaluation
			// TODO: ckeck if it will handle string assign
			if (current_Val->at(access_pos)->curr_type == parser::Type::T_STRING) {
				has_string_access = true;
				break;
			}
			current_Val = &current_Val->at(access_pos)->arr;
		}
		access_pos = access_vector.at(s);
		next_value = current_Val->at(access_pos);
	}

	++i;

	if (i < identifier_vector.size()) {
		next_value = next_value->str->second[identifier_vector[i].identifier];

		if (identifier_vector[i].access_vector.size() > 0 || i < identifier_vector.size()) {
			return access_value(scope, next_value, identifier_vector, i);
		}
	}

	return next_value;
}

void Interpreter::visit(parser::ASTDeclarationNode* astnode) {
	// visit expression to update current value/type
	if (astnode->expr) {
		astnode->expr->accept(this);
	}

	if (astnode->expr && current_expression_value.has_value()) {
		auto type = astnode->type == parser::Type::T_ANY ? current_expression_value.curr_type : astnode->type;
		// declare variable, depending on type
		switch (type) {
		case parser::Type::T_BOOL:
			scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.b);
			break;
		case parser::Type::T_INT:
			scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.i);
			break;
		case parser::Type::T_FLOAT:
			if (current_expression_value.curr_type == parser::Type::T_INT) {
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, (cp_float)current_expression_value.i);
			}
			else {
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.f);
			}
			break;
		case parser::Type::T_CHAR:
			scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.c);
			break;
		case parser::Type::T_STRING:
			if (current_expression_value.curr_type == parser::Type::T_CHAR) {
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, std::string{ current_expression_value.c });
			}
			else {
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.s);
			}
			break;
		case parser::Type::T_ARRAY: {
			if (current_expression_value.arr.size() == 1) {
				//auto init_value = current_expression_value.arr[0];
				//auto dim = evaluate_access_vector(astnode->dim);
				auto arr = build_array(astnode->dim, current_expression_value.arr[0], astnode->dim.size() - 1);
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, arr);
			}
			else {
				scopes[get_namespace()].back()->declare_variable(astnode->identifier, current_expression_value.arr);
			}
			break;
		}
		case parser::Type::T_STRUCT:
			declare_new_structure(astnode->identifier, current_expression_value);
			break;
		}
	}
	else {
		if (is_void(current_expression_value.curr_type)) {
			if (astnode->type == parser::Type::T_STRUCT) {
				auto undefvalue = Value(parser::Type::T_STRUCT);
				undefvalue.set_null();
				undefvalue.str->first = astnode->type_name;
				declare_new_structure(astnode->identifier, undefvalue);
			}
			else {
				scopes[get_namespace()].back()->declare_null_variable(astnode->identifier, astnode->type);
			}
		}
		else {
			if (astnode->type == parser::Type::T_STRUCT) {
				auto nllvalue = Value(parser::Type::T_STRUCT);
				nllvalue.set_undefined();
				nllvalue.str->first = astnode->type_name;
				declare_new_structure(astnode->identifier, nllvalue);
			}
			else {
				scopes[get_namespace()].back()->declare_undef_variable(astnode->identifier, astnode->type);
			}
		}
	}

	Value* val = access_value(scopes[get_namespace()].back(), scopes[get_namespace()].back()->find_declared_variable(astnode->identifier),
		std::vector<parser::Identifier>{parser::Identifier(astnode->identifier, std::vector<parser::ASTExprNode*>())});
	//val->dim = astnode->dim;
	val->arr_type = astnode->array_type;
}

std::vector<Value*> Interpreter::build_array(std::vector<parser::ASTExprNode*> dim, Value* init_value, long long i) {
	auto arr = std::vector<Value*>();

	auto crr_acc = dim[i];
	crr_acc->accept(this);

	size_t size = current_expression_value.i;

	for (size_t j = 0; j < size; ++j) {
		arr.push_back(init_value);
	}

	--i;

	if (i >= 0) {
		auto val = new Value(Type::T_ARRAY);
		val->set(arr);
		return build_array(dim, val, i);
	}

	return arr;
}

void Interpreter::declare_new_structure(std::string identifier_vector, Value new_value) {
	Value* value = nullptr;
	std::string type_name = new_value.str->first;
	parser::StructureDefinition struct_definition;
	size_t str_def_scope_idx = 0;

	if (new_value.has_value()) {
		value = scopes[get_namespace()].back()->declare_variable(identifier_vector, new_value.str);
	}
	else {
		value = scopes[get_namespace()].back()->declare_null_struct_variable(identifier_vector, type_name);
	}
}

std::vector<unsigned int> Interpreter::evaluate_access_vector(std::vector<parser::ASTExprNode*> exprAcessVector) {
	auto access_vector = std::vector<unsigned int>();
	for (auto expr : exprAcessVector) {
		expr->accept(this);
		access_vector.push_back(current_expression_value.i);
	}
	return access_vector;
}

cp_int Interpreter::do_operation(cp_int lval, cp_int rval, std::string op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=") {
		return lval + rval;
	}
	else if (op == "-=") {
		return lval - rval;
	}
	else if (op == "*=") {
		return lval * rval;
	}
	else if (op == "/=") {
		return lval / rval;
	}
	else if (op == "%=") {
		return lval % rval;
	}
	return rval;
}

cp_float Interpreter::do_operation(cp_float lval, cp_float rval, std::string op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=") {
		return lval + rval;
	}
	else if (op == "-=") {
		return lval - rval;
	}
	else if (op == "*=") {
		return lval * rval;
	}
	else if (op == "/=") {
		return lval / rval;
	}
	return rval;
}

cp_string Interpreter::do_operation(cp_string lval, cp_string rval, std::string op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=") {
		return lval + rval;
	}
	return rval;
}

void Interpreter::visit(parser::ASTAssignmentNode* astnode) {
	// determine innermost scope in which variable is declared
	size_t i;
	for (i = scopes[get_namespace()].size() - 1; !scopes[get_namespace()][i]->already_declared_variable(astnode->identifier_vector[0].identifier); i--);

	Value* root = scopes[get_namespace()][i]->find_declared_variable(astnode->identifier_vector[0].identifier);
	Value* value = access_value(scopes[get_namespace()][i], root, astnode->identifier_vector);

	// visit expression node to update current value/type
	astnode->expr->accept(this);

	if (current_expression_value.has_value()) {
		switch (current_expression_value.curr_type) {
		case parser::Type::T_BOOL:
			value->set(current_expression_value.b);
			break;
		case parser::Type::T_INT:
			value->set(do_operation(value->i, current_expression_value.i, astnode->op));
			break;
		case parser::Type::T_FLOAT:
			value->set(do_operation(value->f, current_expression_value.f, astnode->op));
			break;
		case parser::Type::T_CHAR:
			if (is_string(value->curr_type)) {
				value->set(do_operation(value->s, std::string{ current_expression_value.c }, astnode->op));
			}
			else {
				value->set(current_expression_value.c);
			}
			break;
		case parser::Type::T_STRING:
			value->set(do_operation(value->s, current_expression_value.s, astnode->op));
			break;
		case parser::Type::T_ARRAY:
			value->set(current_expression_value.arr);
			break;
		case parser::Type::T_STRUCT:
			value->set(current_expression_value.str);
		}
	}
	else {
		value->set_null();
	}
}

void Interpreter::visit(parser::ASTReturnNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
	for (long long i = scopes[get_namespace()].size() - 1; i >= 0; --i) {
		if (!scopes[get_namespace()][i]->get_name().empty()) {
			return_from_function_name = scopes[get_namespace()][i]->get_name();
			return_from_function = true;
			break;
		}
	}
}

void Interpreter::visit(parser::ASTBlockNode* astnode) {
	// create new scope
	scopes[get_namespace()].push_back(new InterpreterScope(current_function_name));
	current_function_name = "";

	// add parameters to the current scope
	for (unsigned int i = 0; i < current_function_arguments.size(); ++i) {
		switch (current_function_arguments[i].first) {
		case parser::Type::T_BOOL:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->b);
			break;
		case parser::Type::T_INT:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->i);
			break;
		case parser::Type::T_FLOAT:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->f);
			break;
		case parser::Type::T_CHAR:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->c);
			break;
		case parser::Type::T_STRING:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->s);
			break;
		case parser::Type::T_STRUCT:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->str);
			break;
		case parser::Type::T_ARRAY:
			scopes[get_namespace()].back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->arr);
			break;
		}
	}

	// clear the global function parameter/argument vectors
	current_function_parameters.clear();
	current_function_arguments.clear();

	// visit each statement in the block
	for (auto& stmt : astnode->statements) {
		stmt->accept(this);

		if (continue_block && (is_loop || is_switch)) {
			break;
		}

		if (break_block && (is_loop || is_switch)) {
			break;
		}

		if (return_from_function) {
			if (!return_from_function_name.empty() && return_from_function_name == scopes[get_namespace()].back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	// close scope
	scopes[get_namespace()].pop_back();
}

void Interpreter::visit(parser::ASTContinueNode* astnode) {
	continue_block = true;
}

void Interpreter::visit(parser::ASTBreakNode* astnode) {
	break_block = true;
}

void Interpreter::visit(parser::ASTSwitchNode* astnode) {
	is_switch = true;

	// create new scope
	scopes[get_namespace()].push_back(new InterpreterScope(""));

	int pos = -1;

	astnode->condition->accept(this);

	try {
		auto hash = astnode->condition->hash(this);
		pos = astnode->parsed_case_blocks->at(hash);
	}
	catch (...) {
		pos = astnode->default_block;
	}

	// visit each statement in the block
	for (int i = pos; i < astnode->statements->size(); ++i) {
		astnode->statements->at(i)->accept(this);

		if (continue_block) {
			continue_block = false;
			continue;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (return_from_function) {
			if (!return_from_function_name.empty() && return_from_function_name == scopes[get_namespace()].back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	// close scope
	scopes[get_namespace()].pop_back();
	is_switch = false;
}

void Interpreter::visit(parser::ASTElseIfNode* astnode) {
	executed_elif = false;

	// evaluate if condition
	astnode->condition->accept(this);

	bool result = current_expression_value.b;

	// execute appropriate blocks
	if (result) {
		astnode->block->accept(this);
		executed_elif = true;
	}
}

void Interpreter::visit(parser::ASTIfNode* astnode) {
	// evaluate if condition
	astnode->condition->accept(this);

	bool result = current_expression_value.b;

	// execute appropriate blocks
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
}

void Interpreter::visit(parser::ASTForNode* astnode) {
	is_loop = true;
	scopes[get_namespace()].push_back(new InterpreterScope(""));

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);
	}
	else {
		current_expression_value = Value(parser::Type::T_BOOL);
		current_expression_value.set(true);
	}

	bool result = current_expression_value.b;

	while (result) {
		// execute block
		astnode->block->accept(this);

		if (continue_block) {
			continue_block = false;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		if (astnode->dci[2]) {
			astnode->dci[2]->accept(this);
		}

		// re-evaluate for condition
		if (astnode->dci[1]) {
			astnode->dci[1]->accept(this);
		}
		else {
			current_expression_value = Value(parser::Type::T_BOOL);
			current_expression_value.set(true);
		}

		result = current_expression_value.b;
	}

	scopes[get_namespace()].pop_back();
	is_loop = false;
}

void Interpreter::visit(parser::ASTForEachNode* astnode) {
	is_loop = true;

	astnode->collection->accept(this);
	auto colletion = current_expression_value.arr;

	for (auto val : colletion) {
		scopes[get_namespace()].push_back(new InterpreterScope(""));

		astnode->itdecl->accept(this);

		auto itdecl = static_cast<parser::ASTDeclarationNode*>(astnode->itdecl);

		// determine innermost scope in which variable is declared
		size_t i;
		for (i = scopes[get_namespace()].size() - 1; !scopes[get_namespace()][i]->already_declared_variable(itdecl->identifier); --i);

		Value* value = access_value(scopes[get_namespace()].back(), scopes[get_namespace()].back()->find_declared_variable(itdecl->identifier),
			std::vector<parser::Identifier>{parser::Identifier(itdecl->identifier, std::vector<parser::ASTExprNode*>())});

		switch (val->curr_type) {
		case parser::Type::T_BOOL:
			value->set(val->b);
			break;
		case parser::Type::T_INT:
			value->set(val->i);
			break;
		case parser::Type::T_FLOAT:
			value->set(val->f);
			break;
		case parser::Type::T_CHAR:
			value->set(val->c);
			break;
		case parser::Type::T_STRING:
			value->set(val->s);
			break;
		case parser::Type::T_ARRAY:
			value->set(val->arr);
			break;
		case parser::Type::T_STRUCT:
			value->set(val->str);
			break;
		}

		// execute block
		astnode->block->accept(this);

		scopes[get_namespace()].pop_back();

		if (continue_block) {
			continue_block = false;
			continue;
		}

		if (break_block) {
			break_block = false;
			break;
		}
	}

	is_loop = false;
}

void Interpreter::visit(parser::ASTWhileNode* astnode) {
	is_loop = true;

	// evaluate while condition
	astnode->condition->accept(this);

	bool result = current_expression_value.b;

	while (result) {
		// execute block
		astnode->block->accept(this);

		if (continue_block) {
			continue_block = false;
			continue;
		}

		if (break_block) {
			break_block = false;
			break;
		}

		// re-evaluate while condition
		astnode->condition->accept(this);

		result = current_expression_value.b;
	}

	is_loop = false;
}

void Interpreter::visit(parser::ASTFunctionDefinitionNode* astnode) {
	scopes[get_namespace()].back()->declare_function(astnode->identifier, astnode->signature, astnode->variable_names, astnode->block);
}

void Interpreter::visit(parser::ASTStructDefinitionNode* astnode) {
	scopes[get_namespace()].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void Interpreter::visit(parser::ASTLiteralNode<cp_bool>* lit) {
	auto value = Value(parser::Type::T_BOOL);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTLiteralNode<cp_int>* lit) {
	auto value = Value(parser::Type::T_INT);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTLiteralNode<cp_float>* lit) {
	auto value = Value(parser::Type::T_FLOAT);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTLiteralNode<cp_char>* lit) {
	auto value = Value(parser::Type::T_CHAR);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTLiteralNode<cp_string>* lit) {
	auto value = Value(parser::Type::T_STRING);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTArrayConstructorNode* astnode) {
	auto value = Value(parser::Type::T_ARRAY);
	cp_array arr = cp_array();

	for (auto& exrp : astnode->values) {
		exrp->accept(this);
		auto arr_value = new Value(current_expression_value.curr_type);
		arr_value->copy_from(&current_expression_value);
		arr.push_back(arr_value);
	}

	value.set(arr);

	current_expression_value = value;
}

void Interpreter::visit(parser::ASTStructConstructorNode* astnode) {
	auto value = Value(parser::Type::T_STRUCT);

	auto str = new cp_struct();
	str->first = astnode->type_name;
	str->second = cp_struct_values();

	for (auto& expr : astnode->values) {
		expr.second->accept(this);
		auto str_alue = new Value(current_expression_value.curr_type);
		str_alue->copy_from(&current_expression_value);
		str->second[expr.first] = str_alue;
	}

	declare_structure(str, astnode->nmspace);

	value.set(str);
	current_expression_value = value;
}

void Interpreter::declare_structure(cp_struct* str, std::string nmspace) {
	std::string actnmspace = get_namespace(nmspace);
	size_t i;
	for (i = scopes[actnmspace].size() - 1; !scopes[actnmspace][i]->already_declared_structure_definition(str->first); --i);
	auto struct_def = scopes[actnmspace][i]->find_declared_structure_definition(str->first);

	for (auto& struct_var_def : struct_def.variables) {
		if (str->second.find(struct_var_def.identifier) != str->second.end()) {
			str->second[struct_var_def.identifier]->set_type(struct_var_def.type);
		}
		else {
			Value* str_value = new Value(struct_var_def.type);
			str_value->set_null();
			str->second[struct_var_def.identifier] = str_value;
		}
	}
}

void Interpreter::visit(parser::ASTBinaryExprNode* astnode) {
	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	parser::Type l_type = current_expression_value.curr_type;
	Value l_value = current_expression_value;

	// then right node
	astnode->right->accept(this);
	parser::Type r_type = current_expression_value.curr_type;
	Value r_value = current_expression_value;

	// expression struct
	auto value = new Value(parser::Type::T_UNDEF);

	// arithmetic operators for now
	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		// two ints
		if (l_type == parser::Type::T_INT && r_type == parser::Type::T_INT) {
			current_expression_value.curr_type = parser::Type::T_INT;
			if (op == "+") {
				value->set((cp_int)(l_value.i + r_value.i));
			}
			else if (op == "-") {
				value->set((cp_int)(l_value.i - r_value.i));
			}
			else if (op == "*") {
				value->set((cp_int)(l_value.i * r_value.i));
			}
			else if (op == "/") {
				if (r_value.i == 0) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "division by zero encountered");
				}
				value->set((cp_int)(l_value.i / r_value.i));
			}
			else if (op == "%") {
				value->set((cp_int)(l_value.i % r_value.i));
			}
		}
		else if (l_type == parser::Type::T_FLOAT || r_type == parser::Type::T_FLOAT) { // at least one real
			current_expression_value.curr_type = parser::Type::T_FLOAT;
			cp_float l = l_value.f, r = r_value.f;
			if (l_type == parser::Type::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == parser::Type::T_INT) {
				r = cp_float(r_value.i);
			}
			if (op == "+") {
				value->set((cp_float)(l + r));
			}
			else if (op == "-") {
				value->set((cp_float)(l - r));
			}
			else if (op == "*") {
				value->set((cp_float)(l * r));
			}
			else if (op == "/") {
				if (r == 0) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "division by zero encountered");
				}
				value->set((cp_float)l / r);
			}
		}
		else if (l_type == parser::Type::T_CHAR && r_type == parser::Type::T_STRING) { // char and string
			current_expression_value.curr_type = parser::Type::T_STRING;
			value->set(cp_string(std::string{ l_value.c } + r_value.s));
		}
		else if (l_type == parser::Type::T_STRING && r_type == parser::Type::T_CHAR) { // string and char
			current_expression_value.curr_type = parser::Type::T_STRING;
			value->set(cp_string(l_value.s + std::string{ r_value.c }));
		}
		else if (l_type == parser::Type::T_CHAR && r_type == parser::Type::T_CHAR) { // char and string
			current_expression_value.curr_type = parser::Type::T_STRING;
			value->set(cp_string(std::string{ l_value.c } + std::string{ r_value.c }));
		}
		else { // remaining case is for strings
			current_expression_value.curr_type = parser::Type::T_STRING;
			value->set(cp_string(l_value.s + r_value.s));
		}
	}
	else if (op == "and" || op == "or") { // now bool
		current_expression_value.curr_type = parser::Type::T_BOOL;

		cp_bool l = l_value.b;
		cp_bool r = r_value.b;

		if (l_type == parser::Type::T_STRUCT) {
			l = l_value.has_value();
		}
		if (r_type == parser::Type::T_STRUCT) {
			r = r_value.has_value();
		}

		if (op == "and") {
			value->set((cp_bool)(l && r));
		}
		else if (op == "or") {
			value->set((cp_bool)(l || r));
		}
	}
	else { // now comparator operators
		current_expression_value.curr_type = parser::Type::T_BOOL;

		if (l_type == parser::Type::T_VOID || r_type == parser::Type::T_VOID) {
			value->set((cp_bool)((op == "==") ? match_type(l_value.curr_type, r_value.curr_type) : !match_type(l_value.curr_type, r_value.curr_type)));
		}
		else if (l_type == parser::Type::T_BOOL) {
			cp_bool l = l_value.b;
			cp_bool r = r_value.b;

			value->set((cp_bool)((op == "==") ? l_value.b == r_value.b : l_value.b != r_value.b));
		}
		else if (l_type == parser::Type::T_STRING) {
			value->set((cp_bool)((op == "==") ? l_value.s == r_value.s : l_value.s != r_value.s));
		}
		else if (l_type == parser::Type::T_ARRAY) {
			value->set((cp_bool)((op == "==") ? l_value.arr == r_value.arr : l_value.arr != r_value.arr));
		}
		else if (l_type == parser::Type::T_STRUCT) {
			value->set((cp_bool)((op == "==") ? l_value.str == r_value.str : l_value.str != r_value.str));
		}
		else {
			cp_float l = l_value.f, r = r_value.f;

			if (l_type == parser::Type::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == parser::Type::T_INT) {
				r = cp_float(r_value.i);
			}
			if (op == "==") {
				value->set((cp_bool)(l == r));
			}
			else if (op == "!=") {
				value->set((cp_bool)(l != r));
			}
			else if (op == "<") {
				value->set((cp_bool)(l < r));
			}
			else if (op == ">") {
				value->set((cp_bool)(l > r));
			}
			else if (op == ">=") {
				value->set((cp_bool)(l >= r));
			}
			else if (op == "<=") {
				value->set((cp_bool)(l <= r));
			}
		}
	}

	// update current expression
	current_expression_value = *value;
}

void Interpreter::visit(parser::ASTIdentifierNode* astnode) {
	auto identifier = astnode->identifier_vector[0].identifier;
	auto nmspace = get_namespace(astnode->nmspace);
	InterpreterScope* id_scope = nullptr;
	long long i;
	if (astnode->identifier_vector[0].identifier == "List") {
		int a = 0;
	}
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); --i) {
		if (i <= 0) {
			auto dim = astnode->identifier_vector[0].access_vector;
			auto type = Type::T_UNDEF;
			auto expression_value = new Value(Type::T_UNDEF);

			if (identifier == "bool") {
				type = Type::T_BOOL;
			}
			else if (identifier == "int") {
				type = Type::T_INT;
			}
			else if (identifier == "float") {
				type = Type::T_FLOAT;
			}
			else if (identifier == "char") {
				type = Type::T_CHAR;
			}
			else if (identifier == "string") {
				type = Type::T_STRING;
			}

			if (is_undefined(type)) {
				for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); --i);
				type = Type::T_STRUCT;
				auto str = new cp_struct();
				str->first = identifier;
				expression_value->set(str);
			}

			expression_value->set_type(type);
			expression_value->set_curr_type(type);

			if (dim.size() > 0) {
				cp_array arr;

				arr = build_array(dim, expression_value, dim.size() - 1);

				current_expression_value = Value(Type::T_ARRAY);
				current_expression_value.set_arr_type(type);
				current_expression_value.set(arr);
			}
			else {
				current_expression_value.copy_from(expression_value);
			}

			return;
		}
	}
	id_scope = scopes[nmspace][i];

	auto root = id_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	current_expression_value = *access_value(id_scope, root, astnode->identifier_vector);

	if (current_expression_value.curr_type == parser::Type::T_STRING && astnode->identifier_vector.back().access_vector.size() > 0 && has_string_access) {
		astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1]->accept(this);
		auto pos = current_expression_value.i;

		auto char_value = Value(current_expression_value.curr_type);
		char_value.set(cp_char(current_expression_value.s[pos]));
		current_expression_value = char_value;
	}
}

void Interpreter::visit(parser::ASTUnaryExprNode* astnode) {
	bool has_assign = false;

	// update current expression
	astnode->expr->accept(this);
	switch (current_expression_value.curr_type) {
	case parser::Type::T_INT:
		if (astnode->unary_op == "-") {
			current_expression_value.set(cp_int(current_expression_value.i * -1));
		}
		else if (astnode->unary_op == "--") {
			current_expression_value.set(cp_int(--current_expression_value.i));
			has_assign = true;
		}
		else if (astnode->unary_op == "++") {
			current_expression_value.set(cp_int(++current_expression_value.i));
			has_assign = true;
		}
		break;
	case parser::Type::T_FLOAT:
		if (astnode->unary_op == "-") {
			current_expression_value.set(cp_float(current_expression_value.f * -1));
		}
		else if (astnode->unary_op == "--") {
			current_expression_value.set(cp_float(--current_expression_value.f));
			has_assign = true;
		}
		else if (astnode->unary_op == "++") {
			current_expression_value.set(cp_float(++current_expression_value.f));
			has_assign = true;
		}
		break;
	case parser::Type::T_BOOL:
		current_expression_value.set(cp_bool(!current_expression_value.b));
		break;
	}

	if (has_assign) {
		auto id = static_cast<parser::ASTIdentifierNode*>(astnode->expr);

		std::string nmspace = get_namespace(id->nmspace);;
		size_t i;
		for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(id->identifier_vector[0].identifier); i--);
		auto id_scope = scopes[nmspace][i];

		Value* value = access_value(id_scope, id_scope->find_declared_variable(id->identifier_vector[0].identifier), id->identifier_vector);

		switch (value->curr_type) {
		case parser::Type::T_BOOL:
			value->set(current_expression_value.b);
			break;
		case parser::Type::T_INT:
			value->set(current_expression_value.i);
			break;
		case parser::Type::T_FLOAT:
			value->set(current_expression_value.f);
			break;
		case parser::Type::T_CHAR:
			value->set(current_expression_value.c);
			break;
		case parser::Type::T_STRING:
			value->set(current_expression_value.s);
			break;
		case parser::Type::T_ARRAY:
			value->set(current_expression_value.arr);
			break;
		case parser::Type::T_STRUCT:
			value->set(current_expression_value.str);
			break;
		}
	}
}

void Interpreter::visit(parser::ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<parser::Type> signature;
	std::vector<std::pair<parser::Type, Value*>> current_function_arguments;

	// for each parameter
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(current_expression_value.curr_type);

		// add the current expr to the local vector of function arguments, to be
		// used in the creation of the function scope
		Value* value = new Value(current_expression_value.curr_type);
		value->copy_from(&current_expression_value);
		current_function_arguments.emplace_back(current_expression_value.curr_type, value);
	}

	// update the global vector current_function_arguments
	for (auto& arg : current_function_arguments) {
		this->current_function_arguments.push_back(arg);
	}

	// determine in which scope the function is declared
	auto nmspace = get_namespace(astnode->nmspace);
	size_t i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(astnode->identifier, signature); i--);
	auto func_scope = scopes[nmspace][i];

	// populate the global vector of function parameter names, to be used in creation of function scope
	current_function_parameters = std::get<1>(func_scope->find_declared_function(astnode->identifier, signature));

	current_name = astnode->identifier;
	current_function_name = astnode->identifier;

	is_function_context = true;
	current_function_nmspace = nmspace;
	// visit the corresponding function block
	std::get<2>(func_scope->find_declared_function(astnode->identifier, signature))->accept(this);
	is_function_context = false;
	current_function_nmspace = "";

	current_name = current_program->name;
}

void Interpreter::visit(parser::ASTTypeParseNode* astnode) {
	// visit expression node to update current value/type
	astnode->expr->accept(this);

	switch (astnode->type) {
	case parser::Type::T_BOOL:
		switch (current_expression_value.curr_type) {
		case parser::Type::T_BOOL:
			break;
		case parser::Type::T_INT:
			current_expression_value.set(cp_bool(current_expression_value.i != 0));
			break;
		case parser::Type::T_FLOAT:
			current_expression_value.set(cp_bool(current_expression_value.f != .0));
			break;
		case parser::Type::T_CHAR:
			current_expression_value.set(cp_bool(current_expression_value.c != '\0'));
			break;
		case parser::Type::T_STRING:
			current_expression_value.set(cp_bool(!current_expression_value.s.empty()));
			break;
		}
		break;

	case parser::Type::T_INT:
		switch (current_expression_value.curr_type) {
		case parser::Type::T_BOOL:
			current_expression_value.set(cp_int(current_expression_value.b));
			break;
		case parser::Type::T_INT:
			break;
		case parser::Type::T_FLOAT:
			current_expression_value.set(cp_int(current_expression_value.f));
			break;
		case parser::Type::T_CHAR:
			current_expression_value.set(cp_int(current_expression_value.c));
			break;
		case parser::Type::T_STRING:
			try {
				current_expression_value.set(cp_int(std::stoll(current_expression_value.s)));
			}
			catch (...) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + current_expression_value.s + "' is not a valid value to parse int");
			}
			break;
		}
		break;

	case parser::Type::T_FLOAT:
		switch (current_expression_value.curr_type) {
		case parser::Type::T_BOOL:
			current_expression_value.set(cp_float(current_expression_value.b));
			break;
		case parser::Type::T_INT:
			current_expression_value.set(cp_float(current_expression_value.i));
			break;
		case parser::Type::T_FLOAT:
			break;
		case parser::Type::T_CHAR:
			current_expression_value.set(cp_float(current_expression_value.c));
			break;
		case parser::Type::T_STRING:
			try {
				current_expression_value.set(cp_float(std::stold(current_expression_value.s)));
			}
			catch (...) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + current_expression_value.s + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case parser::Type::T_CHAR:
		switch (current_expression_value.curr_type) {
		case parser::Type::T_BOOL:
			current_expression_value.set(cp_char(current_expression_value.b));
			break;
		case parser::Type::T_INT:
			current_expression_value.set(cp_char(current_expression_value.i));
			break;
		case parser::Type::T_FLOAT:
			current_expression_value.set(cp_char(current_expression_value.f));
			break;
		case parser::Type::T_CHAR:
			break;
		case parser::Type::T_STRING:
			if (current_expression_value.s.size() > 1) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + current_expression_value.s + "' is not a valid value to parse char");
			}
			else {
				current_expression_value.set(cp_char(current_expression_value.s[0]));
			}
			break;
		}
		break;

	case parser::Type::T_STRING:
		current_expression_value.set(cp_string(parse_value_to_string(current_expression_value)));

	}

	current_expression_value.set_type(astnode->type);
	current_expression_value.set_curr_type(astnode->type);
}


std::string Interpreter::parse_value_to_string(Value value) {
	switch (value.curr_type) {
	case parser::Type::T_VOID:
		return "null";
	case parser::Type::T_BOOL:
		return ((value.b) ? "true" : "false");
	case parser::Type::T_INT:
		return std::to_string(value.i);
	case parser::Type::T_FLOAT:
		return std::to_string(value.f);
	case parser::Type::T_CHAR:
		return cp_string(std::string{ value.c });
	case parser::Type::T_STRING:
		return value.s;
	case parser::Type::T_STRUCT:
		return parse_struct_to_string(*value.str);
	case parser::Type::T_ARRAY:
		return parse_array_to_string(value.arr);
	default:
		throw std::runtime_error("IERR: can't determine value type on parsing");
	}
}

std::string Interpreter::parse_array_to_string(cp_array value) {
	std::stringstream s = std::stringstream();
	s << "[";
	for (auto i = 0; i < value.size(); ++i) {
		s << parse_value_to_string(*value.at(i));
		if (i < value.size() - 1) {
			s << ",";
		}
	}
	s << "]";
	return s.str();
}

std::string Interpreter::parse_struct_to_string(cp_struct value) {
	std::stringstream s = std::stringstream();
	s << value.first + "{";
	for (auto const& [key, val] : value.second) {
		s << key + ":";
		s << parse_value_to_string(*val);
		s << ",";
	}
	s.seekp(-1, std::ios_base::end);
	s << "}";
	return s.str();
}

void Interpreter::visit(parser::ASTPrintNode* astnode) {
	// visit expression node to update current value/type
	astnode->expr->accept(this);
	std::cout << parse_value_to_string(current_expression_value);
}

std::vector<unsigned int> Interpreter::calculate_array_dim_size(cp_array arr) {
	auto dim = std::vector<unsigned int>();

	dim.push_back(arr.size());

	if (is_array(arr.at(0)->type)) {
		auto dim2 = calculate_array_dim_size(arr.at(0)->arr);
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

void Interpreter::visit(parser::ASTTypeNode* astnode) {
	astnode->expr->accept(this);

	auto currentValue = current_expression_value;
	auto dim = std::vector<unsigned int>();
	auto type = is_void(currentValue.curr_type) ? currentValue.type : currentValue.curr_type;
	std::string str_type = "";

	if (is_array(type)) {
		dim = calculate_array_dim_size(currentValue.arr);
		type = currentValue.arr_type;
	}

	str_type = parser::type_str(type);

	if (type == parser::Type::T_STRUCT) {
		if (dim.size() > 0) {
			auto arr = currentValue.arr[0];
			for (size_t i = 0; i < dim.size() - 1; ++i) {
				arr = arr->arr[0];
			}
			str_type = arr->str->first;
		}
		else {
			str_type = currentValue.str->first;
		}
	}

	if (dim.size() > 0) {
		for (size_t i = 0; i < dim.size(); ++i) {
			str_type += "[" + std::to_string(dim[i]) + "]";
		}
	}

	auto value = Value(parser::Type::T_STRING);
	value.set(cp_string(str_type));
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTLenNode* astnode) {
	astnode->expr->accept(this);
	auto val = Value(parser::Type::T_INT);

	if (current_expression_value.curr_type == parser::Type::T_ARRAY) {
		val.set(cp_int(current_expression_value.arr.size()));
	}
	else if (current_expression_value.curr_type == parser::Type::T_STRING) {
		val.set(cp_int(current_expression_value.s.size()));
	}

	current_expression_value = val;
}

void Interpreter::visit(parser::ASTRoundNode* astnode) {
	astnode->expr->accept(this);
	auto val = Value(parser::Type::T_FLOAT);
	val.set(cp_float(roundl(current_expression_value.f)));
	current_expression_value = val;
}


void Interpreter::visit(parser::ASTNullNode* astnode) {
	auto value = Value(parser::Type::T_VOID);
	value.set_null();
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTThisNode* astnode) {
	auto value = Value(parser::Type::T_STRING);
	value.set(cp_string(current_name));
	current_expression_value = value;
}

void Interpreter::visit(parser::ASTReadNode* astnode) {
	std::string line;
	std::getline(std::cin, line);

	current_expression_value.set(cp_string(std::move(line)));
}

std::string Interpreter::msg_header(unsigned int row, unsigned int col) {
	return "(IERR) " + current_program->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
}

unsigned int Interpreter::hash(parser::ASTExprNode* astnode) {
	return 0;
}

unsigned int Interpreter::hash(parser::ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(parser::ASTLiteralNode<cp_int>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(parser::ASTLiteralNode<cp_float>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(parser::ASTLiteralNode<cp_char>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(parser::ASTLiteralNode<cp_string>* astnode) {
	return axe::Util::hashcode(astnode->val);
}

unsigned int Interpreter::hash(parser::ASTIdentifierNode* astnode) {
	// determine innermost scope in which variable is declared
	std::string nmspace = get_namespace(astnode->nmspace);

	size_t i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(astnode->identifier_vector[0].identifier); i--);
	auto id_scope = scopes[nmspace][i];

	Value* value = access_value(id_scope, id_scope->find_declared_variable(astnode->identifier_vector[0].identifier), astnode->identifier_vector);

	switch (value->curr_type) {
	case parser::Type::T_BOOL:
		return static_cast<unsigned int>(value->b);
	case parser::Type::T_INT:
		return static_cast<unsigned int>(value->i);
	case parser::Type::T_FLOAT:
		return static_cast<unsigned int>(value->f);
	case parser::Type::T_CHAR:
		return static_cast<unsigned int>(value->c);
	case parser::Type::T_STRING:
		return axe::Util::hashcode(value->s);
	}
}
