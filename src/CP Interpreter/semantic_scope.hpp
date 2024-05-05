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
		std::map<std::string, parser::SemanticVariable_t*> variable_symbol_table;
		std::multimap<std::string, parser::FunctionDefinition_t> function_symbol_table;

	public:
		SemanticScope();

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::Type>);

		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declare_variable(std::string, parser::Type, bool, parser::Type, parser::Type, std::vector<parser::ASTExprNode*>,
			std::string, std::map<std::string, parser::SemanticValue*>, parser::ASTExprNode*, bool,
			unsigned int, unsigned int, bool = false);
		void declare_variable(std::string, parser::Type, bool, parser::SemanticValue*, unsigned int, unsigned int, bool = false);
		void declare_function(std::string, parser::Type, std::string, parser::Type, parser::Type,
			std::vector<parser::ASTExprNode*>, std::vector<parser::Type>, std::vector<parser::VariableDefinition_t>,
			parser::ASTBlockNode*, unsigned int, unsigned int);

		void change_current_variable_type(std::string, parser::Type);
		void change_variable_type_name(std::string, std::string);

		parser::StructureDefinition_t find_declared_structure_definition(std::string);
		parser::FunctionDefinition_t find_declared_function(std::string, std::vector<parser::Type>);
		parser::SemanticVariable_t* find_declared_variable(std::string);

		parser::SemanticValue_t* access_value(std::vector<std::string>, std::vector<parser::ASTExprNode*>);
		parser::SemanticValue_t* access_value_of_array(parser::SemanticValue_t*, std::vector<parser::ASTExprNode*>);
		parser::SemanticValue_t* access_value_of_structure(parser::SemanticValue_t*, std::vector<std::string>);
	};
}

#endif // SEMANTIC_SCOPE_HPP
