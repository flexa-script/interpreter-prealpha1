#include <iostream>
#include <cmath>

#include "interpreter_scope.hpp"
#include "interpreter.hpp"
#include "vendor/axeutils.hpp"


using namespace visitor;
using namespace parser;


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

bool InterpreterScope::already_declared_function_name(std::string identifier) {
	try {
		find_declared_functions(identifier);
		return true;
	}
	catch (...) {
		return false;
	}
}

Value* InterpreterScope::declare_empty_variable(std::string identifier, parser::Type type, parser::Type empty_type) {
	Value* value = new Value(type);
	value->set_empty(empty_type);
	variable_symbol_table[identifier] = value;
	return value;
}

Value* InterpreterScope::declare_empty_struct_variable(std::string identifier, std::string type_name, parser::Type empty_type) {
	Value* value = declare_empty_variable(identifier, parser::Type::T_STRUCT, empty_type);
	std::get<1>(*value->str) = type_name;
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

Value* InterpreterScope::declare_variable(std::string identifier, cp_array arrValue, Type arr_type) {
	Value* value = new Value(parser::Type::T_ARRAY, arr_type);
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

Value* InterpreterScope::declare_variable(std::string identifier, cp_function fun) {
	Value* value = new Value(parser::Type::T_FUNCTION);
	value->set(fun);
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

void InterpreterScope::declare_function(std::string identifier, interpreter_parameter_list_t variables, parser::ASTBlockNode* block) {
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(variables, block)));
}

parser::StructureDefinition InterpreterScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

Value* InterpreterScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

interpreter_function_t InterpreterScope::find_declared_function(std::string identifier, std::vector<parser::TypeDefinition> signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		auto& func_params = it->second.first;
		bool rest = false;
		auto found = true;
		bool is_arr = false;
		Type stype = Type::T_UNDEFINED;
		Type ftype = Type::T_UNDEFINED;
		size_t func_sig_size = func_params.size();
		size_t call_sig_size = signature.size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				is_arr = is_array(std::get<1>(func_params.at(i)).type) && is_array(signature.at(i).type);
				ftype = is_arr ? std::get<1>(func_params.at(i)).array_type : std::get<1>(func_params.at(i)).type;
				stype = is_arr ? signature.at(i).array_type : signature.at(i).type;

				if (!match_type(ftype, stype) && !is_any(ftype)
					&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second;
			}
		}

		// if function signature is lesser than signature call, handle rest case
		found = true;
		if (func_sig_size < call_sig_size) {
			for (size_t i = 0; i < func_params.size(); ++i) {
				if (!rest) {
					is_arr = is_array(std::get<1>(func_params.at(i)).type) && is_array(signature.at(i).type);
					ftype = is_arr ? std::get<1>(func_params.at(i)).array_type : std::get<1>(func_params.at(i)).type;

					// store current rest function, and try to find an exactly signature match
					if (std::get<3>(it->second.first[i])) {
						rest = true;
					}
				}
				stype = is_arr ? signature.at(i).array_type : signature.at(i).type;

				if (!match_type(ftype, stype) && !is_any(ftype)
					&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second;
			}
		}

		// if function signature is greater than signature call, handle default value cases
		found = true;
		for (size_t i = 0; i < func_sig_size; ++i) {
			if (func_sig_size <= call_sig_size) {
				is_arr = is_array(std::get<1>(func_params.at(i)).type) && is_array(signature.at(i).type);
				ftype = is_arr ? std::get<1>(func_params.at(i)).array_type : std::get<1>(func_params.at(i)).type;
				stype = is_arr ? signature.at(i).array_type : signature.at(i).type;

				if (!match_type(ftype, stype) && !is_any(ftype)
					&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
					found = false;
					break;
				}
			}
			else {
				break;
			}
		}

		// if found and exactly signature size (not rest)
		if (found) {
			return it->second;
		}
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}

std::pair<interpreter_function_list_t::iterator, interpreter_function_list_t::iterator>
InterpreterScope::find_declared_functions(std::string identifier) {
	auto funcs = function_symbol_table.equal_range(identifier);
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}
	return funcs;
}
