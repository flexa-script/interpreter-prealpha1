#include <iostream>
#include <cmath>

#include "interpreter_scope.hpp"
#include "interpreter.hpp"
#include "vendor/util.hpp"


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

bool InterpreterScope::already_declared_function(std::string identifier, std::vector<parser::TypeDefinition> signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

Value* InterpreterScope::declare_undef_variable(std::string identifier, parser::Type type) {
	Value* value = new Value(type);
	value->set_undefined();
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_undef_struct_variable(std::string identifier, std::string type_name) {
	Value* value = declare_undef_variable(identifier, parser::Type::T_STRUCT);
	value->str->first = type_name;
	return value;
}

Value* InterpreterScope::declare_null_variable(std::string identifier, parser::Type type) {
	Value* value = new Value(type);
	value->set_null();
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_null_struct_variable(std::string identifier, std::string type_name) {
	Value* value = declare_null_variable(identifier, parser::Type::T_STRUCT);
	value->str->first = type_name;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_bool boolValue) {
	Value* value = new Value(parser::Type::T_BOOL);
	value->set(boolValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_int intValue) {
	Value* value = new Value(parser::Type::T_INT);
	value->set(intValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_float floatValue) {
	Value* value = new Value(parser::Type::T_FLOAT);
	value->set(floatValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_char charValue) {
	Value* value = new Value(parser::Type::T_CHAR);
	value->set(charValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_string stringValue) {
	Value* value = new Value(parser::Type::T_STRING);
	value->set(stringValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_array arrValue) {
	Value* value = new Value(parser::Type::T_ARRAY);
	value->set(arrValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_variable(std::string identifier, cp_struct* strValue) {
	Value* value = new Value(parser::Type::T_STRUCT);
	value->set(strValue);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_value(std::string identifier, Value* value) {
	variable_symbol_table[identifier] = value;
	return value;
}

void InterpreterScope::declare_structure_definition(std::string name, std::vector<parser::VariableDefinition> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition type(name, variables, row, col);
	structure_symbol_table[name] = (type);
}

void InterpreterScope::declare_function(std::string identifier, std::vector<parser::TypeDefinition> signature, std::vector<std::string> variable_names, parser::ASTBlockNode* block) {
	function_symbol_table.insert(std::make_pair(identifier, std::make_tuple(signature, variable_names, block)));
}

parser::StructureDefinition InterpreterScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

Value* InterpreterScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

interpreter_function_t InterpreterScope::find_declared_function(std::string identifier, std::vector<parser::TypeDefinition> signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	// check signature for each function in multimap
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& func_sig = std::get<0>(i->second);
		auto found = true;
		for (size_t it = 0; it < func_sig.size(); ++it) {
			auto is_arr = is_array(func_sig.at(it).type) && is_array(signature.at(it).type);
			auto ftype = is_arr ? func_sig.at(it).array_type : func_sig.at(it).type;
			auto stype = is_arr ? signature.at(it).array_type : signature.at(it).type;
			if (!match_type(ftype, stype) && !is_any(ftype)
				&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
				found = false;
				break;
			}
		}
		if (found && func_sig.size() == signature.size()) return i->second;
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}
