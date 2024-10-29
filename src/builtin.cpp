#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <conio.h>

#include "builtin.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

#include "visitor.hpp"

using namespace modules;

Builtin::Builtin() {}

Builtin::~Builtin() = default;

void Builtin::register_functions(visitor::SemanticAnalyser* visitor) {
	std::vector<TypeDefinition> signature;
	std::vector<ParameterDefinition> parameters;
	VariableDefinition variable;

	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("print", FunctionDefinition("print", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["print"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("println", FunctionDefinition("println", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["println"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("read", FunctionDefinition("read", Type::T_STRING, signature, parameters));
	visitor->builtin_functions["read"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	visitor->scopes[default_namespace].back()->declare_function("readch", FunctionDefinition("readch", Type::T_CHAR, signature, parameters));
	visitor->builtin_functions["readch"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_array("arr", Type::T_ANY);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, signature, parameters));

	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("str", Type::T_STRING);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, signature, parameters));

	visitor->builtin_functions["len"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("sleep", Type::T_INT);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("sleep", FunctionDefinition("sleep", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["sleep"] = nullptr;


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("cmd", Type::T_STRING);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("system", FunctionDefinition("system", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["system"] = nullptr;
}

void Builtin::register_functions(visitor::Interpreter* visitor) {
	std::vector<TypeDefinition> signature;
	std::vector<ParameterDefinition> parameters;
	VariableDefinition variable;

	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("print", FunctionDefinition("print", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["print"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		if (visitor->builtin_arguments.size() == 0) {
			return;
		}
		for (size_t i = 0; i < visitor->builtin_arguments[0]->get_arr().size(); ++i) {
			std::cout << visitor->parse_value_to_string(visitor->builtin_arguments[0]->get_arr()[i]);
		}
		};

	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("println", FunctionDefinition("println", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["println"] = nullptr;
	visitor->builtin_functions["println"] = [this, visitor]() {
		visitor->builtin_functions["print"]();
		std::cout << std::endl;
		};


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("read", FunctionDefinition("read", Type::T_STRING, signature, parameters));
	visitor->builtin_functions["read"] = [this, visitor]() {
		if (visitor->builtin_arguments.size() > 0) {
			visitor->builtin_functions["print"]();
		}
		std::string line;
		std::getline(std::cin, line);
		visitor->current_expression_value = new RuntimeValue(Type::T_STRING);
		visitor->current_expression_value->set(cp_string(std::move(line)));
		};


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	visitor->scopes[default_namespace].back()->declare_function("readch", FunctionDefinition("readch", Type::T_CHAR, signature, parameters));
	visitor->builtin_functions["readch"] = [this, visitor]() {
		while (!_kbhit());
		char ch = _getch();
		visitor->current_expression_value = new RuntimeValue(Type::T_CHAR);
		visitor->current_expression_value->set(cp_char(ch));
		};


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_array("arr", Type::T_ANY);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, signature, parameters));

	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("str", Type::T_STRING);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("len", FunctionDefinition("len", Type::T_INT, signature, parameters));

	visitor->builtin_functions["len"] = [this, visitor]() {
		auto& curr_val = visitor->builtin_arguments[0];
		auto val = new RuntimeValue(Type::T_INT);

		if (is_array(curr_val->type)) {
			val->set(cp_int(curr_val->get_arr().size()));
		}
		else {
			val->set(cp_int(curr_val->get_s().size()));
		}

		visitor->current_expression_value = val;
		};


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("sleep", Type::T_INT);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("sleep", FunctionDefinition("sleep", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["sleep"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		std::this_thread::sleep_for(std::chrono::milliseconds(visitor->builtin_arguments[0]->get_i()));
		};


	signature = std::vector<TypeDefinition>();
	parameters = std::vector<ParameterDefinition>();
	variable = VariableDefinition::get_basic("cmd", Type::T_STRING);
	parameters.push_back(variable);
	signature.push_back(variable);
	visitor->scopes[default_namespace].back()->declare_function("system", FunctionDefinition("system", Type::T_VOID, signature, parameters));
	visitor->builtin_functions["system"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		system(visitor->builtin_arguments[0]->get_s().c_str());
		};

}
