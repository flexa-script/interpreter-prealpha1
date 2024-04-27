#include <utility>
#include <iostream>

#include "semantic_scope.hpp"
#include "util.hpp"


using namespace visitor;


SemanticScope::SemanticScope() {}

parser::StructureDefinition_t SemanticScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

parser::VariableDefinition_t SemanticScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

parser::FunctionDefinition_t SemanticScope::find_declared_function(std::string identifier, std::vector<parser::Type> signature) {
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
			if (func_sig.at(it) != signature.at(it) && func_sig.at(it) != parser::Type::T_ANY) {
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

bool SemanticScope::already_declared_function(std::string identifier, std::vector<parser::Type> signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

void SemanticScope::declare_structure_definition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t str_def(name, variables, row, col);
	structure_symbol_table[name] = str_def;
}

void SemanticScope::declare_variable(std::string identifier, parser::Type type, std::string type_name, parser::Type any_type, parser::Type array_type, std::vector<parser::ASTExprNode*> dim, parser::ASTExprNode* expr, bool is_const, unsigned int row, unsigned int col, bool is_parameter) {
	parser::VariableDefinition_t var(identifier, type, type_name, any_type, array_type, dim, expr, is_const, row, col, is_parameter);
	variable_symbol_table[identifier] = var;
}

void SemanticScope::declare_function(std::string identifier, parser::Type type, std::string type_name, parser::Type any_type, parser::Type array_type, std::vector<parser::ASTExprNode*> dim, std::vector<parser::Type> signature, unsigned int row, unsigned int col) {
	parser::FunctionDefinition_t fun(identifier, type, type_name, any_type, array_type, dim, signature, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::assign_variable(std::string identifier) {
	auto var = variable_symbol_table[identifier];
	variable_symbol_table[identifier] = var;
}

void SemanticScope::change_variable_type_name(std::string identifier, std::string type_name) {
	auto var = variable_symbol_table[identifier];
	var.type_name = type_name;
	variable_symbol_table[identifier] = var;
}

void SemanticScope::change_any_variable_type(std::string identifier, parser::Type type) {
	auto var = variable_symbol_table[identifier];
	var.any_type = type;
	variable_symbol_table[identifier] = var;
}
