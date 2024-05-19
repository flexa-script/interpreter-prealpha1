#include <iostream>
#include <cmath>

#include "interpreter_scope.hpp"
#include "interpreter.hpp"
#include "util.hpp"


using namespace visitor;


InterpreterScope::InterpreterScope(std::string name) : name(name) {}

InterpreterScope::InterpreterScope() : name("") { }

std::string InterpreterScope::get_name() {
	return name;
}

void InterpreterScope::set_name(std::string name) {
	this->name = name;
}

bool InterpreterScope::already_declared_variable(std::string identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool InterpreterScope::already_declared_structure_definition(std::string identifier) {
	return structure_symbol_table.find(identifier) != structure_symbol_table.end();
}

bool InterpreterScope::already_declared_function(std::string identifier, std::vector<parser::Type> signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

Value_t* InterpreterScope::declare_undef_variable(std::string identifier, parser::Type type) {
	Value_t* value = new Value_t(type);
	value->set_undefined();
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_undef_struct_variable(std::string identifier, std::string type_name) {
	Value_t* value = declare_undef_variable(identifier, parser::Type::T_STRUCT);
	value->str->first = type_name;
	return value;
}

Value_t* InterpreterScope::declare_null_variable(std::string identifier, parser::Type type) {
	Value_t* value = new Value_t(type);
	value->set_null();
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_null_struct_variable(std::string identifier, std::string type_name) {
	Value_t* value = declare_null_variable(identifier, parser::Type::T_STRUCT);
	value->str->first = type_name;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_bool boolValue) {
	Value_t* value = new Value_t(parser::Type::T_BOOL);
	value->set(boolValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_int intValue) {
	Value_t* value = new Value_t(parser::Type::T_INT);
	value->set(intValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_float floatValue) {
	Value_t* value = new Value_t(parser::Type::T_FLOAT);
	value->set(floatValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_char charValue) {
	Value_t* value = new Value_t(parser::Type::T_CHAR);
	value->set(charValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_string stringValue) {
	Value_t* value = new Value_t(parser::Type::T_STRING);
	value->set(stringValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_array arrValue) {
	Value_t* value = new Value_t(parser::Type::T_ARRAY);
	value->set(arrValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_variable(std::string identifier, cp_struct* strValue) {
	Value_t* value = new Value_t(parser::Type::T_STRUCT);
	value->set(strValue);
	variable_symbol_table[identifier] = value;
	return value;
}

//void InterpreterScope::declare_value(std::string identifier, Value_t* value) {
//	variable_symbol_table[identifier] = value;
//}

void InterpreterScope::declare_structure_definition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structure_symbol_table[name] = (type);
}

void InterpreterScope::declare_function(std::string identifier, std::vector<parser::Type> signature, std::vector<std::string> variable_names, parser::ASTBlockNode* block) {
	function_symbol_table.insert(std::make_pair(identifier, std::make_tuple(signature, variable_names, block)));
}

parser::StructureDefinition_t InterpreterScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

Value_t* InterpreterScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

std::tuple<std::vector<parser::Type>, std::vector<std::string>, parser::ASTBlockNode*> InterpreterScope::find_declared_function(std::string identifier, std::vector<parser::Type> signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	// check signature for each function in multimap
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& funcSig = std::get<0>(i->second);
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::Type::T_ANY
				&& signature.at(it) != parser::Type::T_VOID && signature.at(it) != parser::Type::T_UNDEF) {
				found = false;
				break;
			}
		}
		if (found) return i->second;
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}
