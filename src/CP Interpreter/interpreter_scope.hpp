#ifndef INTERPRETER_SCOPE_HPP
#define INTERPRETER_SCOPE_HPP

#include <map>
#include <stack>

#include "visitor.hpp"
#include "ast.hpp"


using namespace visitor;
using namespace parser;


namespace visitor {
	typedef std::map<std::string, visitor::StructureDefinition> interpreter_struct_list_t;
	typedef std::map<std::string, Variable*> interpreter_variable_list_t;
	typedef std::tuple<std::string, visitor::TypeDefinition, parser::ASTExprNode*, bool> interpreter_parameter_t;
	typedef std::vector<interpreter_parameter_t> interpreter_parameter_list_t;
	typedef std::tuple<interpreter_parameter_list_t, parser::ASTBlockNode*, visitor::TypeDefinition> interpreter_function_t;
	typedef std::multimap<std::string, interpreter_function_t> interpreter_function_list_t;

	class Interpreter;

	class InterpreterScope {
	public:
		bool has_string_access = false;

	private:
		std::string name;
		interpreter_struct_list_t structure_symbol_table;
		interpreter_variable_list_t variable_symbol_table;
		interpreter_function_list_t function_symbol_table;

	public:
		InterpreterScope();
		InterpreterScope(std::string);
		~InterpreterScope() = default;

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string identifier, std::vector<TypeDefinition> signature, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector);
		bool already_declared_function_name(std::string identifier);

		Variable* declare_variable(std::string, Variable*);

		void declare_function(std::string identifier, interpreter_parameter_list_t variables, parser::ASTBlockNode* block, visitor::TypeDefinition type);
		void declare_structure_definition(std::string, std::map<std::string, visitor::VariableDefinition>, unsigned int, unsigned int);

		visitor::StructureDefinition find_declared_structure_definition(std::string);
		Variable* find_declared_variable(std::string);
		interpreter_function_t find_declared_function(std::string identifier, std::vector<TypeDefinition> signature, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector);
		std::pair<interpreter_function_list_t::iterator, interpreter_function_list_t::iterator> find_declared_functions(std::string identifier);

		std::string get_name();
		void set_name(std::string);
	};
}

#endif // !INTERPRETER_SCOPE_HPP
