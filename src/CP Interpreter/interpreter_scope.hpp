#ifndef INTERPRETER_SCOPE_HPP
#define INTERPRETER_SCOPE_HPP

#include <map>
#include <stack>

#include "visitor.hpp"
#include "ast.hpp"


namespace visitor {
	typedef std::map<std::string, parser::StructureDefinition> interpreter_struct_list_t;
	typedef std::map<std::string, Value*> interpreter_variable_list_t;
	typedef std::tuple<std::string, parser::TypeDefinition, parser::ASTExprNode*, bool> interpreter_parameter_t;
	typedef std::vector<interpreter_parameter_t> interpreter_parameter_list_t;
	typedef std::pair<interpreter_parameter_list_t, parser::ASTBlockNode*> interpreter_function_t;
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
		bool already_declared_function(std::string, std::vector<parser::TypeDefinition>);
		bool already_declared_function_name(std::string identifier);

		Value* declare_empty_variable(std::string identifier, parser::Type type, parser::Type empty_type);
		Value* declare_empty_struct_variable(std::string identifier, std::string type_name, parser::Type empty_type);
		Value* declare_variable(std::string identifier, parser::Type type, cp_bool);
		Value* declare_variable(std::string identifier, parser::Type type, cp_int);
		Value* declare_variable(std::string identifier, parser::Type type, cp_float);
		Value* declare_variable(std::string identifier, parser::Type type, cp_char);
		Value* declare_variable(std::string identifier, parser::Type type, cp_string);
		Value* declare_variable(std::string identifier, parser::Type type, cp_array, parser::Type arr_type);
		Value* declare_variable(std::string identifier, parser::Type type, cp_struct*);
		Value* declare_variable(std::string identifier, parser::Type type, cp_function);
		Value* declare_value(std::string, Value*);

		void declare_function(std::string identifier, interpreter_parameter_list_t variables, parser::ASTBlockNode* block);
		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition>, unsigned int, unsigned int);

		parser::StructureDefinition find_declared_structure_definition(std::string);
		Value* find_declared_variable(std::string);
		interpreter_function_t find_declared_function(std::string, std::vector<parser::TypeDefinition>);
		std::pair<interpreter_function_list_t::iterator, interpreter_function_list_t::iterator> find_declared_functions(std::string identifier);

		std::string get_name();
		void set_name(std::string);
	};
}

#endif // !INTERPRETER_SCOPE_HPP
