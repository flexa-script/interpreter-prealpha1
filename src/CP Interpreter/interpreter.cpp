#include <iostream>
#include <cmath>

#include "interpreter.hpp"
#include "util.hpp"
#include "watch.h"


using namespace visitor;


Interpreter::Interpreter(InterpreterScope* globalScope, std::vector<parser::ASTProgramNode*> programs)
	: current_expression_value(Value_t(parser::Type::T_ND)),
	Visitor(programs, programs[0], programs[0]) {
	// add global scope
	current_name = current_program->name;
	globalScope->set_parent(this);
	scopes.push_back(globalScope);
}

Interpreter::Interpreter()
	: current_expression_value(Value_t(parser::Type::T_ND)), is_function_context(false),
	Visitor(std::vector<parser::ASTProgramNode*>(), nullptr, nullptr) {
	// add global scope
	scopes.push_back(new InterpreterScope(this, ""));
}

std::string visitor::Interpreter::get_namespace() {
	if (!current_program->alias.empty() && !is_function_context && current_program->alias != main_program->name) {
		return current_program->alias + ".";
	}
	return "";
}

Interpreter::~Interpreter() = default;

void Interpreter::start() {
	visit(current_program);
}

void visitor::Interpreter::visit(parser::ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void visitor::Interpreter::visit(parser::ASTUsingNode* astnode) {
	for (auto program : programs) {
		if (astnode->library == program->name) {
			auto prev_program = current_program;
			current_program = program;
			start();
			current_program = prev_program;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTDeclarationNode* astnode) {
	// visit expression to update current value/type
	if (astnode->expr) {
		astnode->expr->accept(this);
	}

	if (astnode->expr && current_expression_value.curr_type != parser::Type::T_NULL && current_expression_value.has_value) {
		auto type = astnode->type == parser::Type::T_ANY ? current_expression_value.curr_type : astnode->type;
		// declare variable, depending on type
		switch (type) {
		case parser::Type::T_BOOL:
			scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.b);
			break;
		case parser::Type::T_INT:
			scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.i);
			break;
		case parser::Type::T_FLOAT:
			if (current_expression_value.curr_type == parser::Type::T_INT)
				scopes.back()->declare_variable(get_namespace() + astnode->identifier, (cp_float)current_expression_value.i);
			else
				scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.f);
			break;
		case parser::Type::T_CHAR:
			scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.c);
			break;
		case parser::Type::T_STRING:
			if (current_expression_value.curr_type == parser::Type::T_CHAR)
				scopes.back()->declare_variable(get_namespace() + astnode->identifier, std::string{ current_expression_value.c });
			else
				scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.s);
			break;
		case parser::Type::T_ARRAY:
			scopes.back()->declare_variable(get_namespace() + astnode->identifier, current_expression_value.arr);
			break;
		case parser::Type::T_STRUCT:
			declare_structure_variable(get_namespace() + astnode->identifier, current_expression_value);
			break;
		}
	}
	else {
		if (astnode->type == parser::Type::T_STRUCT) {
			auto nllVal = Value_t(parser::Type::T_STRUCT);
			nllVal.set_null();
			nllVal.str.first = astnode->type_name;
			declare_structure_variable(get_namespace() + astnode->identifier, nllVal);
		}
		else {
			scopes.back()->declare_null_variable(get_namespace() + astnode->identifier, astnode->type);
		}
	}

	Value_t* val = scopes.back()->access_value(std::vector<std::string> {get_namespace() + astnode->identifier}, std::vector<parser::ASTExprNode*>());
	val->dim = astnode->dim;
	val->arr_type = astnode->array_type;
}

