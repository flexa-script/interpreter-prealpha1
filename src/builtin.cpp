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

std::string modules::BUILTIN_NAMES[] = {
	"print",
	"println",
	"read",
	"readch",
	"len",
	"sleep",
	"system"
};

Builtin::Builtin() {
	build_decls();
}

Builtin::~Builtin() = default;

void Builtin::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::PRINT], func_decls[BUILTIN_NAMES[BuintinFuncs::PRINT]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINT]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::PRINTLN], func_decls[BUILTIN_NAMES[BuintinFuncs::PRINTLN]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINTLN]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::READ], func_decls[BUILTIN_NAMES[BuintinFuncs::READ]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::READ]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::READCH], func_decls[BUILTIN_NAMES[BuintinFuncs::READCH]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::READCH]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::LEN], func_decls[BUILTIN_NAMES[BuintinFuncs::LEN] + "A"]);
	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::LEN], func_decls[BUILTIN_NAMES[BuintinFuncs::LEN] + "S"]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::LEN]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::SLEEP], func_decls[BUILTIN_NAMES[BuintinFuncs::SLEEP]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::SLEEP]] = nullptr;

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::SYSTEM], func_decls[BUILTIN_NAMES[BuintinFuncs::SYSTEM]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::SYSTEM]] = nullptr;

}

void Builtin::register_functions(visitor::Interpreter* visitor) {
	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::PRINT], func_decls[BUILTIN_NAMES[BuintinFuncs::PRINT]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINT]] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		if (visitor->builtin_arguments.size() == 0) {
			return;
		}
		for (size_t i = 0; i < visitor->builtin_arguments[0]->get_arr().size(); ++i) {
			std::cout << visitor->parse_value_to_string(visitor->builtin_arguments[0]->get_arr()[i]);
		}
		};

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::PRINTLN], func_decls[BUILTIN_NAMES[BuintinFuncs::PRINTLN]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINTLN]] = [this, visitor]() {
		visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINT]]();
		std::cout << std::endl;
		};

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::READ], func_decls[BUILTIN_NAMES[BuintinFuncs::READ]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::READ]] = [this, visitor]() {
		if (visitor->builtin_arguments.size() > 0) {
			visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::PRINT]]();
		}
		std::string line;
		std::getline(std::cin, line);
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_STRING));
		visitor->current_expression_value->set(cp_string(std::move(line)));
		};

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::READCH], func_decls[BUILTIN_NAMES[BuintinFuncs::READCH]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::READCH]] = [this, visitor]() {
		while (!_kbhit());
		char ch = _getch();
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_CHAR));
		visitor->current_expression_value->set(cp_char(ch));
		};

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::LEN], func_decls[BUILTIN_NAMES[BuintinFuncs::LEN] + "A"]);
	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::LEN], func_decls[BUILTIN_NAMES[BuintinFuncs::LEN] + "S"]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::LEN]] = [this, visitor]() {
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

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::SLEEP], func_decls[BUILTIN_NAMES[BuintinFuncs::SLEEP]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::SLEEP]] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		std::this_thread::sleep_for(std::chrono::milliseconds(visitor->builtin_arguments[0]->get_i()));
		};

	visitor->scopes[default_namespace].back()->declare_function(BUILTIN_NAMES[BuintinFuncs::SYSTEM], func_decls[BUILTIN_NAMES[BuintinFuncs::SYSTEM]]);
	visitor->builtin_functions[BUILTIN_NAMES[BuintinFuncs::SYSTEM]] = [this, visitor]() {
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(Type::T_UNDEFINED));
		system(visitor->builtin_arguments[0]->get_s().c_str());
		};

}

void Builtin::build_decls() {
	std::vector<TypeDefinition*> parameters;
	VariableDefinition* variable;

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::PRINT], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::PRINT], Type::T_VOID, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::PRINTLN], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::PRINTLN], Type::T_VOID, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("args", Type::T_ANY, std::make_shared<ASTNullNode>(0, 0), true);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::READ], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::READ], Type::T_STRING, parameters));

	parameters = std::vector<TypeDefinition*>();
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::READCH], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::READCH], Type::T_CHAR, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("arr", Type::T_ANY, std::vector<std::shared_ptr<ASTExprNode>>());
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::LEN] + "A", FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::LEN] + "A", Type::T_INT, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("str", Type::T_STRING);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::LEN] + "S", FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::LEN] + "S", Type::T_INT, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("ms", Type::T_INT);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::SLEEP], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::SLEEP], Type::T_VOID, parameters));

	parameters = std::vector<TypeDefinition*>();
	variable = new VariableDefinition("cmd", Type::T_STRING);
	parameters.push_back(variable);
	func_decls.emplace(BUILTIN_NAMES[BuintinFuncs::SYSTEM], FunctionDefinition(BUILTIN_NAMES[BuintinFuncs::SYSTEM], Type::T_VOID, parameters));
}
