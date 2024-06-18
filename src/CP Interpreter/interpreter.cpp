#include <iostream>
#include <cmath>
#include <conio.h>

#include "interpreter.hpp"
#include "vendor/util.hpp"
#include "vendor/watch.h"
#include "graphics.hpp"
#include "files.hpp"
#include "console.hpp"


using namespace visitor;
using namespace parser;


Interpreter::Interpreter(InterpreterScope* global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: current_expression_value(Value(Type::T_UNDEFINED)), is_function_context(false), current_name(std::stack<std::string>()),
	Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	if (main_program) {
		current_name.push(main_program->name);
	}
	scopes[default_namespace].push_back(global_scope);
	register_built_in_functions();
}

std::string Interpreter::get_namespace(std::string nmspace) {
	return get_namespace(current_program, nmspace);
}

std::string Interpreter::get_namespace(ASTProgramNode* program, std::string nmspace) {
	return nmspace.empty() ? (
		current_function_nmspace.size() == 0 ? (
			program->alias.empty() ? default_namespace : program->alias
			) : current_function_nmspace.top()
		) : nmspace;
}

std::string Interpreter::get_current_namespace() {
	return current_function_nmspace.size() == 0 ? (
		current_program->alias.empty() ? default_namespace : current_program->alias
		) : current_function_nmspace.top();
}

void Interpreter::start() {
	visit(current_program);
}

void Interpreter::visit(ASTProgramNode* astnode) {
	for (auto& statement : astnode->statements) {
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
			else {
				exception = true;
				throw std::runtime_error(msg_header() + ex.what());
			}
		}
	}
	current_expression_value = Value(Type::T_UNDEFINED);
}

void Interpreter::visit(ASTUsingNode* astnode) {
	std::string libname = axe::Util::join(astnode->library, ".");

	if (axe::Util::contains(built_in_libs, libname)) {
		register_built_in_lib(libname);
	}

	auto program = programs[libname];

	// add lib to current program
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::Util::contains(libs, libname)) {
		libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		program_nmspaces[get_current_namespace()].push_back(default_namespace);
		if (!program->alias.empty()) {
			scopes[program->alias].push_back(new InterpreterScope());
		}
		start();
		current_program = prev_program;
	}
}

void Interpreter::visit(ASTAsNamespaceNode* astnode) {
	program_nmspaces[get_namespace(current_program->alias)].push_back(astnode->nmspace);
}

void Interpreter::visit(ASTDeclarationNode* astnode) {
	auto nmspace = get_namespace();

	if (astnode->expr) {
		astnode->expr->accept(this);
	}

	if (astnode->expr && current_expression_value.has_value()) {
		auto type = astnode->type == Type::T_ANY ? current_expression_value.curr_type : astnode->type;
		switch (type) {
		case Type::T_BOOL:
			scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.b);
			break;
		case Type::T_INT:
			scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.i);
			break;
		case Type::T_FLOAT:
			if (current_expression_value.curr_type == Type::T_INT) {
				scopes[nmspace].back()->declare_variable(astnode->identifier, (cp_float)current_expression_value.i);
			}
			else {
				scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.f);
			}
			break;
		case Type::T_CHAR:
			scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.c);
			break;
		case Type::T_STRING:
			if (current_expression_value.curr_type == Type::T_CHAR) {
				scopes[nmspace].back()->declare_variable(astnode->identifier, std::string{ current_expression_value.c });
			}
			else {
				scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.s);
			}
			break;
		case Type::T_ARRAY: {
			if (current_expression_value.arr.size() == 1) {
				auto arr = build_array(astnode->dim, current_expression_value.arr[0], astnode->dim.size() - 1);
				scopes[nmspace].back()->declare_variable(astnode->identifier, arr);
			}
			else {
				scopes[nmspace].back()->declare_variable(astnode->identifier, current_expression_value.arr);
			}
			break;
		}
		case Type::T_STRUCT:
			declare_new_structure(astnode->identifier, current_expression_value);
			break;
		}
	}
	else {
		if (is_void(current_expression_value.curr_type)) {
			if (astnode->type == Type::T_STRUCT) {
				auto undefvalue = Value(Type::T_STRUCT);
				undefvalue.set_null();
				undefvalue.str->first = astnode->type_name;
				declare_new_structure(astnode->identifier, undefvalue);
			}
			else {
				scopes[nmspace].back()->declare_null_variable(astnode->identifier, astnode->type);
			}
		}
		else {
			if (astnode->type == Type::T_STRUCT) {
				auto nllvalue = Value(Type::T_STRUCT);
				nllvalue.set_undefined();
				nllvalue.str->first = astnode->type_name;
				declare_new_structure(astnode->identifier, nllvalue);
			}
			else {
				scopes[nmspace].back()->declare_undef_variable(astnode->identifier, astnode->type);
			}
		}
	}

	Value* val = access_value(scopes[nmspace].back(), scopes[nmspace].back()->find_declared_variable(astnode->identifier),
		std::vector<Identifier>{Identifier(astnode->identifier, std::vector<ASTExprNode*>())});
	val->arr_type = astnode->array_type;
}

void Interpreter::visit(ASTEnumNode* astnode) {
	auto nmspace = get_namespace();
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		scopes[nmspace].back()->declare_variable(astnode->identifiers[i], (cp_int)i);
	}
}

