#include <iostream>
#include <cmath>

#include "compiler_scope.hpp"
//#include "interpreter.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;
using namespace parser;

CompilerScope::CompilerScope(const std::string& name) : name(name) {}

CompilerScope::CompilerScope() : name("") { }

std::string CompilerScope::get_name() {
	return name;
}

void CompilerScope::set_name(const std::string& name) {
	this->name = name;
}

bool CompilerScope::already_declared_variable(const std::string& identifier) {
	return variable_symbol_table.find(identifier) != variable_symbol_table.end();
}

bool CompilerScope::already_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table.find(identifier) != structure_symbol_table.end();
}

bool CompilerScope::already_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
	std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector_ptr, bool strict) {
	try {
		find_declared_function(identifier, signature, evaluate_access_vector_ptr, strict);
		return true;
	}
	catch (...) {
		return false;
	}
}

bool CompilerScope::already_declared_function_name(const std::string& identifier) {
	try {
		find_declared_functions(identifier);
		return true;
	}
	catch (...) {
		return false;
	}
}

void CompilerScope::declare_variable(const std::string& identifier) {
	variable_symbol_table[identifier] = variable_symbol_table.size();
}

void CompilerScope::declare_structure_definition(const std::string& name, std::map<std::string, VariableDefinition> variables, unsigned int row, unsigned int col) {
	StructureDefinition type(name, variables, row, col);
	structure_symbol_table[name] = (type);
}

void CompilerScope::declare_function(const std::string& identifier, interpreter_parameter_list_t variables, parser::ASTBlockNode* block, TypeDefinition type) {
	function_symbol_table.insert(std::make_pair(identifier, std::make_pair(std::make_tuple(variables, block, type), function_symbol_table.size())));
}

StructureDefinition CompilerScope::find_declared_structure_definition(const std::string& identifier) {
	return structure_symbol_table[identifier];
}

Variable* CompilerScope::find_declared_variable(const std::string& identifier) {
	auto var = variable_symbol_table[identifier];
	var->value->reset_ref();
	return var;
}

cp_int CompilerScope::find_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
	std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector_ptr, bool strict) {
	auto funcs = function_symbol_table.equal_range(identifier);

	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	for (auto& it = funcs.first; it != funcs.second; ++it) {
		if (!signature) {
			return it->second.second;
		}

		auto& func_params = std::get<0>(it->second.first);
		bool rest = false;
		auto found = true;
		TypeDefinition stype;
		TypeDefinition ftype;
		size_t func_sig_size = func_params.size();
		size_t call_sig_size = signature->size();

		// if signatures size match, handle normal cases
		if (func_sig_size == call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				ftype = std::get<1>(func_params.at(i));
				stype = signature->at(i);

				if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second.second;
			}
		}

		// if function signature is lesser than signature call, handle rest case
		found = true;
		if (func_sig_size >= 1 && func_sig_size < call_sig_size) {
			for (size_t i = 0; i < call_sig_size; ++i) {
				if (!rest) {
					ftype = std::get<1>(func_params.at(i));

					if (std::get<3>(std::get<0>(it->second.first)[i])) {
						rest = true;
						if (is_array(ftype.type)) {
							ftype = TypeDefinition(ftype.array_type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), ftype.type_name, ftype.type_name_space);
						}
					}

					if (!std::get<3>(std::get<0>(it->second.first)[i]) && i == func_params.size() - 1) {
						found = false;
						break;
					}
				}
				stype = signature->at(i);

				if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
					found = false;
					break;
				}
			}

			if (found) {
				return it->second.second;
			}
		}

		// if function signature is greater than signature call, handle default value cases
		found = true;
		if (func_sig_size > call_sig_size) {
			for (size_t i = 0; i < func_sig_size; ++i) {
				if (i < call_sig_size) {
					ftype = std::get<1>(func_params.at(i));
					stype = signature->at(i);

					if (!TypeDefinition::is_any_or_match_type(&ftype, ftype, nullptr, stype, evaluate_access_vector_ptr, strict)) {
						found = false;
						break;
					}
				}
				else {
					if (!std::get<2>(std::get<0>(it->second.first)[i])) {
						found = false;
						break;
					}
				}
			}

			// if found and exactly signature size (not rest)
			if (found) {
				return it->second.second;
			}
		}
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}

std::pair<interpreter_function_list_t::iterator, interpreter_function_list_t::iterator>
CompilerScope::find_declared_functions(const std::string& identifier) {
	auto funcs = function_symbol_table.equal_range(identifier);
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}
	return funcs;
}
