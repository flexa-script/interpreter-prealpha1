#include <utility>
#include <iostream>

#include "compiler_scope.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;
using namespace parser;

CompilerScope::CompilerScope() = default;

CompilerScope::~CompilerScope() = default;

std::pair<StructureDefinition, size_t> CompilerScope::find_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table[identifier];
}

std::pair<TypeDefinition, size_t> CompilerScope::find_declared_variable(const std::string& identifier) {
	return variable_symbol_table[identifier];
}

std::pair<FunctionDefinition, size_t>& CompilerScope::find_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
	std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		if (it->second.first.is_var || !signature) {
			return it->second;
		}

		auto& func_sig = it->second.first.signature;
		bool rest = false;
		auto found = true;
		TypeDefinition stype;
		TypeDefinition ftype;
		size_t func_sig_size = func_sig.size();
		size_t call_sig_size = signature->size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				ftype = func_sig.at(i);
				stype = signature->at(i);

				if (!TypeDefinition::is_any_or_match_type(ftype, stype, evaluate_access_vector, strict || stype.use_ref)) {
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
		if (func_sig_size >= 1 && func_sig_size < call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				if (!rest) {
					ftype = func_sig.at(i);

					if (it->second.first.parameters[i].is_rest) {
						rest = true;
						if (is_array(ftype.type)) {
							ftype = TypeDefinition(ftype.array_type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), ftype.type_name, ftype.type_name_space);
						}
					}

					if (!it->second.first.parameters[i].is_rest && i == func_sig.size() -1) {
						found = false;
						break;
					}
				}
				stype = signature->at(i);

				if (!TypeDefinition::is_any_or_match_type(ftype, stype, evaluate_access_vector, strict || stype.use_ref)) {
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
					ftype = func_sig.at(i);
					stype = signature->at(i);

					if (!TypeDefinition::is_any_or_match_type(ftype, stype, evaluate_access_vector, strict || stype.use_ref)) {
						found = false;
						break;
					}
				}
				else {
					if (!it->second.first.parameters[i].default_value) {
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

bool CompilerScope::already_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table.find(identifier) != structure_symbol_table.end();
}

bool CompilerScope::already_declared_variable(const std::string& identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool CompilerScope::already_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
	std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict) {
	try {
		find_declared_function(identifier, signature, evaluate_access_vector, strict);
		return true;
	}
	catch (...) {
		return false;
	}
}

size_t CompilerScope::declare_structure_definition(const std::string& name, const std::map<std::string, VariableDefinition>& variables, unsigned int row, unsigned int col) {
	StructureDefinition str_def(name, variables, row, col);
	auto id = structure_symbol_table.size();
	structure_symbol_table[name] = std::make_pair(str_def, id);
	return id;
}

size_t CompilerScope::declare_variable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, unsigned int row, unsigned int col) {
	auto id = variable_symbol_table.size();
	variable_symbol_table[identifier] = std::make_pair(TypeDefinition(type, array_type, dim, type_name, type_name_space), id);
	return id;
}

size_t CompilerScope::declare_variable(const std::string& identifier, TypeDefinition var) {
	auto id = variable_symbol_table.size();
	variable_symbol_table[identifier] = std::make_pair(var, id);
	return id;
}

size_t CompilerScope::declare_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
	Type array_type, const std::vector<ASTExprNode*>& dim, const std::vector<TypeDefinition>& signature,
	const std::vector<VariableDefinition>& parameters, ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, array_type, dim, signature, parameters, block, row, col);
	auto id = function_symbol_table.size();
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(fun, id)));
	return id;
}

size_t CompilerScope::declare_variable_function(const std::string& identifier, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, row, col);
	auto id = function_symbol_table.size();
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(fun, id)));
	return id;
}

size_t CompilerScope::declare_basic_function(const std::string& identifier, Type type, std::vector<TypeDefinition> signature,
	std::vector<VariableDefinition> parameters, ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, block, row, col);
	auto id = function_symbol_table.size();
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(fun, id)));
	return id;
}

size_t CompilerScope::declare_array_function(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, "", "", array_type, dim, signature, parameters, block, row, col);
	auto id = function_symbol_table.size();
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(fun, id)));
	return id;
}

size_t CompilerScope::declare_struct_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, ASTBlockNode* block, unsigned int row, unsigned int col) {
	FunctionDefinition fun(identifier, type, type_name, type_name_space, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), signature, parameters, block, row, col);
	auto id = function_symbol_table.size();
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(fun, id)));
	return id;
}