void Interpreter::visit(ASTAssignmentNode* astnode) {
	InterpreterScope* astscope;
	try {
		auto nmspace = get_namespace(astnode->nmspace);
		astscope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	Value* root = astscope->find_declared_variable(astnode->identifier_vector[0].identifier);
	Value* value = access_value(astscope, root, astnode->identifier_vector);

	astnode->expr->accept(this);

	if (is_undefined(value->curr_type)) {
		value->set_curr_type(is_any(value->type) ? current_expression_value.curr_type : value->type);
	}

	if (current_expression_value.has_value()) {
		switch (current_expression_value.curr_type) {
		case Type::T_BOOL:
			value->set(current_expression_value.b);
			break;
		case Type::T_INT:
			value->set(do_operation(value->i, current_expression_value.i, astnode->op));
			break;
		case Type::T_FLOAT:
			value->set(do_operation(value->f, current_expression_value.f, astnode->op));
			break;
		case Type::T_CHAR:
			if (is_string(value->curr_type)) {
				if (astnode->identifier_vector.back().access_vector.size() > 0 && has_string_access) {
					has_string_access = false;
					auto c = current_expression_value.c;
					astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1]->accept(this);
					auto pos = current_expression_value.i;
					auto str = value->s;
					str[pos] = c;
					value->set(cp_string(str));
				}
				else {
					value->set(do_operation(value->s, std::string{ current_expression_value.c }, astnode->op));
				}
			}
			else {
				value->set(current_expression_value.c);
			}
			break;
		case Type::T_STRING:
			value->set(do_operation(value->s, current_expression_value.s, astnode->op));
			break;
		case Type::T_ARRAY:
			value->set(current_expression_value.arr);
			break;
		case Type::T_STRUCT:
			value->set(current_expression_value.str);
		}
	}
	else {
		value->set_null();
	}
}

void Interpreter::visit(ASTReturnNode* astnode) {
	auto nmspace = get_namespace();
	astnode->expr->accept(this);
	for (long long i = scopes[nmspace].size() - 1; i >= 0; --i) {
		if (!scopes[nmspace][i]->get_name().empty()) {
			return_from_function_name = scopes[nmspace][i]->get_name();
			return_from_function = true;
			break;
		}
	}
}

void Interpreter::visit(ASTExitNode* astnode) {
	astnode->exit_code->accept(this);
	exit_from_program = true;
}

void Interpreter::visit(ASTContinueNode* astnode) {
	astnode->accept(this);
	continue_block = true;
}

void Interpreter::visit(ASTBreakNode* astnode) {
	break_block = true;
}

void Interpreter::visit(ASTSwitchNode* astnode) {
	auto nmspace = get_namespace();
	is_switch = true;

	scopes[nmspace].push_back(new InterpreterScope(""));

	int pos = -1;

	astnode->condition->accept(this);

	try {
		auto hash = astnode->condition->hash(this);
		pos = astnode->parsed_case_blocks->at(hash);
	}
	catch (...) {
		pos = astnode->default_block;
	}

	for (int i = pos; i < astnode->statements->size(); ++i) {
		astnode->statements->at(i)->accept(this);

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
	is_switch = false;
}

void Interpreter::visit(ASTElseIfNode* astnode) {
	executed_elif = false;

	astnode->condition->accept(this);

	bool result = current_expression_value.b;

	if (result) {
		astnode->block->accept(this);
		executed_elif = true;
	}
}

void Interpreter::visit(ASTIfNode* astnode) {
	astnode->condition->accept(this);

	bool result = current_expression_value.b;

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

void Interpreter::visit(ASTForNode* astnode) {
	auto nmspace = get_namespace();
	is_loop = true;
	scopes[nmspace].push_back(new InterpreterScope(""));

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);
	}
	else {
		current_expression_value = Value(Type::T_BOOL);
		current_expression_value.set(true);
	}

	bool result = current_expression_value.b;

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

		if (astnode->dci[2]) {
			astnode->dci[2]->accept(this);
		}

		if (astnode->dci[1]) {
			astnode->dci[1]->accept(this);
		}
		else {
			current_expression_value = Value(Type::T_BOOL);
			current_expression_value.set(true);
		}

		result = current_expression_value.b;
	}

	scopes[nmspace].pop_back();
	is_loop = false;
}

void Interpreter::visit(ASTForEachNode* astnode) {
	auto nmspace = get_namespace();
	is_loop = true;

	astnode->collection->accept(this);
	auto colletion = current_expression_value.arr;

	for (auto val : colletion) {
		scopes[nmspace].push_back(new InterpreterScope(""));

		astnode->itdecl->accept(this);

		auto itdecl = static_cast<ASTDeclarationNode*>(astnode->itdecl);

		Value* value = access_value(scopes[nmspace].back(), scopes[nmspace].back()->find_declared_variable(itdecl->identifier),
			std::vector<Identifier>{Identifier(itdecl->identifier, std::vector<ASTExprNode*>())});

		switch (val->curr_type) {
		case Type::T_BOOL:
			value->set(val->b);
			break;
		case Type::T_INT:
			value->set(val->i);
			break;
		case Type::T_FLOAT:
			value->set(val->f);
			break;
		case Type::T_CHAR:
			value->set(val->c);
			break;
		case Type::T_STRING:
			value->set(val->s);
			break;
		case Type::T_ARRAY:
			value->set(val->arr);
			break;
		case Type::T_STRUCT:
			value->set(val->str);
			break;
		}

		astnode->block->accept(this);

		scopes[nmspace].pop_back();

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
	}

	is_loop = false;
}

