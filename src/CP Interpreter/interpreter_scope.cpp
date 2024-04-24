#include <iostream>
#include <cmath>

#include "interpreter_scope.hpp"
#include "interpreter.hpp"
#include "util.hpp"


using namespace visitor;


InterpreterScope::InterpreterScope(Interpreter* parent, std::string name) : parent(parent), name(name) {}

InterpreterScope::InterpreterScope() {
	name = "";
	parent = nullptr;
}

std::string InterpreterScope::get_name() {
	return name;
}

void InterpreterScope::set_name(std::string name) {
	this->name = name;
}

void InterpreterScope::set_parent(Interpreter* parent) {
	this->parent = parent;
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

Value_t* InterpreterScope::declare_null_variable(std::string identifier, parser::Type type) {
	Value_t* value = new Value_t(type);
	value->set_null();
	variable_symbol_table[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare_null_struct_variable(std::string identifier, std::string type_name) {
	Value_t* value = declare_null_variable(identifier, parser::Type::T_STRUCT);
	value->str.first = type_name;
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


Value_t* InterpreterScope::declare_variable(std::string identifier, cp_struct strValue) {
	Value_t* value = new Value_t(parser::Type::T_STRUCT);
	value->set(strValue);
	variable_symbol_table[identifier] = value;
	return value;
}

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

Value_t* InterpreterScope::access_value_of_array(Value_t* arr, std::vector<parser::ASTExprNode*> access_vector) {
	cp_array* currentVal = &arr->arr;
	size_t s = 0;
	size_t accessPos = 0;

	for (s = 0; s < access_vector.size() - 1; ++s) {
		access_vector.at(s)->accept(parent);
		accessPos = parent->get_current_expression_value().i;
		if (accessPos >= currentVal->size()) {
			throw std::runtime_error("ISERR: tryed to access a invalid position");
		}
		if (currentVal->at(accessPos)->curr_type != parser::Type::T_ARRAY) {
			has_string_access = true;
			break;
		}
		currentVal = &currentVal->at(accessPos)->arr;
	}

	access_vector.at(s)->accept(parent);
	accessPos = parent->get_current_expression_value().i;
	if (accessPos >= currentVal->size()) {
		throw std::runtime_error("ISERR: tryed to access a invalid position");
	}

	return currentVal->at(accessPos);
}

Value_t* InterpreterScope::access_value_of_structure(Value_t* value, std::vector<std::string> identifier_vector) {
	Value_t* strValue = value;

	for (size_t i = 1; i < identifier_vector.size(); ++i) {
		for (size_t j = 0; j < strValue->str.second.size(); ++j) {
			if (identifier_vector[i] == strValue->str.second[j].first) {
				strValue = strValue->str.second[j].second;
				break;
			}
		}
	}

	return strValue;
}

Value_t* InterpreterScope::access_value(std::vector<std::string> identifier_vector, std::vector<parser::ASTExprNode*> access_vector) {
	Value_t* value = variable_symbol_table[identifier_vector[0]];

	if (identifier_vector.size() > 1) {
		value = access_value_of_structure(value, identifier_vector);
	}

	if (access_vector.size() > 0 && value->curr_type == parser::Type::T_ARRAY) {
		has_string_access = false;
		value = access_value_of_array(value, access_vector);
	}
	else {
		has_string_access = true;
	}

	return value;
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
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::Type::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return i->second;
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}
