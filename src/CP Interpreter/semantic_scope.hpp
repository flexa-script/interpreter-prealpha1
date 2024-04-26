#ifndef SEMANTIC_SCOPE_HPP
#define SEMANTIC_SCOPE_HPP

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "ast.hpp"


namespace visitor {

	class SemanticScope {
	private:
		std::map<std::string, parser::StructureDefinition_t> structure_symbol_table;
		std::map<std::string, parser::VariableDefinition_t> variable_symbol_table;
		std::multimap<std::string, parser::FunctionDefinition_t> function_symbol_table;

	public:
		SemanticScope();

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::Type>);

		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declare_variable(std::string, parser::Type, std::string, parser::Type, std::vector<parser::ASTExprNode*>, parser::ASTExprNode*, bool, bool, unsigned int, unsigned int, bool);
		void declare_function(std::string, parser::Type, std::string, std::vector<parser::Type>, bool, unsigned int, unsigned int);

		void assign_variable(std::string, bool);
		void change_variable_type(std::string, parser::Type);
		void change_variable_type_name(std::string, std::string);

		parser::StructureDefinition_t find_declared_structure_definition(std::string);
		parser::VariableDefinition_t find_declared_variable(std::string);
		parser::FunctionDefinition_t find_declared_function(std::string, std::vector<parser::Type>);
	};
}

#endif // SEMANTIC_SCOPE_HPP