void Interpreter::visit(ASTTryCatchNode* astnode) {
	auto nmspace = get_namespace();

	try {
		scopes[nmspace].push_back(new InterpreterScope(""));
		astnode->try_block->accept(this);
		scopes[nmspace].pop_back();
	}
	catch (std::exception ex) {
		scopes[nmspace].push_back(new InterpreterScope(""));

		astnode->decl->accept(this);

		if (auto itdecl = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
			Value* value = access_value(scopes[nmspace].back(), scopes[nmspace].back()->find_declared_variable(itdecl->identifier),
				std::vector<Identifier>{Identifier(itdecl->identifier, std::vector<ASTExprNode*>())});

			value->set_type(Type::T_STRUCT);
			value->set_curr_type(Type::T_STRUCT);
			value->str = new cp_struct();
			value->str->first = "Exception";
			value->str->second["error"] = new Value(Type::T_STRING);
			value->str->second["error"]->s = ex.what();
		}

		astnode->catch_block->accept(this);
		scopes[nmspace].pop_back();
	}
}

void Interpreter::visit(parser::ASTThrowNode* astnode) {
	astnode->accept(this);
	throw std::exception(current_expression_value.str->second["error"]->s.c_str());
}

void Interpreter::visit(parser::ASTReticencesNode* astnode) {
	auto value = Value(Type::T_UNDEFINED);
	value.set_undefined();
	current_expression_value = value;
}

void Interpreter::visit(ASTWhileNode* astnode) {
	is_loop = true;

	astnode->condition->accept(this);

	bool result = current_expression_value.b;

	while (result) {
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

		astnode->condition->accept(this);

		result = current_expression_value.b;
	}

	is_loop = false;
}

void Interpreter::visit(ASTDoWhileNode* astnode) {
	is_loop = true;

	bool result = false;

	do {
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

		astnode->condition->accept(this);

		result = current_expression_value.b;
	} while (result);

	is_loop = false;
}

