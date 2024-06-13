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

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'");
	}

	FunctionDefinition* func = nullptr;

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		auto& func_sig = it->second.signature;
		bool rest = false;
		bool match_sig_size = true;
		auto found = true;
		bool is_arr = false;
		bool was_arr = false;
		Type stype = Type::T_UNDEFINED;
		Type ftype = Type::T_UNDEFINED;

		//if (!func && it->second.is_variable) {
		//	func = &it->second;
		//}

		if (func_sig.size() != signature.size()) {
			match_sig_size = false;
		}

		for (size_t i = 0; i < signature.size(); ++i) {
			// check default value parameters

			if (rest) {
				is_arr = was_arr;
			}
			else {
				is_arr = is_array(func_sig.at(i).type) && is_array(signature.at(i).type);
				was_arr = is_arr;
				ftype = is_arr ? func_sig.at(i).array_type : func_sig.at(i).type;

				if (!func && it->second.parameters[i].is_rest) {
					rest = true;
					func = &it->second;
				}
			}
			stype = is_arr ? signature.at(i).array_type : signature.at(i).type;
			if (!match_type(ftype, stype) && !is_any(ftype)
				&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
				found = false;
				break;
			}
		}

		if (found && match_sig_size) {
			return it->second;
		}
	}

	if (func) {
		return *func;
	}

	throw std::runtime_error("definition of '" + identifier + "' function not found");
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
	Type array_type, std::vector<ASTExprNode*> dim, std::vector<TypeDefinition> signature, std::vector<VariableDefinition> parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, array_type, dim, signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_variable_function(std::string identifier, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, Type::T_UNDEFINED, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(),
		std::vector<TypeDefinition>(), std::vector<VariableDefinition>(), row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_basic_function(std::string identifier, Type type, std::vector<TypeDefinition> signature,
	std::vector<VariableDefinition> parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_array_function(std::string identifier, Type type, Type array_type, std::vector<ASTExprNode*> dim,
	std::vector<TypeDefinition> signature, std::vector<VariableDefinition> parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", array_type, dim, signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_struct_function(std::string identifier, Type type, std::string type_name, std::string type_name_space,
	std::vector<TypeDefinition> signature, std::vector<VariableDefinition> parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::change_variable_type_name(std::string identifier, std::string type_name) {
	auto var = variable_symbol_table[identifier]->value->type_name = type_name;
}

void SemanticScope::change_current_variable_type(std::string identifier, Type type) {
	auto var = variable_symbol_table[identifier]->value->type = type;
}
