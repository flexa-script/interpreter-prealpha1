#include <utility>
#include <iostream>

#include "semantic_scope.hpp"
#include "util.hpp"


using namespace visitor;
using namespace parser;


SemanticScope::SemanticScope(std::string name) : name(name) {}

SemanticScope::SemanticScope() = default;

SemanticScope::~SemanticScope() = default;

std::string SemanticScope::get_name() {
	return name;
}

void SemanticScope::set_name(std::string name) {
	this->name = name;
}

StructureDefinition_t SemanticScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

SemanticVariable_t* SemanticScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

FunctionDefinition_t SemanticScope::find_declared_function(std::string identifier, std::vector<Type> signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'.");
	}

	// check signature for each function in functionSymbolTable
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& func_sig = i->second.signature;
		auto found = true;
		for (size_t it = 0; it < func_sig.size(); ++it) {
			if (func_sig.at(it) != signature.at(it) && func_sig.at(it) != Type::T_ANY
				&& signature.at(it) != Type::T_VOID && signature.at(it) != Type::T_UNDEF) {
				found = false;
				break;
			}
		}
		if (found) return i->second;
	}

	throw std::runtime_error("SSERR: definition of '" + identifier + "' function not found");
}

bool SemanticScope::already_declared_structure_definition(std::string identifier) {
	return structure_symbol_table.find(identifier) != structure_symbol_table.end();
}

bool SemanticScope::already_declared_variable(std::string identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool SemanticScope::already_declared_function(std::string identifier, std::vector<Type> signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

void SemanticScope::declare_structure_definition(std::string name, std::vector<VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	StructureDefinition_t str_def(name, variables, row, col);
	structure_symbol_table[name] = str_def;
}

void SemanticScope::declare_variable(std::string identifier, Type type, Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, SemanticValue* value, bool is_const, unsigned int row, unsigned int col, bool is_parameter) {
	SemanticVariable_t* var = new SemanticVariable_t(identifier, type, array_type, dim,
		type_name, value, is_const, row, col, is_parameter);
	variable_symbol_table[identifier] = var;
}

void SemanticScope::declare_function(std::string identifier, Type type, std::string type_name, Type any_type,
	Type array_type, std::vector<ASTExprNode*> dim, std::vector<Type> signature, std::vector<VariableDefinition_t> parameters,
	ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition_t fun(identifier, type, type_name, any_type, array_type, dim, signature, parameters, block, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::change_variable_type_name(std::string identifier, std::string type_name) {
	auto var = variable_symbol_table[identifier]->value->type_name = type_name;
}

void SemanticScope::change_current_variable_type(std::string identifier, Type type) {
	auto var = variable_symbol_table[identifier]->value->type = type;
}

//SemanticValue_t* SemanticScope::access_value_of_array(SemanticValue_t* arr, std::vector<ASTExprNode*> access_vector) {
//	//cp_array* currentVal = &arr->;
//	//size_t s = 0;
//	//size_t accessPos = 0;
//
//	//for (s = 0; s < access_vector.size() - 1; ++s) {
//	//	access_vector.at(s)->accept(intepr);
//	//	accessPos = intepr->get_current_expression_value().i;
//	//	if (accessPos >= currentVal->size()) {
//	//		throw std::runtime_error("ISERR: tryed to access a invalid position");
//	//	}
//	//	if (currentVal->at(accessPos)->curr_type != Type::T_ARRAY) {
//	//		has_string_access = true;
//	//		break;
//	//	}
//	//	currentVal = &currentVal->at(accessPos)->arr;
//	//}
//
//	//access_vector.at(s)->accept(intepr);
//	//accessPos = intepr->get_current_expression_value().i;
//	//if (accessPos >= currentVal->size()) {
//	//	throw std::runtime_error("ISERR: tryed to access a invalid position");
//	//}
//
//	//return currentVal->at(accessPos);
//	return nullptr;
//}

//SemanticValue_t* SemanticScope::access_value_of_structure(SemanticValue_t* value, std::vector<std::string> identifier_vector) {
//	SemanticValue_t* strValue = value;
//
//	for (size_t i = 1; i < identifier_vector.size(); ++i) {
//		strValue = strValue->struct_vars[identifier_vector[i]];
//	}
//
//	return strValue;
//}

//SemanticValue_t* SemanticScope::access_value(std::vector<std::string> identifier_vector, std::vector<ASTExprNode*> access_vector, bool check_undef) {
//	SemanticValue_t* value = find_declared_variable(identifier_vector[0])->value;
//
//	if (check_undef && is_undefined(value->type)) {
//		throw std::runtime_error("SSERR: variable '" + identifier_vector[0] + "' is not initialized");
//	}
//
//	if (identifier_vector.size() > 1) {
//		value = access_value_of_structure(value, identifier_vector);
//	}
//
//	//if (access_vector.size() > 0 && value->type == Type::T_ARRAY) {
//	//	value = access_value_of_array(value, access_vector);
//	//}
//
//	return value;
//}