void Interpreter::declare_structure_variable(std::string identifier_vector, Value_t newValue) {
	Value_t* value = nullptr;
	std::string type_name = newValue.str.first;
	parser::StructureDefinition_t typeStruct;
	int strDefScopeIdx = 0;

	if (newValue.has_value) {
		value = scopes.back()->declare_variable(identifier_vector, newValue.str);
	}
	else {
		value = scopes.back()->declare_null_struct_variable(identifier_vector, type_name);
	}

	for (strDefScopeIdx = scopes.size() - 1; !scopes[strDefScopeIdx]->already_declared_structure_definition(type_name); --strDefScopeIdx);
	typeStruct = scopes[strDefScopeIdx]->find_declared_structure_definition(type_name);

	for (size_t j = 0; j < typeStruct.variables.size(); ++j) {
		bool found = false;
		for (size_t k = 0; k < value->str.second.size(); ++k) {
			if (value->str.second[k].first == typeStruct.variables[j].identifier) {
				found = true;
				value->str.second[k].second->set_type(typeStruct.variables[j].type);
				break;
			}
		}
		if (!found) {
			cp_struct_value strVal;
			strVal.first = typeStruct.variables[j].identifier;
			strVal.second = new Value_t(typeStruct.variables[j].type);
			strVal.second->set_null();
			value->str.second.push_back(strVal);
		}
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

cp_int visitor::Interpreter::do_operation(cp_int lval, cp_int rval, std::string op) {
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
}

cp_float visitor::Interpreter::do_operation(cp_float lval, cp_float rval, std::string op) {
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
}

void visitor::Interpreter::visit(parser::ASTAssignmentNode* astnode) {
	// determine innermost scope in which variable is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(astnode->identifier_vector[0]); i--);

	// visit expression node to update current value/type
	astnode->expr->accept(this);

	Value_t* value = scopes[i]->access_value(astnode->identifier_vector, astnode->access_vector);

	if (current_expression_value.curr_type != parser::Type::T_NULL && current_expression_value.has_value) {
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
	else {
		value->set_null();
	}
}

void Interpreter::print_value(Value_t value) {
	if (value.has_value) {
		switch (value.curr_type) {
		case parser::Type::T_BOOL:
			std::cout << ((value.b) ? "true" : "false");
			break;
		case parser::Type::T_INT:
			std::cout << value.i;
			break;
		case parser::Type::T_FLOAT:
			std::cout << value.f;
			break;
		case parser::Type::T_CHAR:
			std::cout << value.c;
			break;
		case parser::Type::T_STRING:
			std::cout << value.s;
			break;
		case parser::Type::T_STRUCT:
			print_struct(value.str);
			break;
		case parser::Type::T_ARRAY:
			print_array(value.arr);
			break;
		default:
			throw std::runtime_error("IERR: can't determine value type on printing");
		}
	}
	else {
		std::cout << "null";
	}
}

void Interpreter::print_array(cp_array value) {
	std::cout << '[';
	for (auto i = 0; i < value.size(); ++i) {
		print_value(*value.at(i));
		if (i < value.size() - 1) {
			std::cout << ',';
		}
	}
	std::cout << ']';
}

void Interpreter::print_struct(cp_struct value) {
	std::cout << value.first << "{";
	for (auto i = 0; i < value.second.size(); ++i) {
		std::cout << value.second.at(i).first << ":";
		print_value(*value.second.at(i).second);
		if (i < value.second.size() - 1) {
			std::cout << ",";
		}
	}
	std::cout << "}";
}

void visitor::Interpreter::visit(parser::ASTPrintNode* astnode) {
	// visit expression node to update current value/type
	astnode->expr->accept(this);
	print_value(current_expression_value);
}

void visitor::Interpreter::visit(parser::ASTReturnNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
	for (int i = scopes.size() - 1; i >= 0; --i) {
		if (!scopes[i]->get_name().empty()) {
			return_from_function_name = scopes[i]->get_name();
			return_from_function = true;
			break;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTBlockNode* astnode) {
	// create new scope
	scopes.push_back(new InterpreterScope(this, current_function_name));
	current_function_name = "";

	// check whether this is a function block by seeing if we have any current function parameters
	// if we do, then add them to the current scope
	for (unsigned int i = 0; i < current_function_arguments.size(); ++i) {
		switch (current_function_arguments[i].first) {
		case parser::Type::T_BOOL:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->b);
			break;
		case parser::Type::T_INT:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->i);
			break;
		case parser::Type::T_FLOAT:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->f);
			break;
		case parser::Type::T_CHAR:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->c);
			break;
		case parser::Type::T_STRING:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->s);
			break;
		case parser::Type::T_STRUCT:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->str);
			break;
		case parser::Type::T_ARRAY:
			scopes.back()->declare_variable(current_function_parameters[i], current_function_arguments[i].second->arr);
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
			if (!return_from_function_name.empty() && return_from_function_name == scopes.back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	// close scope
	scopes.pop_back();
}

void visitor::Interpreter::visit(parser::ASTContinueNode* astnode) {
	continue_block = true;
}

void visitor::Interpreter::visit(parser::ASTBreakNode* astnode) {
	break_block = true;
}

void visitor::Interpreter::visit(parser::ASTSwitchNode* astnode) {
	is_switch = true;

	// create new scope
	scopes.push_back(new InterpreterScope(this, ""));

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
			if (!return_from_function_name.empty() && return_from_function_name == scopes.back()->get_name()) {
				return_from_function_name = "";
				return_from_function = false;
			}
			break;
		}
	}

	// close scope
	scopes.pop_back();
	is_switch = false;
}

