#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <conio.h>

#include "builtin.hpp"

#include "semantic_analysis.hpp"
#include "interpreter.hpp"

#include "visitor.hpp"

using namespace modules;

Builtin::Builtin() {}

Builtin::~Builtin() = default;

void Builtin::register_functions(visitor::SemanticAnalyser* visitor) {
	std::vector<TypeDefinition*> parameters;
	VariableDefinition* variable;

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("print", FunctionDefinition("print", Type::T_VOID, parameters));
	visitor->builtin_functions["print"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("println", FunctionDefinition("println", Type::T_VOID, parameters));
	visitor->builtin_functions["println"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("read", FunctionDefinition("read", Type::T_STRING, parameters));
	visitor->builtin_functions["read"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	visitor->scopes[default_namespace].back()->declare_function("readch", FunctionDefinition("readch", Type::T_CHAR, parameters));
	visitor->builtin_functions["readch"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("arr", Type::T_ANY, std::vector<std::shared_ptr<ASTExprNode>>());
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("str", Type::T_STRING);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, parameters));

	visitor->builtin_functions["len"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("sleep", Type::T_INT);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("sleep", FunctionDefinition("sleep", Type::T_VOID, parameters));
	visitor->builtin_functions["sleep"] = nullptr;


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("cmd", Type::T_STRING);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("system", FunctionDefinition("system", Type::T_VOID, parameters));
	visitor->builtin_functions["system"] = nullptr;
}

void Builtin::register_functions(visitor::Interpreter* visitor) {
	std::vector<TypeDefinition*> parameters;
	VariableDefinition* variable;

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("print", FunctionDefinition("print", Type::T_VOID, parameters));
	visitor->builtin_functions["print"] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		if (visitor->builtin_arguments.size() == 0) {
			return;
		}
		for (size_t i = 0; i < visitor->builtin_arguments[0]->get_arr().size(); ++i) {
			std::cout << visitor->parse_value_to_string(visitor->builtin_arguments[0]->get_arr()[i]);
		}
		};

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("println", FunctionDefinition("println", Type::T_VOID, parameters));
	visitor->builtin_functions["println"] = nullptr;
	visitor->builtin_functions["println"] = [this, visitor]() {
		visitor->builtin_functions["print"]();
		std::cout << std::endl;
		};


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("read", FunctionDefinition("read", Type::T_STRING, parameters));
	visitor->builtin_functions["read"] = [this, visitor]() {
		if (visitor->builtin_arguments.size() > 0) {
			visitor->builtin_functions["print"]();
		}
		std::string line;
		std::getline(std::cin, line);
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_STRING));
		visitor->current_expression_value->set(cp_string(std::move(line)));
		};


	parameters = std::vector<TypeDefinition*>();
	visitor->scopes[default_namespace].back()->declare_function("readch", FunctionDefinition("readch", Type::T_CHAR, parameters));
	visitor->builtin_functions["readch"] = [this, visitor]() {
		while (!_kbhit());
		char ch = _getch();
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_CHAR));
		visitor->current_expression_value->set(cp_char(ch));
		};


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("arr", Type::T_ANY, std::vector<std::shared_ptr<ASTExprNode>>());
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("str", Type::T_STRING);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, parameters));

	visitor->builtin_functions["len"] = [this, visitor]() {
		auto& curr_val = visitor->builtin_arguments[0];
		auto val = visitor->alocate_value(new RuntimeValue(Type::T_INT));

		if (is_array(curr_val->type)) {
			val->set(cp_int(curr_val->get_arr().size()));
		}
		else {
			val->set(cp_int(curr_val->get_s().size()));
		}

		visitor->current_expression_value = val;
		};


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("sleep", Type::T_INT);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("sleep", FunctionDefinition("sleep", Type::T_VOID, parameters));
	visitor->builtin_functions["sleep"] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		std::this_thread::sleep_for(std::chrono::milliseconds(visitor->builtin_arguments[0]->get_i()));
		};


	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("cmd", Type::T_STRING);
	parameters.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("system", FunctionDefinition("system", Type::T_VOID, parameters));
	visitor->builtin_functions["system"] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		system(visitor->builtin_arguments[0]->get_s().c_str());
		};

}
