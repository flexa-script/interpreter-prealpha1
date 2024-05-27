#include <utility>
#include <iostream>

#include "semantic_scope.hpp"
#include "vendor/util.hpp"


using namespace visitor;
using namespace parser;


SemanticScope::SemanticScope() = default;

SemanticScope::~SemanticScope() = default;

StructureDefinition SemanticScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

SemanticVariable* SemanticScope::find_declared_variable(std::string identifier) {
	return variable_symbol_table[identifier];
}

FunctionDefinition SemanticScope::find_declared_function(std::string identifier, std::vector<TypeDefinition> signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'");
	}

	// check signature for each function in functionSymbolTable
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& func_sig = i->second.signature;
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

bool SemanticScope::already_declared_function(std::string identifier, std::vector<TypeDefinition> signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

void SemanticScope::declare_structure_definition(std::string name, std::vector<VariableDefinition> variables, unsigned int row, unsigned int col) {
	StructureDefinition str_def(name, variables, row, col);
	structure_symbol_table[name] = str_def;
}

void SemanticScope::declare_variable(std::string identifier, Type type, Type array_type, std::vector<ASTExprNode*> dim,
	std::string type_name, std::string type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col) {
	SemanticVariable* var = new SemanticVariable(identifier, type, array_type, dim,
		type_name, type_name_space, value, is_const, row, col);
	variable_symbol_table[identifier] = var;
}

void SemanticScope::declare_function(std::string identifier, Type type, std::string type_name, std::string type_name_space,
	Type array_type, std::vector<ASTExprNode*> dim, std::vector<TypeDefinition> signature, std::vector<VariableDefinition> parameters,
	ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, array_type, dim, signature, parameters, block, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::change_variable_type_name(std::string identifier, std::string type_name) {
	auto var = variable_symbol_table[identifier]->value->type_name = type_name;
}

void SemanticScope::change_current_variable_type(std::string identifier, Type type) {
	auto var = variable_symbol_table[identifier]->value->type = type;
}