void visitor::Interpreter::visit(parser::ASTElseIfNode* astnode) {
	executed_elif = false;

	// evaluate if condition
	astnode->condition->accept(this);

	bool result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;

	// execute appropriate blocks
	if (result) {
		astnode->block->accept(this);
		executed_elif = true;
	}
}

void visitor::Interpreter::visit(parser::ASTIfNode* astnode) {
	// evaluate if condition
	astnode->condition->accept(this);

	bool result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;

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

void visitor::Interpreter::visit(parser::ASTForNode* astnode) {
	is_loop = true;
	scopes.push_back(new InterpreterScope(this, ""));

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

	bool result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;

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

		result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;
	}

	scopes.pop_back();
	is_loop = false;
}

void visitor::Interpreter::visit(parser::ASTForEachNode* astnode) {
	is_loop = true;

	astnode->collection->accept(this);
	auto colletion = current_expression_value.arr;

	for (auto val : colletion) {
		scopes.push_back(new InterpreterScope(this, ""));

		astnode->itdecl->accept(this);

		auto itdecl = static_cast<parser::ASTDeclarationNode*>(astnode->itdecl);

		// determine innermost scope in which variable is declared
		size_t i;
		for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(itdecl->identifier); --i);

		Value_t* value = scopes[i]->access_value(std::vector<std::string>{ itdecl->identifier }, std::vector<parser::ASTExprNode*>());

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

		scopes.pop_back();

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

void visitor::Interpreter::visit(parser::ASTWhileNode* astnode) {
	is_loop = true;

	// evaluate while condition
	astnode->condition->accept(this);

	bool result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;

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

		result = current_expression_value.curr_type == parser::Type::T_BOOL ? current_expression_value.b : current_expression_value.has_value;
	}

	is_loop = false;
}

void visitor::Interpreter::visit(parser::ASTFunctionDefinitionNode* astnode) {
	// add function to symbol table
	scopes.back()->declare_function(get_namespace() + astnode->identifier, astnode->signature, astnode->variable_names, astnode->block);
}

void visitor::Interpreter::visit(parser::ASTStructDefinitionNode* astnode) {
	scopes.back()->declare_structure_definition(get_namespace() + astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_bool>* lit) {
	Value_t* value = new Value_t(parser::Type::T_BOOL);
	value->set(lit->val);
	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_int>* lit) {
	Value_t* value = new Value_t(parser::Type::T_INT);
	value->set(lit->val);
	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_float>* lit) {
	Value_t* value = new Value_t(parser::Type::T_FLOAT);
	value->set(lit->val);
	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_char>* lit) {
	Value_t* value = new Value_t(parser::Type::T_CHAR);
	value->set(lit->val);
	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_string>* lit) {
	Value_t* value = new Value_t(parser::Type::T_STRING);
	value->set(lit->val);
	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTArrayConstructorNode* astnode) {
	Value_t* value = new Value_t(parser::Type::T_ARRAY);
	cp_array arr = cp_array();

	for (auto& exrp : astnode->values) {
		exrp->accept(this);
		Value_t* arrValue = new Value_t(current_expression_value.curr_type);
		arrValue->copy_from(&current_expression_value);
		arr.push_back(arrValue);
	}

	value->set(arr);

	current_expression_value = *value;
}

void visitor::Interpreter::visit(parser::ASTStructConstructorNode* astnode) {
	Value_t* value = new Value_t(parser::Type::T_STRUCT);

	auto str = cp_struct();
	str.first = astnode->type_name;
	str.second = cp_struct_values();

	for (auto& expr : astnode->values) {
		expr.second->accept(this);
		Value_t* calcValue = new Value_t(current_expression_value.curr_type);
		calcValue->copy_from(&current_expression_value);
		auto strValue = cp_struct_value();
		strValue.first = expr.first;
		strValue.second = calcValue;
		str.second.push_back(strValue);
	}

	declare_structure_definition_first_level_variables(&str);

	value->set(str);
	current_expression_value = *value;
}

void visitor::Interpreter::declare_structure_definition_first_level_variables(cp_struct* str) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(str->first); --i);
	auto typeStruct = scopes[i]->find_declared_structure_definition(str->first);

	for (auto& varTypeStruct : typeStruct.variables) {
		auto found = false;
		for (size_t i = 0; i < str->second.size(); ++i) {
			if (str->second.at(i).first == varTypeStruct.identifier) {
				found = true;
			}
		}
		if (!found) {
			auto strValue = cp_struct_value();
			strValue.first = varTypeStruct.identifier;
			strValue.second = new Value_t(varTypeStruct.type);
			strValue.second->set_null();
			str->second.push_back(strValue);
		}
	}
}

