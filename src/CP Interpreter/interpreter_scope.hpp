#ifndef INTERPRETER_SCOPE_HPP
#define INTERPRETER_SCOPE_HPP

#include <map>
#include <stack>
#include <any>

#include "visitor.hpp"
#include "ast.hpp"


namespace visitor {
	class Interpreter;

	class InterpreterScope {
	public:
		bool has_string_access = false;

	private:
		std::string name;
		std::map<std::string, parser::StructureDefinition> structure_symbol_table;
		std::map<std::string, Value*> variable_symbol_table;
		std::multimap<std::string, std::tuple<std::vector<parser::Type>, std::vector<std::string>, parser::ASTBlockNode*>> function_symbol_table;

	public:
		InterpreterScope();
		InterpreterScope(std::string);

		bool already_declared_structure_definition(std::string);
		bool already_declared_variable(std::string);
		bool already_declared_function(std::string, std::vector<parser::Type>);

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

		void declare_function(std::string, std::vector<parser::Type>, std::vector<std::string>, parser::ASTBlockNode*);
		void declare_structure_definition(std::string, std::vector<parser::VariableDefinition>, unsigned int, unsigned int);

		parser::StructureDefinition find_declared_structure_definition(std::string);
		Value* find_declared_variable(std::string);
		std::tuple<std::vector<parser::Type>, std::vector<std::string>, parser::ASTBlockNode*> find_declared_function(std::string, std::vector<parser::Type>);

		std::string get_name();
		void set_name(std::string);
	};
}

#endif // INTERPRETER_SCOPE_HPP
