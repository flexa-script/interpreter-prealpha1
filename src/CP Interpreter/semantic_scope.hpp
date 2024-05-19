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
		SemanticScope(std::string);
		SemanticScope();
		~SemanticScope();

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::Type>);

		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declare_variable(std::string, parser::Type, parser::Type, std::vector<parser::ASTExprNode*>,
			std::string, parser::SemanticValue*, bool, unsigned int, unsigned int);
		void declare_function(std::string, parser::Type, std::string/*, parser::Type*/, parser::Type,
			std::vector<parser::ASTExprNode*>, std::vector<parser::Type>, std::vector<parser::VariableDefinition_t>,
			parser::ASTBlockNode*, unsigned int, unsigned int);

		void change_current_variable_type(std::string, parser::Type);
		void change_variable_type_name(std::string, std::string);

		parser::StructureDefinition_t find_declared_structure_definition(std::string);
		parser::FunctionDefinition_t find_declared_function(std::string, std::vector<parser::Type>);
		parser::SemanticVariable_t* find_declared_variable(std::string);

		std::string get_name();
		void set_name(std::string);
	};
}

#endif // SEMANTIC_SCOPE_HPP