void visitor::Interpreter::visit(parser::ASTBinaryExprNode* astnode) {
	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	parser::Type l_type = current_expression_value.curr_type;
	Value_t l_value = current_expression_value;

	// then right node
	astnode->right->accept(this);
	parser::Type r_type = current_expression_value.curr_type;
	Value_t r_value = current_expression_value;

	// expression struct
	Value_t value = Value_t(parser::Type::T_ND);

	// arithmetic operators for now
	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		// two ints
		if (l_type == parser::Type::T_INT && r_type == parser::Type::T_INT) {
			current_expression_value.curr_type = parser::Type::T_INT;
			if (op == "+") {
				value.set((cp_int)(l_value.i + r_value.i));
			}
			else if (op == "-") {
				value.set((cp_int)(l_value.i - r_value.i));
			}
			else if (op == "*") {
				value.set((cp_int)(l_value.i * r_value.i));
			}
			else if (op == "/") {
				if (r_value.i == 0) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "division by zero encountered");
				}
				value.set((cp_int)(l_value.i / r_value.i));
			}
			else if (op == "%") {
				value.set((cp_int)(l_value.i % r_value.i));
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
				value.set((cp_float)(l + r));
			}
			else if (op == "-") {
				value.set((cp_float)(l - r));
			}
			else if (op == "*") {
				value.set((cp_float)(l * r));
			}
			else if (op == "/") {
				if (r == 0) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "division by zero encountered");
				}
				value.set((cp_float)l / r);
			}
		}
		else if (l_type == parser::Type::T_CHAR && r_type == parser::Type::T_STRING) { // char and string
			current_expression_value.curr_type = parser::Type::T_STRING;
			value.set(cp_string(std::string{ l_value.c } + r_value.s));
		}
		else if (l_type == parser::Type::T_STRING && r_type == parser::Type::T_CHAR) { // string and char
			current_expression_value.curr_type = parser::Type::T_STRING;
			value.set(cp_string(l_value.s + std::string{ r_value.c }));
		}
		else if (l_type == parser::Type::T_CHAR && r_type == parser::Type::T_CHAR) { // char and string
			current_expression_value.curr_type = parser::Type::T_STRING;
			value.set(cp_string(std::string{ l_value.c } + std::string{ r_value.c }));
		}
		else { // remaining case is for strings
			current_expression_value.curr_type = parser::Type::T_STRING;
			value.set(cp_string(l_value.s + r_value.s));
		}
	}
	else if (op == "and" || op == "or") { // now bool
		current_expression_value.curr_type = parser::Type::T_BOOL;

		cp_bool l = l_value.b;
		cp_bool r = r_value.b;

		if (l_type == parser::Type::T_STRUCT) {
			l = l_value.has_value;
		}
		if (r_type == parser::Type::T_STRUCT) {
			r = r_value.has_value;
		}

		if (op == "and") {
			value.set((cp_bool)(l && r));
		}
		else if (op == "or") {
			value.set((cp_bool)(l || r));
		}
	}
	else { // now comparator operators
		current_expression_value.curr_type = parser::Type::T_BOOL;

		if (l_type == parser::Type::T_BOOL || l_type == parser::Type::T_STRUCT) {

			cp_bool l = l_value.b;
			cp_bool r = r_value.b;

			if (l_type == parser::Type::T_STRUCT) {
				l = l_value.has_value;
			}
			if (r_type == parser::Type::T_STRUCT) {
				r = r_value.has_value;
			}

			value.set((cp_bool)((op == "==") ? l_value.b == r_value.b : l_value.b != r_value.b));
		}
		else if (l_type == parser::Type::T_STRING) {
			value.set((cp_bool)((op == "==") ? l_value.s == r_value.s : l_value.s != r_value.s));
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
				value.set((cp_bool)(l == r));
			}
			else if (op == "!=") {
				value.set((cp_bool)(l != r));
			}
			else if (op == "<") {
				value.set((cp_bool)(l < r));
			}
			else if (op == ">") {
				value.set((cp_bool)(l > r));
			}
			else if (op == ">=") {
				value.set((cp_bool)(l >= r));
			}
			else if (op == "<=") {
				value.set((cp_bool)(l <= r));
			}
		}
	}

	// update current expression
	current_expression_value = value;
}

