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
	auto signature = std::vector<TypeDefinition>();
	auto parameters = std::vector<VariableDefinition>();

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_basic_function("print", Type::T_VOID, signature, parameters);
	visitor->builtin_functions["print"] = nullptr;

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_basic_function("println", Type::T_VOID, signature, parameters);
	visitor->builtin_functions["println"] = nullptr;


	signature.clear();
	parameters.clear();
	visitor->scopes[default_namespace].back()->declare_basic_function("read", Type::T_STRING, signature, parameters);
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_basic_function("read", Type::T_STRING, signature, parameters);
	visitor->builtin_functions["read"] = nullptr;


	signature.clear();
	parameters.clear();
	visitor->scopes[default_namespace].back()->declare_basic_function("readch", Type::T_CHAR, signature, parameters);
	visitor->builtin_functions["readch"] = nullptr;


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_array(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_array("arr", Type::T_ANY));
	visitor->scopes[default_namespace].back()->declare_basic_function("len", Type::T_INT, signature, parameters);
	visitor->builtin_functions["system"] = nullptr;

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_STRING));
	parameters.push_back(VariableDefinition::get_basic("str", Type::T_STRING));
	visitor->scopes[default_namespace].back()->declare_basic_function("len", Type::T_INT, signature, parameters);
	visitor->builtin_functions["len"] = nullptr;


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_INT));
	parameters.push_back(VariableDefinition::get_basic("sleep", Type::T_INT));
	visitor->scopes[default_namespace].back()->declare_basic_function("sleep", Type::T_VOID, signature, parameters);
	visitor->builtin_functions["sleep"] = nullptr;


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_STRING));
	parameters.push_back(VariableDefinition::get_basic("cmd", Type::T_STRING));
	visitor->scopes[default_namespace].back()->declare_basic_function("system", Type::T_VOID, signature, parameters);
	visitor->builtin_functions["system"] = nullptr;
}

void Builtin::register_functions(visitor::Interpreter* visitor) {

	interpreter_parameter_list_t params;

	visitor->builtin_functions["print"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		if (visitor->builtin_arguments.size() == 0) {
			return;
		}
		for (size_t i = 0; i < visitor->builtin_arguments[0]->get_arr().second; ++i) {
			std::cout << visitor->parse_value_to_string(visitor->builtin_arguments[0]->get_arr().first[i]);
		}
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_function("print", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));

	visitor->builtin_functions["println"] = [this, visitor]() {
		visitor->builtin_functions["print"]();
		std::cout << std::endl;
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_function("println", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));


	visitor->builtin_functions["read"] = [this, visitor]() {
		if (visitor->builtin_arguments.size() > 0) {
			visitor->builtin_functions["print"]();
		}
		std::string line;
		std::getline(std::cin, line);
		visitor->current_expression_value = new RuntimeValue(Type::T_STRING);
		visitor->current_expression_value->set(cp_string(std::move(line)));
		};
	params.clear();
	params.push_back(std::make_tuple("args", TypeDefinition::get_basic(Type::T_ANY), new ASTNullNode(0, 0), true));
	visitor->scopes[default_namespace].back()->declare_function("read", params, nullptr, TypeDefinition::get_basic(Type::T_STRING));


	visitor->builtin_functions["readch"] = [this, visitor]() {
		while (!_kbhit());
		char ch = _getch();
		visitor->current_expression_value = new RuntimeValue(Type::T_CHAR);
		visitor->current_expression_value->set(cp_char(ch));
		};
	params.clear();
	visitor->scopes[default_namespace].back()->declare_function("readch", params, nullptr, TypeDefinition::get_basic(Type::T_CHAR));


	visitor->builtin_functions["len"] = [this, visitor]() {
		auto& curr_val = visitor->builtin_arguments[0];
		auto val = new RuntimeValue(Type::T_INT);

		if (is_array(curr_val->type)) {
			val->set(cp_int(curr_val->get_arr().second));
		}
		else {
			val->set(cp_int(curr_val->get_s().size()));
		}

		visitor->current_expression_value = val;
		};
	params.clear();
	params.push_back(std::make_tuple("arr", TypeDefinition::get_array(Type::T_ANY), nullptr, false));
	visitor->scopes[default_namespace].back()->declare_function("len", params, nullptr, TypeDefinition::get_basic(Type::T_INT));
	params.clear();
	params.push_back(std::make_tuple("str", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	visitor->scopes[default_namespace].back()->declare_function("len", params, nullptr, TypeDefinition::get_basic(Type::T_INT));


	visitor->builtin_functions["sleep"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		std::this_thread::sleep_for(std::chrono::milliseconds(visitor->builtin_arguments[0]->get_i()));
		};
	params.clear();
	params.push_back(std::make_tuple("time", TypeDefinition::get_basic(Type::T_INT), nullptr, false));
	visitor->scopes[default_namespace].back()->declare_function("sleep", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));


	visitor->builtin_functions["system"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(Type::T_UNDEFINED);
		system(visitor->builtin_arguments[0]->get_s().c_str());
		};
	params.clear();
	params.push_back(std::make_tuple("cmd", TypeDefinition::get_basic(Type::T_STRING), nullptr, false));
	visitor->scopes[default_namespace].back()->declare_function("system", params, nullptr, TypeDefinition::get_basic(Type::T_VOID));

}
