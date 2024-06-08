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

		void declare_function(std::string identifier, parser::Type type, std::string type_name, std::string type_name_space,
			parser::Type array_type, std::vector<parser::ASTExprNode*> dim, std::vector<parser::TypeDefinition> signature,
			std::vector<parser::VariableDefinition> parameters, unsigned int row, unsigned int col);

		void declare_variable_function(std::string identifier, unsigned int row, unsigned int col);

		void declare_basic_function(std::string identifier, parser::Type type, std::vector<parser::TypeDefinition> signature,
			std::vector<parser::VariableDefinition> parameters, unsigned int row = 0, unsigned int col = 0);

		void declare_array_function(std::string identifier, parser::Type type, parser::Type array_type, std::vector<parser::ASTExprNode*> dim,
			std::vector<parser::TypeDefinition> signature, std::vector<parser::VariableDefinition> parameters,
			unsigned int row = 0, unsigned int col = 0);

		void declare_struct_function(std::string identifier, parser::Type type, std::string type_name, std::string type_name_space,
			std::vector<parser::TypeDefinition> signature, std::vector<parser::VariableDefinition> parameters, unsigned int row = 0, unsigned int col = 0);

		void change_current_variable_type(std::string, parser::Type);
		void change_variable_type_name(std::string, std::string);

		parser::StructureDefinition find_declared_structure_definition(std::string);
		parser::FunctionDefinition find_declared_function(std::string, std::vector<parser::TypeDefinition>);
		parser::SemanticVariable* find_declared_variable(std::string);

	};
}

#endif // !SEMANTIC_SCOPE_HPP