bool visitor::Interpreter::is_namespace(std::string identifier) {
	for (auto& prog : programs) {
		if (prog->alias == identifier) {
			return true;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTIdentifierNode* astnode) {

	// determine innermost scope in which variable is declared
	long long i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(astnode->identifier_vector[0]); --i) {
		if (i <= 0) {
			if (is_namespace(astnode->identifier_vector[0])) {
				astnode->identifier_vector[0] = astnode->identifier_vector[0] + "." + astnode->identifier_vector[1];
				astnode->identifier_vector.erase(std::next(astnode->identifier_vector.begin()));
			}
			for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(astnode->identifier_vector[0]); --i);
			break;
		}
	}

	current_expression_value = *scopes[i]->access_value(astnode->identifier_vector, astnode->access_vector);

	if (current_expression_value.curr_type == parser::Type::T_STRING && astnode->access_vector.size() > 0 && scopes[i]->has_string_access) {
		astnode->access_vector[astnode->access_vector.size() - 1]->accept(this);
		auto pos = current_expression_value.i;

		if (pos >= current_expression_value.s.size()) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "tryed to access a invalid position in a string");
		}

		auto charValue = Value_t(current_expression_value.curr_type);
		charValue.set(cp_char(current_expression_value.s[pos]));
		current_expression_value = charValue;
	}
}

void visitor::Interpreter::visit(parser::ASTUnaryExprNode* astnode) {
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
			current_expression_value.set(cp_int(--current_expression_value.f));
			has_assign = true;
		}
		else if (astnode->unary_op == "++") {
			current_expression_value.set(cp_int(++current_expression_value.f));
			has_assign = true;
		}
		break;
	case parser::Type::T_BOOL:
		current_expression_value.set(cp_bool(!current_expression_value.b));
		break;
	case parser::Type::T_STRUCT:
		current_expression_value.set(cp_bool(!current_expression_value.has_value));
		break;
	}

	if (has_assign) {
		auto id = static_cast<parser::ASTIdentifierNode*>(astnode->expr);

		size_t i;
		for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(id->identifier_vector[0]); i--);

		Value_t* value = scopes[i]->access_value(id->identifier_vector, id->access_vector);

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

void visitor::Interpreter::visit(parser::ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<parser::Type> signature;
	std::vector<std::pair<parser::Type, Value_t*>> current_function_arguments;

	// for each parameter
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(current_expression_value.curr_type);

		// add the current expr to the local vector of function arguments, to be
		// used in the creation of the function scope
		Value_t* value = new Value_t(current_expression_value.curr_type);
		value->copy_from(&current_expression_value);
		current_function_arguments.emplace_back(current_expression_value.curr_type, value);
	}

	// update the global vector current_function_arguments
	for (auto arg : current_function_arguments) {
		this->current_function_arguments.push_back(arg);
	}

	// determine in which scope the function is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_function(astnode->identifier, signature); i--);

	// populate the global vector of function parameter names, to be used in creation of function scope
	current_function_parameters = std::get<1>(scopes[i]->find_declared_function(astnode->identifier, signature));

	current_name = astnode->identifier;
	current_function_name = astnode->identifier;

	is_function_context = true;
	// visit the corresponding function block
	std::get<2>(scopes[i]->find_declared_function(astnode->identifier, signature))->accept(this);
	is_function_context = false;

	current_name = current_program->name;
}

