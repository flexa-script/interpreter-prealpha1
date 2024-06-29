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
		std::map<std::string, visitor::StructureDefinition> structure_symbol_table;
		std::map<std::string, visitor::SemanticVariable*> variable_symbol_table;
		std::multimap<std::string, visitor::FunctionDefinition> function_symbol_table;

	public:
		SemanticScope();
		~SemanticScope();

		bool already_declared_structure_definition(const std::string& identifier);
		bool already_declared_variable(const std::string& identifier);
		bool already_declared_function(const std::string& identifier, const std::vector<visitor::TypeDefinition>& signature);

		void declare_structure_definition(const std::string& name, const std::vector<visitor::VariableDefinition>& variables, unsigned int row, unsigned int col);

		void declare_variable(const std::string& identifier, parser::Type type, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::string& type_name, const std::string&, visitor::SemanticValue* value, bool is_const, unsigned int row, unsigned int col);

		void declare_function(const std::string& identifier, parser::Type type, const std::string& type_name, const std::string& type_name_space,
			parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim, const std::vector<visitor::TypeDefinition>& signature,
			const std::vector<visitor::VariableDefinition>& parameters, unsigned int row, unsigned int col);

		void declare_variable_function(const std::string& identifier, unsigned int row, unsigned int col);

		void declare_basic_function(const std::string& identifier, parser::Type type, std::vector<visitor::TypeDefinition> signature,
			std::vector<visitor::VariableDefinition> parameters, unsigned int row = 0, unsigned int col = 0);

		void declare_array_function(const std::string& identifier, parser::Type type, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::vector<visitor::TypeDefinition>& signature, const std::vector<visitor::VariableDefinition>& parameters,
			unsigned int row = 0, unsigned int col = 0);

		void declare_struct_function(const std::string& identifier, parser::Type type, const std::string& type_name, const std::string& type_name_space,
			const std::vector<visitor::TypeDefinition>& signature, const std::vector<visitor::VariableDefinition>& parameters, unsigned int row = 0, unsigned int col = 0);

		void change_variable_type(const std::string& identifier, parser::Type type);
		void change_variable_type_name(const std::string& identifier, const std::string& type_name);
		void change_variable_value_type(const std::string& identifier, parser::Type type);
		void change_variable_value_type_name(const std::string& identifier, const std::string& type_name);

		visitor::StructureDefinition find_declared_structure_definition(const std::string& identifier);
		visitor::FunctionDefinition find_declared_function(const std::string& identifier, const std::vector<visitor::TypeDefinition>& signature);
		visitor::SemanticVariable* find_declared_variable(const std::string& identifier);

	};
}

#endif // !SEMANTIC_SCOPE_HPP