void Interpreter::visit(ASTStructDefinitionNode* astnode) {
	scopes[get_namespace()].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void Interpreter::visit(ASTLiteralNode<cp_bool>* lit) {
	auto value = Value(Type::T_BOOL);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_int>* lit) {
	auto value = Value(Type::T_INT);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_float>* lit) {
	auto value = Value(Type::T_FLOAT);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_char>* lit) {
	auto value = Value(Type::T_CHAR);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTLiteralNode<cp_string>* lit) {
	auto value = Value(Type::T_STRING);
	value.set(lit->val);
	current_expression_value = value;
}

void Interpreter::visit(ASTArrayConstructorNode* astnode) {
	auto value = Value(Type::T_ARRAY);
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

void Interpreter::visit(ASTStructConstructorNode* astnode) {
	auto value = Value(Type::T_STRUCT);

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
	current_expression_nmspace = astnode->nmspace;

	value.set(str);
	current_expression_value = value;
}

void Interpreter::visit(ASTBinaryExprNode* astnode) {
	std::string op = astnode->op;

	astnode->left->accept(this);
	Type l_type = current_expression_value.curr_type;
	Value l_value = current_expression_value;

	astnode->right->accept(this);
	Type r_type = current_expression_value.curr_type;
	Value r_value = current_expression_value;

	auto value = new Value(Type::T_UNDEFINED);

	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		if (l_type == Type::T_INT && r_type == Type::T_INT) {
			current_expression_value.curr_type = Type::T_INT;
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
					set_curr_pos(astnode->row, astnode->col);
					throw std::runtime_error("division by zero encountered");
				}
				value->set((cp_int)(l_value.i / r_value.i));
			}
			else if (op == "%") {
				value->set((cp_int)(l_value.i % r_value.i));
			}
		}
		else if (l_type == Type::T_FLOAT || r_type == Type::T_FLOAT) {
			current_expression_value.curr_type = Type::T_FLOAT;
			cp_float l = l_value.f, r = r_value.f;
			if (l_type == Type::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == Type::T_INT) {
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
					set_curr_pos(astnode->row, astnode->col);
					throw std::runtime_error("division by zero encountered");
				}
				value->set((cp_float)l / r);
			}
		}
		else if (l_type == Type::T_CHAR && r_type == Type::T_STRING) {
			current_expression_value.curr_type = Type::T_STRING;
			value->set(cp_string(std::string{ l_value.c } + r_value.s));
		}
		else if (l_type == Type::T_STRING && r_type == Type::T_CHAR) {
			current_expression_value.curr_type = Type::T_STRING;
			value->set(cp_string(l_value.s + std::string{ r_value.c }));
		}
		else if (l_type == Type::T_CHAR && r_type == Type::T_CHAR) {
			current_expression_value.curr_type = Type::T_STRING;
			value->set(cp_string(std::string{ l_value.c } + std::string{ r_value.c }));
		}
		else {
			current_expression_value.curr_type = Type::T_STRING;
			value->set(cp_string(l_value.s + r_value.s));
		}
	}
	else if (op == "and" || op == "or") {
		current_expression_value.curr_type = Type::T_BOOL;

		cp_bool l = l_value.b;
		cp_bool r = r_value.b;

		if (l_type == Type::T_STRUCT) {
			l = l_value.has_value();
		}
		if (r_type == Type::T_STRUCT) {
			r = r_value.has_value();
		}

		if (op == "and") {
			value->set((cp_bool)(l && r));
		}
		else if (op == "or") {
			value->set((cp_bool)(l || r));
		}
	}
	else {
		current_expression_value.curr_type = Type::T_BOOL;

		if (l_type == Type::T_VOID || r_type == Type::T_VOID) {
			value->set((cp_bool)((op == "==") ? match_type(l_value.curr_type, r_value.curr_type) : !match_type(l_value.curr_type, r_value.curr_type)));
		}
		else if (l_type == Type::T_BOOL) {
			cp_bool l = l_value.b;
			cp_bool r = r_value.b;

			value->set((cp_bool)((op == "==") ? l_value.b == r_value.b : l_value.b != r_value.b));
		}
		else if (l_type == Type::T_STRING) {
			value->set((cp_bool)((op == "==") ? l_value.s == r_value.s : l_value.s != r_value.s));
		}
		else if (l_type == Type::T_ARRAY) {
			value->set((cp_bool)((op == "==") ? l_value.arr == r_value.arr : l_value.arr != r_value.arr));
		}
		else if (l_type == Type::T_STRUCT) {
			value->set((cp_bool)((op == "==") ? l_value.str == r_value.str : l_value.str != r_value.str));
		}
		else {
			cp_float l = l_value.f, r = r_value.f;

			if (l_type == Type::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == Type::T_INT) {
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

	value->set_type(value->curr_type);
	current_expression_value = *value;
}

void Interpreter::visit(ASTUnaryExprNode* astnode) {
	bool has_assign = false;

	astnode->expr->accept(this);
	switch (current_expression_value.curr_type) {
	case Type::T_INT:
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
	case Type::T_FLOAT:
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
	case Type::T_BOOL:
		current_expression_value.set(cp_bool(!current_expression_value.b));
		break;
	}

	if (has_assign) {
		auto id = static_cast<ASTIdentifierNode*>(astnode->expr);

		std::string nmspace = get_namespace(id->nmspace);;
		InterpreterScope* id_scope;
		try {
			id_scope = get_inner_most_variable_scope(nmspace, id->identifier_vector[0].identifier);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}

		Value* value = access_value(id_scope, id_scope->find_declared_variable(id->identifier_vector[0].identifier), id->identifier_vector);

		switch (value->curr_type) {
		case Type::T_BOOL:
			value->set(current_expression_value.b);
			break;
		case Type::T_INT:
			value->set(current_expression_value.i);
			break;
		case Type::T_FLOAT:
			value->set(current_expression_value.f);
			break;
		case Type::T_CHAR:
			value->set(current_expression_value.c);
			break;
		case Type::T_STRING:
			value->set(current_expression_value.s);
			break;
		case Type::T_ARRAY:
			value->set(current_expression_value.arr);
			break;
		case Type::T_STRUCT:
			value->set(current_expression_value.str);
			break;
		}
	}
}

void Interpreter::visit(ASTIdentifierNode* astnode) {
	auto identifier = astnode->identifier_vector[0].identifier;
	auto nmspace = get_namespace(astnode->nmspace);
	InterpreterScope* id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, identifier);
	}
	catch (...) {
		auto dim = astnode->identifier_vector[0].access_vector;
		auto type = Type::T_UNDEFINED;
		auto expression_value = new Value(Type::T_UNDEFINED);

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
			InterpreterScope* curr_scope;
			try {
				curr_scope = get_inner_most_struct_definition_scope(nmspace, identifier);
			}
			catch (...) {
				try {
					curr_scope = get_inner_most_function_scope(nmspace, identifier, std::vector<TypeDefinition>());
					auto fun = cp_function();
					fun.first = curr_scope;
					fun.second = identifier;
					current_expression_value = Value(Type::T_FUNCTION);
					current_expression_value.set(fun);
					return;
				}
				catch (...) {
					set_curr_pos(astnode->row, astnode->col);
					throw std::runtime_error("identifier '" + identifier + "' was not declared");
				}
			}
			current_expression_nmspace = nmspace;
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

	auto root = id_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	auto sub_val = access_value(id_scope, root, astnode->identifier_vector);
	current_expression_value = *sub_val;
	if ((is_reference || is_struct(root->curr_type)) && !sub_val || sub_val == root) {
		current_variable = root;
	}

	if (is_struct(current_expression_value.curr_type)) {
		current_expression_nmspace = nmspace;
	}

	if (current_expression_value.curr_type == Type::T_STRING && astnode->identifier_vector.back().access_vector.size() > 0 && has_string_access) {
		has_string_access = false;
		auto str = current_expression_value.s;
		astnode->identifier_vector.back().access_vector[astnode->identifier_vector.back().access_vector.size() - 1]->accept(this);
		auto pos = current_expression_value.i;

		auto char_value = Value(Type::T_CHAR);
		char_value.set(cp_char(str[pos]));
		current_expression_value = char_value;
	}
}

void Interpreter::visit(ASTTernaryNode* astnode) {
	astnode->condition->accept(this);
	if (current_expression_value.b) {
		astnode->value_if_true->accept(this);
	}
	else {
		astnode->value_if_false->accept(this);
	}
}

void Interpreter::visit(ASTInNode* astnode) {
	astnode->value->accept(this);
	auto expr_val = current_expression_value;
	astnode->collection->accept(this);
	bool res = false;

	if (is_array(current_expression_value.curr_type)) {
		auto expr_col = current_expression_value.arr;

		is_vbv = astnode->vbv;
		for (auto it : expr_col) {
			res = equals_value(&expr_val, it);
			if (res) {
				break;
			}
		}
	}
	else {
		auto expr_col = current_expression_value.s;

		if (is_char(expr_val.curr_type)) {
			res = current_expression_value.s.find(expr_val.c) != std::string::npos;
		}
		else {
			res = current_expression_value.s.find(expr_val.s) != std::string::npos;
		}
	}
	is_vbv = false;

	auto value = Value(Type::T_BOOL);
	value.set(res);
	current_expression_value = value;
}

void Interpreter::visit(ASTFunctionDefinitionNode* astnode) {
	interpreter_parameter_list_t params;
	for (size_t i = 0; i < astnode->parameters.size(); ++i)	{
		interpreter_parameter_t param = std::make_tuple(astnode->variable_names[i], astnode->signature[i], astnode->parameters[i].default_value, astnode->parameters[i].is_rest);
		params.push_back(param);
	}
	scopes[get_namespace()].back()->declare_function(astnode->identifier, params, astnode->block);
}

void Interpreter::visit(ASTFunctionCallNode* astnode) {
	auto nmspace = get_namespace(astnode->nmspace);

	std::vector<TypeDefinition> signature;
	std::vector<Value*> function_arguments;

	for (auto param : astnode->parameters) {
		is_reference = param.first;
		param.second->accept(this);
		Value* reference_value = dynamic_cast<parser::ASTIdentifierNode*>(param.second) ? current_variable : nullptr;
		current_variable = nullptr;

		auto td = TypeDefinition(current_expression_value.curr_type, current_expression_value.arr_type,
			std::vector<ASTExprNode*>(), "", "");
		signature.push_back(td);

		Value* value = new Value(&current_expression_value);
		function_arguments.push_back(reference_value ? reference_value : value);
	}

	for (size_t i = 0; i < function_arguments.size(); ++i) {
		last_function_arguments.push_back(function_arguments[i]);
	}

	InterpreterScope* func_scope = get_inner_most_function_scope(nmspace, astnode->identifier, signature);

	function_call_parameters = func_scope->find_declared_function(astnode->identifier, signature).first;

	is_function_context = true;
	function_call_name = astnode->identifier;
	current_name.push(astnode->identifier);
	current_function_nmspace.push(nmspace);

	auto block = func_scope->find_declared_function(astnode->identifier, signature).second;
	if (block) {
		block->accept(this);
	}
	else {
		call_builtin_function(astnode->identifier);
	}

	current_function_nmspace.pop();
	current_name.pop();
	is_function_context = false;
}

void Interpreter::visit(ASTBlockNode* astnode) {
	auto nmspace = get_namespace();
	scopes[nmspace].push_back(new InterpreterScope(function_call_name));
	function_call_name = "";

	declare_function_block_parameters(nmspace);

	for (auto& stmt : astnode->statements) {
		stmt->accept(this);

		if (exit_from_program) {
			return;
		}

		if (continue_block && (is_loop || is_switch)) {
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

void Interpreter::visit(ASTTypeParseNode* astnode) {
	astnode->expr->accept(this);

	switch (astnode->type) {
	case Type::T_BOOL:
		switch (current_expression_value.curr_type) {
		case Type::T_BOOL:
			break;
		case Type::T_INT:
			current_expression_value.set(cp_bool(current_expression_value.i != 0));
			break;
		case Type::T_FLOAT:
			current_expression_value.set(cp_bool(current_expression_value.f != .0));
			break;
		case Type::T_CHAR:
			current_expression_value.set(cp_bool(current_expression_value.c != '\0'));
			break;
		case Type::T_STRING:
			current_expression_value.set(cp_bool(!current_expression_value.s.empty()));
			break;
		}
		break;

	case Type::T_INT:
		switch (current_expression_value.curr_type) {
		case Type::T_BOOL:
			current_expression_value.set(cp_int(current_expression_value.b));
			break;
		case Type::T_INT:
			break;
		case Type::T_FLOAT:
			current_expression_value.set(cp_int(current_expression_value.f));
			break;
		case Type::T_CHAR:
			current_expression_value.set(cp_int(current_expression_value.c));
			break;
		case Type::T_STRING:
			try {
				current_expression_value.set(cp_int(std::stoll(current_expression_value.s)));
			}
			catch (...) {
				set_curr_pos(astnode->row, astnode->col);
				throw std::runtime_error("'" + current_expression_value.s + "' is not a valid value to parse int");
			}
			break;
		}
		break;

	case Type::T_FLOAT:
		switch (current_expression_value.curr_type) {
		case Type::T_BOOL:
			current_expression_value.set(cp_float(current_expression_value.b));
			break;
		case Type::T_INT:
			current_expression_value.set(cp_float(current_expression_value.i));
			break;
		case Type::T_FLOAT:
			break;
		case Type::T_CHAR:
			current_expression_value.set(cp_float(current_expression_value.c));
			break;
		case Type::T_STRING:
			try {
				current_expression_value.set(cp_float(std::stold(current_expression_value.s)));
			}
			catch (...) {
				set_curr_pos(astnode->row, astnode->col);
				throw std::runtime_error("'" + current_expression_value.s + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case Type::T_CHAR:
		switch (current_expression_value.curr_type) {
		case Type::T_BOOL:
			current_expression_value.set(cp_char(current_expression_value.b));
			break;
		case Type::T_INT:
			current_expression_value.set(cp_char(current_expression_value.i));
			break;
		case Type::T_FLOAT:
			current_expression_value.set(cp_char(current_expression_value.f));
			break;
		case Type::T_CHAR:
			break;
		case Type::T_STRING:
			if (current_expression_value.s.size() > 1) {
				set_curr_pos(astnode->row, astnode->col);
				throw std::runtime_error("'" + current_expression_value.s + "' is not a valid value to parse char");
			}
			else {
				current_expression_value.set(cp_char(current_expression_value.s[0]));
			}
			break;
		}
		break;

	case Type::T_STRING:
		current_expression_value.set(cp_string(parse_value_to_string(current_expression_value)));

	}

	current_expression_value.set_type(astnode->type);
	current_expression_value.set_curr_type(astnode->type);
}

void Interpreter::visit(ASTNullNode* astnode) {
	auto value = Value(Type::T_VOID);
	value.set_null();
	current_expression_value = value;
}

void Interpreter::visit(ASTThisNode* astnode) {
	auto value = Value(Type::T_STRING);
	value.set(cp_string(current_name.top()));
	current_expression_value = value;
}

void Interpreter::visit(ASTTypingNode* astnode) {
	current_expression_nmspace = "";

	astnode->expr->accept(this);

	auto curr_value = current_expression_value;
	auto dim = std::vector<unsigned int>();
	auto type = is_void(curr_value.curr_type) ? curr_value.type : curr_value.curr_type;
	std::string str_type = "";

	if (is_array(type)) {
		dim = calculate_array_dim_size(curr_value.arr);
		type = curr_value.arr_type;
	}

	str_type = type_str(type);

	if (type == Type::T_STRUCT) {
		if (dim.size() > 0) {
			auto arr = curr_value.arr[0];
			for (size_t i = 0; i < dim.size() - 1; ++i) {
				arr = arr->arr[0];
			}
			str_type = arr->str->first;
		}
		else {
			str_type = curr_value.str->first;
		}
	}

	if (dim.size() > 0) {
		for (size_t i = 0; i < dim.size(); ++i) {
			str_type += "[" + std::to_string(dim[i]) + "]";
		}
	}

	str_type = current_expression_nmspace + "::" + str_type;

	if (astnode->image == "typeid") {
		auto value = Value(Type::T_INT);
		value.set(cp_int(axe::Util::hashcode(str_type)));
		current_expression_value = value;
	}
	else {
		auto value = Value(Type::T_STRING);
		value.set(cp_string(str_type));
		current_expression_value = value;
	}
}

bool Interpreter::equals_value(Value* lval, Value* rval) {
	switch (lval->curr_type) {
	case Type::T_BOOL:
		if (lval->b == rval->b) {
			return true;
		}
		break;
	case Type::T_INT:
		if (lval->i == rval->i) {
			return true;
		}
		break;
	case Type::T_FLOAT:
		if (lval->f == rval->f) {
			return true;
		}
		break;
	case Type::T_CHAR:
		if (lval->c == rval->c) {
			return true;
		}
		break;
	case Type::T_STRING:
		if (lval->s == rval->s) {
			return true;
		}
		break;
	case Type::T_ARRAY:
		if (equals_array(lval->arr, rval->arr)) {
			return true;
		}
		break;
	case Type::T_STRUCT:
		if (is_vbv && equals_struct(lval->str, rval->str)
			|| !is_vbv && lval->str == rval->str) {
			return true;
		}
		break;
	}

	return false;
}

bool Interpreter::equals_struct(cp_struct* lstr, cp_struct* rstr) {
	if (lstr->first != rstr->first
		|| lstr->second.size() != rstr->second.size()) {
		return false;
	}

	for (auto& lval : lstr->second) {
		if (rstr->second.find(lval.first) == rstr->second.end()) {
			return false;
		}
		if (!equals_value(lval.second, rstr->second[lval.first])) {
			return false;
		}
	}

	return true;
}

bool Interpreter::equals_array(cp_array larr, cp_array rarr) {
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

InterpreterScope* Interpreter::get_inner_most_variable_scope(std::string nmspace, std::string identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			for (auto prgnmspace : program_nmspaces[get_current_namespace()]) {
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
			throw std::runtime_error("something went wrong searching '" + identifier + "'");
		}
	}
	return scopes[nmspace][i];
}

InterpreterScope* Interpreter::get_inner_most_struct_definition_scope(std::string nmspace, std::string identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			bool found = false;
			for (auto prgnmspace : program_nmspaces[get_current_namespace()]) {
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
			throw std::runtime_error("something went wrong searching '" + identifier + "'");
		}
	}
	return scopes[nmspace][i];
}

InterpreterScope* Interpreter::get_inner_most_function_scope(std::string nmspace, std::string identifier, std::vector<TypeDefinition> signature) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature); --i) {
		if (i <= 0) {
			for (auto prgnmspace : program_nmspaces[get_current_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function(identifier, signature); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "'");
		}
	}
	return scopes[nmspace][i];
}

Value* Interpreter::access_value(InterpreterScope* scope, Value* value, std::vector<Identifier> identifier_vector, size_t i) {
	Value* next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		cp_array* current_Val = &next_value->arr;
		size_t s = 0;
		size_t access_pos = 0;

		for (s = 0; s < access_vector.size() - 1; ++s) {
			access_pos = access_vector.at(s);
			// break if it is a string, and the string access will be handled in identifier node evaluation
			if (current_Val->at(access_pos)->curr_type == Type::T_STRING) {
				has_string_access = true;
				break;
			}
			if (access_pos >= current_Val->size()) {
				throw std::runtime_error("invalid array position access");
			}
			current_Val = &current_Val->at(access_pos)->arr;
		}
		if (is_string(next_value->curr_type)) {
			has_string_access = true;
			return next_value;
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

std::vector<Value*> Interpreter::build_array(std::vector<ASTExprNode*> dim, Value* init_value, long long i) {
	auto arr = std::vector<Value*>();

	auto crr_acc = dim[i];
	crr_acc->accept(this);

	size_t size = current_expression_value.i;

	for (size_t j = 0; j < size; ++j) {
		auto val = new Value(init_value);
		arr.push_back(val);
	}

	--i;

	if (i >= 0) {
		auto val = new Value(Type::T_ARRAY);
		val->arr_type = init_value->type;
		val->set(arr);
		return build_array(dim, val, i);
	}

	return arr;
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

void Interpreter::declare_function_block_parameters(std::string nmspace) {
	auto curr_scope = scopes[nmspace].back();
	auto rest_name = std::string();
	auto arr = cp_array();
	Value* last_value = nullptr;
	size_t i;

	for (i = 0; i < last_function_arguments.size(); ++i) {
		if (i >= function_call_parameters.size()) {
			arr.push_back(new Value(last_function_arguments[i]));
		}
		else {
			auto pname = std::get<0>(function_call_parameters[i]);

			if (is_function(last_function_arguments[i]->curr_type)) {
				last_value = last_function_arguments[i];
				auto funcs = ((InterpreterScope*)last_function_arguments[i]->fun.first)->find_declared_functions(last_function_arguments[i]->fun.second);
				for (auto& it = funcs.first; it != funcs.second; ++it) {
					auto& func_params = it->second.first;
					auto& func_block = it->second.second;
					scopes[nmspace].back()->declare_function(pname, func_params, func_block);
				}
			}
			else {
				last_value = scopes[nmspace].back()->declare_value(pname, new Value(last_function_arguments[i]));
			}

			if (std::get<3>(function_call_parameters[i])) {
				rest_name = pname;
				arr.push_back(last_value);
			}
		}
	}
	for (; i < function_call_parameters.size(); ++i) {
		auto pname = std::get<0>(function_call_parameters[i]);
		std::get<2>(function_call_parameters[i])->accept(this);

		if (is_function(current_expression_value.curr_type)) {
			auto funcs = ((InterpreterScope*)current_expression_value.fun.first)->find_declared_functions(current_expression_value.fun.second);
			for (auto& it = funcs.first; it != funcs.second; ++it) {
				auto& func_params = it->second.first;
				auto& func_block = it->second.second;
				scopes[nmspace].back()->declare_function(pname, func_params, func_block);
			}
		}
		else {
			scopes[nmspace].back()->declare_value(pname, new Value(current_expression_value));
		}
	}

	if (arr.size() > 0) {
		auto rest = new Value(Type::T_ARRAY);
		rest->set(arr);
		scopes[nmspace].back()->declare_value(rest_name, rest);
	}

	function_call_parameters.clear();
	last_function_arguments.clear();
}

void Interpreter::declare_new_structure(std::string identifier_vector, Value new_value) {
	Value* value = nullptr;
	std::string type_name = new_value.str->first;
	StructureDefinition struct_definition;
	size_t str_def_scope_idx = 0;

	if (new_value.has_value()) {
		value = scopes[get_namespace()].back()->declare_variable(identifier_vector, new_value.str);
	}
	else {
		value = scopes[get_namespace()].back()->declare_null_struct_variable(identifier_vector, type_name);
	}
}

void Interpreter::declare_structure(cp_struct* str, std::string nmspace) {
	std::string actnmspace = get_namespace(nmspace);
	InterpreterScope* curr_scope;
	try {
		curr_scope = get_inner_most_struct_definition_scope(get_namespace(actnmspace), str->first);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}
	auto struct_def = curr_scope->find_declared_structure_definition(str->first);

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

std::vector<unsigned int> Interpreter::evaluate_access_vector(std::vector<ASTExprNode*> exprAcessVector) {
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

std::string Interpreter::parse_value_to_string(Value value) {
	switch (value.curr_type) {
	case Type::T_VOID:
		return "null";
	case Type::T_BOOL:
		return ((value.b) ? "true" : "false");
	case Type::T_INT:
		return std::to_string(value.i);
	case Type::T_FLOAT:
		return std::to_string(value.f);
	case Type::T_CHAR:
		return cp_string(std::string{ value.c });
	case Type::T_STRING:
		return value.s;
	case Type::T_STRUCT:
		return parse_struct_to_string(*value.str);
	case Type::T_ARRAY:
		return parse_array_to_string(value.arr);
	case Type::T_FUNCTION: {
		auto funcs = ((InterpreterScope*)value.fun.first)->find_declared_functions(value.fun.second);
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

			return func_decl;
		}
		break;
	}
	default:
		throw std::runtime_error("can't determine value type on parsing");
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
		if (key != modules::Module::INSTANCE_ID_NAME) {
			s << key + ":";
			s << parse_value_to_string(*val);
			s << ",";
		}
	}
	if (s.str() != "{") {
		s.seekp(-1, std::ios_base::end);
	}
	s << "}";
	return s.str();
}

void Interpreter::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Interpreter::msg_header() {
	return "(IERR) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}

unsigned int Interpreter::hash(ASTExprNode* astnode) {
	return 0;
}

unsigned int Interpreter::hash(ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(ASTLiteralNode<cp_int>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(ASTLiteralNode<cp_float>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(ASTLiteralNode<cp_char>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int Interpreter::hash(ASTLiteralNode<cp_string>* astnode) {
	return axe::Util::hashcode(astnode->val);
}

unsigned int Interpreter::hash(ASTIdentifierNode* astnode) {
	std::string nmspace = get_namespace(astnode->nmspace);

	InterpreterScope* id_scope;
	try {
		id_scope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (std::exception ex) {
		throw std::runtime_error(ex.what());
	}

	Value* value = access_value(id_scope, id_scope->find_declared_variable(astnode->identifier_vector[0].identifier), astnode->identifier_vector);

	switch (value->curr_type) {
	case Type::T_BOOL:
		return static_cast<unsigned int>(value->b);
	case Type::T_INT:
		return static_cast<unsigned int>(value->i);
	case Type::T_FLOAT:
		return static_cast<unsigned int>(value->f);
	case Type::T_CHAR:
		return static_cast<unsigned int>(value->c);
	case Type::T_STRING:
		return axe::Util::hashcode(value->s);
	}
}

void Interpreter::call_builtin_function(std::string identifier) {
	auto arr = cp_array();

	for (size_t i = 0; i < last_function_arguments.size(); ++i) {
		if (i >= function_call_parameters.size()) {
			arr.push_back(new Value(last_function_arguments[i]));
		}
		else {
			auto last_value = last_function_arguments[i];

			if (std::get<3>(function_call_parameters[i])) {
				arr.push_back(last_value);
			}
			else {
				builtin_arguments.push_back(last_value);
			}
		}
	}

	if (arr.size() > 0) {
		auto rest = new Value(Type::T_ARRAY);
		rest->set(arr);
		builtin_arguments.push_back(rest);
	}

	last_function_arguments.clear();

	builtin_functions[identifier]();
	builtin_arguments.clear();
}

void Interpreter::register_built_in_functions() {
	interpreter_parameter_list_t params;

	builtin_functions["print"] = [this]() {
		for (auto arg : builtin_arguments[0]->arr) {
			std::cout << parse_value_to_string(*arg);
		}
	};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), nullptr, true));
	scopes[default_namespace].back()->declare_function("print", params, nullptr);


	builtin_functions["read"] = [this]() {
		if (builtin_arguments.size() > 0) {
			builtin_functions["print"]();
		}
		std::string line;
		std::getline(std::cin, line);
		current_expression_value.set(cp_string(std::move(line)));
	};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), nullptr, true));
	scopes[default_namespace].back()->declare_function("read", params, nullptr);


	builtin_functions["readch"] = [this]() {
		while (!_kbhit());
		char ch = _getch();
		current_expression_value.set(cp_char(ch));
	};
	params.clear();
	scopes[default_namespace].back()->declare_function("readch", params, nullptr);


	builtin_functions["len"] = [this]() {
		auto& curr_val = builtin_arguments[0];
		auto val = Value(Type::T_INT);

		if (is_array(curr_val->curr_type)) {
			val.set(cp_int(curr_val->arr.size()));
		}
		else {
			val.set(cp_int(curr_val->s.size()));
		}

		current_expression_value = val;
	};
	params.clear();
	params.push_back(std::make_tuple("arr", TypeDefinition::get_array(Type::T_ARRAY, Type::T_ANY), nullptr, false));
	scopes[default_namespace].back()->declare_function("len", params, nullptr);
	params.clear();
	params.push_back(std::make_tuple("str", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	scopes[default_namespace].back()->declare_function("len", params, nullptr);


	builtin_functions["equals"] = [this]() {
		auto& rval = builtin_arguments[0];
		auto& lval = builtin_arguments[1];
		auto res = Value(Type::T_BOOL);

		res.b = equals_struct(rval->str, lval->str);

		current_expression_value = res;
	};
	params.clear();
	params.push_back(std::make_tuple("lval", TypeDefinition::get_basic(Type::T_ANY), nullptr, false));
	params.push_back(std::make_tuple("rval", TypeDefinition::get_basic(Type::T_ANY), nullptr, false));
	scopes[default_namespace].back()->declare_function("equals", params, nullptr);


	builtin_functions["system"] = [this]() {
		system(builtin_arguments[0]->s.c_str());
	};
	params.clear();
	params.push_back(std::make_tuple("cmd", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	scopes[default_namespace].back()->declare_function("system", params, nullptr);
}

void Interpreter::register_built_in_lib(std::string libname) {
	if (built_in_libs[0] == libname) {
		cpgraphics = new modules::Graphics();
		cpgraphics->register_functions(this);
	}

	if (built_in_libs[1] == libname) {
		cpfiles = new modules::Files();
		cpfiles->register_functions(this);
	}

	if (built_in_libs[3] == libname) {
		cpconsole = new modules::Console();
		cpconsole->register_functions(this);
	}
}