void visitor::Interpreter::visit(parser::ASTTypeParseNode* astnode) {
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


std::string Interpreter::parse_value_to_string(Value_t value) {
	if (value.has_value) {
		switch (value.curr_type) {
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
			return parse_struct_to_string(value.str);
		case parser::Type::T_ARRAY:
			return parse_array_to_string(value.arr);
		default:
			throw std::runtime_error("IERR: can't determine value type on printing");
		}
	}
	return "null";
}

std::string Interpreter::parse_array_to_string(cp_array value) {
	std::string s = "[";
	for (auto i = 0; i < value.size(); ++i) {
		s += parse_value_to_string(*value.at(i));
		if (i < value.size() - 1) {
			s += ",";
		}
	}
	s += "]";
	return s;
}

std::string Interpreter::parse_struct_to_string(cp_struct value) {
	std::string s = value.first + "{";
	for (auto i = 0; i < value.second.size(); ++i) {
		s += value.second.at(i).first + ":";
		s += parse_value_to_string(*value.second.at(i).second);
		if (i < value.second.size() - 1) {
			s += ",";
		}
	}
	s += "}";
	return s;
}

void visitor::Interpreter::visit(parser::ASTTypeNode* astnode) {
	astnode->expr->accept(this);

	auto currentValue = current_expression_value;

	auto dim = evaluate_access_vector(currentValue.dim);

	auto type = currentValue.curr_type;
	auto strT = parser::type_str(type);

	if (type == parser::Type::T_ARRAY) {
		strT = parser::type_str(currentValue.arr_type);
		for (size_t i = 0; i < dim.size(); ++i) {
			strT += "[" + std::to_string(dim[i]) + "]";
		}
	}
	else if (type == parser::Type::T_STRUCT) {
		strT += "<" + currentValue.str.first + ">";
	}

	Value_t value = Value_t(parser::Type::T_STRING);
	value.set(cp_string(strT));
	current_expression_value = value;
}

void visitor::Interpreter::visit(parser::ASTLenNode* astnode) {
	astnode->expr->accept(this);
	Value_t val = Value_t(parser::Type::T_INT);

	if (current_expression_value.curr_type == parser::Type::T_ARRAY) {
		val.set(cp_int(current_expression_value.arr.size()));
	}
	else if (current_expression_value.curr_type == parser::Type::T_STRING) {
		val.set(cp_int(current_expression_value.s.size()));
	}

	current_expression_value = val;
}

void visitor::Interpreter::visit(parser::ASTRoundNode* astnode) {
	astnode->expr->accept(this);
	Value_t val = Value_t(parser::Type::T_FLOAT);
	val.set(cp_float(roundl(current_expression_value.f)));
	current_expression_value = val;
}


void visitor::Interpreter::visit(parser::ASTNullNode* astnode) {
	Value_t value = Value_t(parser::Type::T_NULL);
	value.set_null();
	current_expression_value = value;
}

void visitor::Interpreter::visit(parser::ASTThisNode* astnode) {
	Value_t value = Value_t(parser::Type::T_STRING);
	value.set(cp_string(current_name));
	current_expression_value = value;
}

void visitor::Interpreter::visit(parser::ASTReadNode* astnode) {
	std::string line;
	std::getline(std::cin, line);

	current_expression_value.set(cp_string(std::move(line)));
}

std::pair<parser::Type, Value_t*> Interpreter::current_expr() {
	return std::move(std::make_pair(current_expression_value.curr_type, &current_expression_value));
};

std::string Interpreter::msg_header(unsigned int row, unsigned int col) {
	return "(IERR) " + current_program->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
}

unsigned int visitor::Interpreter::hash(parser::ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int visitor::Interpreter::hash(parser::ASTLiteralNode<cp_int>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int visitor::Interpreter::hash(parser::ASTLiteralNode<cp_float>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int visitor::Interpreter::hash(parser::ASTLiteralNode<cp_char>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int visitor::Interpreter::hash(parser::ASTLiteralNode<cp_string>* astnode) {
	return axe::Util::hashcode(astnode->val);
}

unsigned int visitor::Interpreter::hash(parser::ASTIdentifierNode* astnode) {
	// determine innermost scope in which variable is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(astnode->identifier_vector[0]); i--);

	Value_t* value = scopes[i]->access_value(astnode->identifier_vector, astnode->access_vector);

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
