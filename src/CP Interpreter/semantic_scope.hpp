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
		std::map<std::string, parser::StructureDefinition> structure_symbol_table;
		std::map<std::string, parser::SemanticVariable*> variable_symbol_table;
		std::multimap<std::string, parser::FunctionDefinition> function_symbol_table;

	public:
		SemanticScope();
		~SemanticScope();

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::TypeDefinition>);

		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition>, unsigned int, unsigned int);
		void declare_variable(std::string, parser::Type, parser::Type, std::vector<parser::ASTExprNode*>,
			std::string, std::string, parser::SemanticValue*, bool, unsigned int, unsigned int);
		void declare_function(std::string, parser::Type, std::string, std::string, parser::Type,
			std::vector<parser::ASTExprNode*>, std::vector<parser::TypeDefinition>, std::vector<parser::VariableDefinition>,
			parser::ASTBlockNode*, unsigned int, unsigned int);

		void change_current_variable_type(std::string, parser::Type);
		void change_variable_type_name(std::string, std::string);

		parser::StructureDefinition find_declared_structure_definition(std::string);
		parser::FunctionDefinition find_declared_function(std::string, std::vector<parser::TypeDefinition>);
		parser::SemanticVariable* find_declared_variable(std::string);

	};
}

#endif // SEMANTIC_SCOPE_HPP
