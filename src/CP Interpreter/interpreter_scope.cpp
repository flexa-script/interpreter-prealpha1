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

bool InterpreterScope::already_declared_function(std::string identifier, std::vector<TypeDefinition> signature,
	std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector_ptr, bool strict) {
	try {
		find_declared_function(identifier, signature, evaluate_access_vector_ptr, strict);
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

Variable* InterpreterScope::declare_variable(std::string identifier, Variable* value) {
	variable_symbol_table[identifier] = value;
	return value;
}

void InterpreterScope::declare_structure_definition(std::string name, std::map<std::string, VariableDefinition> variables, unsigned int row, unsigned int col) {
	StructureDefinition type(name, variables, row, col);
	structure_symbol_table[name] = (type);
}

void InterpreterScope::declare_function(std::string identifier, interpreter_parameter_list_t variables, parser::ASTBlockNode* block, TypeDefinition type) {
	function_symbol_table.insert(std::make_pair(identifier, std::make_tuple(variables, block, type)));
}

StructureDefinition InterpreterScope::find_declared_structure_definition(std::string identifier) {
	return structure_symbol_table[identifier];
}

Variable* InterpreterScope::find_declared_variable(std::string identifier) {
	auto var = variable_symbol_table[identifier];
	var->use_ref = is_struct(var->type);
	return var;
}

interpreter_function_t InterpreterScope::find_declared_function(std::string identifier, std::vector<TypeDefinition> signature,
	std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector_ptr, bool strict) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		auto& func_params = std::get<0>(it->second);
		bool rest = false;
		auto found = true;
		TypeDefinition stype;
		TypeDefinition ftype;
		size_t func_sig_size = func_params.size();
		size_t call_sig_size = signature.size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				ftype = std::get<1>(func_params.at(i));
				stype = signature.at(i);

				if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
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
					ftype = std::get<1>(func_params.at(i));

					if (std::get<3>(std::get<0>(it->second)[i])) {
						rest = true;
					}
				}
				stype = signature.at(i);

				if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
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
					ftype = std::get<1>(func_params.at(i));
					stype = signature.at(i);

					if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
						found = false;
						break;
					}
				}
				else {
					if (!std::get<2>(std::get<0>(it->second)[i])) {
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
