#include <utility>
#include <iostream>

#include "semantic_scope.hpp"
#include "vendor/axeutils.hpp"


using namespace visitor;
using namespace parser;


SemanticScope::SemanticScope() = default;

SemanticScope::~SemanticScope() = default;

StructureDefinition SemanticScope::find_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table[identifier];
}

SemanticVariable* SemanticScope::find_declared_variable(const std::string& identifier) {
	return variable_symbol_table[identifier];
}

FunctionDefinition SemanticScope::find_declared_function(const std::string& identifier, const std::vector<TypeDefinition>& signature) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		auto& func_sig = it->second.signature;
		bool rest = false;
		auto found = true;
		bool is_arr = false;
		Type stype = Type::T_UNDEFINED;
		Type ftype = Type::T_UNDEFINED;
		size_t func_sig_size = func_sig.size();
		size_t call_sig_size = signature.size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				is_arr = is_array(func_sig.at(i).type) && is_array(signature.at(i).type);
				ftype = is_arr ? func_sig.at(i).array_type : func_sig.at(i).type;
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
			for (size_t i = 0; i < call_sig_size; ++i) {
				if (!rest) {
					is_arr = is_array(func_sig.at(i).type) && is_array(signature.at(i).type);
					ftype = is_arr ? func_sig.at(i).array_type : func_sig.at(i).type;

					if (it->second.parameters[i].is_rest) {
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
		if (func_sig_size > call_sig_size) {
			for (size_t i = 0; i < func_sig_size; ++i) {
				if (i < call_sig_size) {
					is_arr = is_array(func_sig.at(i).type) && is_array(signature.at(i).type);
					ftype = is_arr ? func_sig.at(i).array_type : func_sig.at(i).type;
					stype = is_arr ? signature.at(i).array_type : signature.at(i).type;

					if (!match_type(ftype, stype) && !is_any(ftype)
						&& !is_void(stype) && !is_undefined(stype) && !is_any(stype)) {
						found = false;
						break;
					}
				}
				else {
					if (!it->second.parameters[i].default_value) {
						found = false;
						break;
					}
				}
			}

			// if found and exactly signature size (not rest)
			if (found) {
				return it->second;
			}
		}
	}

	throw std::runtime_error("definition of '" + identifier + "' function not found");
}

bool SemanticScope::already_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table.find(identifier) != structure_symbol_table.end();
}

bool SemanticScope::already_declared_variable(const std::string& identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool SemanticScope::already_declared_function(const std::string& identifier, const std::vector<TypeDefinition>& signature) {
	try {
		find_declared_function(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

void SemanticScope::declare_structure_definition(const std::string& name, const std::map<std::string, VariableDefinition>& variables, unsigned int row, unsigned int col) {
	StructureDefinition str_def(name, variables, row, col);
	structure_symbol_table[name] = str_def;
}

void SemanticScope::declare_variable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col) {
	SemanticVariable* var = new SemanticVariable(identifier, type, array_type, dim,
		type_name, type_name_space, value, is_const, row, col);
	variable_symbol_table[identifier] = var;
}

void SemanticScope::declare_variable(const std::string& identifier, SemanticVariable* var) {
	variable_symbol_table[identifier] = var;
}

void SemanticScope::declare_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
	Type array_type, const std::vector<ASTExprNode*>& dim, const std::vector<TypeDefinition>& signature,
	const std::vector<VariableDefinition>& parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, array_type, dim, signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_variable_function(const std::string& identifier, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, Type::T_UNDEFINED, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(),
		std::vector<TypeDefinition>(), std::vector<VariableDefinition>(), row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_basic_function(const std::string& identifier, Type type, std::vector<TypeDefinition> signature,
	std::vector<VariableDefinition> parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_array_function(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", array_type, dim, signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::declare_struct_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, row, col);
	function_symbol_table.insert(std::make_pair(identifier, fun));
}

void SemanticScope::change_variable_type_name(const std::string& identifier, const std::string& type_name) {
	variable_symbol_table[identifier]->value->type_name = type_name;
}

void SemanticScope::change_variable_type(const std::string& identifier, Type type) {
	variable_symbol_table[identifier]->value->type = type;
}

void SemanticScope::change_variable_value_type_name(const std::string& identifier, const std::string& type_name) {
	variable_symbol_table[identifier]->value->type_name = type_name;
}

void SemanticScope::change_variable_value_type(const std::string& identifier, Type type) {
	variable_symbol_table[identifier]->value->type = type;
}
