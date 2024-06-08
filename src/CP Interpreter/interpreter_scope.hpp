#ifndef INTERPRETER_SCOPE_HPP
#define INTERPRETER_SCOPE_HPP

#include <map>
#include <stack>

#include "visitor.hpp"
#include "ast.hpp"


namespace visitor {
	typedef std::map<std::string, parser::StructureDefinition> interpreter_struct_t;
	typedef std::map<std::string, Value*> interpreter_variable_t;
	typedef std::tuple<std::vector<parser::TypeDefinition>, std::vector<std::string>, parser::ASTBlockNode*> interpreter_function_t;

	class Interpreter;

	class InterpreterScope {
	public:
		bool has_string_access = false;

	private:
		std::string name;
		interpreter_struct_t structure_symbol_table;
		interpreter_variable_t variable_symbol_table;
		std::multimap<std::string, interpreter_function_t> function_symbol_table;

	public:
		InterpreterScope();
		InterpreterScope(std::string);

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::TypeDefinition>);

		Value* declare_undef_variable(std::string, parser::Type);
		Value* declare_undef_struct_variable(std::string, std::string);
		Value* declare_null_variable(std::string, parser::Type);
		Value* declare_null_struct_variable(std::string, std::string);
		Value* declare_variable(std::string, cp_bool);
		Value* declare_variable(std::string, cp_int);
		Value* declare_variable(std::string, cp_float);
		Value* declare_variable(std::string, cp_char);
		Value* declare_variable(std::string, cp_string);
		Value* declare_variable(std::string, cp_array);
		Value* declare_variable(std::string, cp_struct*);
		Value* declare_value(std::string, Value*);

		void declare_function(std::string, std::vector<parser::TypeDefinition>, std::vector<std::string>, parser::ASTBlockNode*);
		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition>, unsigned int, unsigned int);

		parser::StructureDefinition find_declared_structure_definition(std::string);
		Value* find_declared_variable(std::string);
		interpreter_function_t find_declared_function(std::string, std::vector<parser::TypeDefinition>);

		std::string get_name();
		void set_name(std::string);
	};
}

#endif // !INTERPRETER_SCOPE_HPP
