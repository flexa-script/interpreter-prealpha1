#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
#include <stack>
#include <any>
#include <functional>

#include "visitor.hpp"
#include "ast.hpp"
#include "interpreter_scope.hpp"


namespace modules {
	class Graphics;
	class Files;
}

namespace visitor {

	class Interpreter : public Visitor {
	public:
		std::map<std::string, std::function<void()>> builtin_functions;
		std::vector<std::pair<parser::Type, Value*>> last_function_arguments;
		Value current_expression_value;

	private:
		std::map<std::string, std::vector<InterpreterScope*>> scopes;
		std::vector<std::string> libs;
		std::string function_call_name;
		std::vector<std::string> function_call_parameters;
		std::stack<std::string> current_function_nmspace;
		std::string return_from_function_name;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		bool is_function_context;
		bool return_from_function = false;
		bool is_switch = false;
		bool is_loop = false;
		bool continue_block = false;
		bool break_block = false;
		bool executed_elif = false;
		bool has_string_access = false;

		std::vector<std::string> built_in_libs = {
			"cp.core.graphics",
			"cp.core.files"
		};

		modules::Graphics* cpgraphics;
		modules::Files* cpfiles;


		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		std::vector<unsigned int> calculate_array_dim_size(cp_array);

		std::vector<Value*> build_array(std::vector<parser::ASTExprNode*>, Value*, long long);

		void declare_new_structure(std::string, Value);
		void declare_structure(cp_struct*, std::string);

		cp_int do_operation(cp_int, cp_int, std::string);
		cp_float do_operation(cp_float, cp_float, std::string);
		cp_string do_operation(cp_string, cp_string, std::string);
		std::string parse_value_to_string(Value);
		std::string parse_array_to_string(cp_array);
		std::string parse_struct_to_string(cp_struct);

		Value* access_value(InterpreterScope*, Value*, std::vector<parser::Identifier>, size_t i = 0);

		void register_built_in_functions();
		void register_built_in_lib(std::string libname);

		std::string msg_header(unsigned int, unsigned int);

	public:
		Interpreter(InterpreterScope*, parser::ASTProgramNode*, std::map<std::string, parser::ASTProgramNode*>);
		Interpreter() = default;
		~Interpreter() = default;

		std::string get_namespace(std::string = "") override;
		std::string get_namespace(parser::ASTProgramNode*, std::string = "") override;

		void start();

		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTBlockNode*) override;
		void visit(parser::ASTContinueNode*) override;
		void visit(parser::ASTBreakNode*) override;
		void visit(parser::ASTSwitchNode*) override;
		void visit(parser::ASTElseIfNode*) override;
		void visit(parser::ASTIfNode*) override;
		void visit(parser::ASTForNode*) override;
		void visit(parser::ASTForEachNode*) override;
		void visit(parser::ASTWhileNode*) override;
		void visit(parser::ASTFunctionDefinitionNode*) override;
		void visit(parser::ASTStructDefinitionNode*) override;
		void visit(parser::ASTLiteralNode<cp_bool>*) override;
		void visit(parser::ASTLiteralNode<cp_int>*) override;
		void visit(parser::ASTLiteralNode<cp_float>*) override;
		void visit(parser::ASTLiteralNode<cp_char>*) override;
		void visit(parser::ASTLiteralNode<cp_string>*) override;
		void visit(parser::ASTArrayConstructorNode*) override;
		void visit(parser::ASTStructConstructorNode*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;
		void visit(parser::ASTTypeofNode*) override;

		unsigned int hash(parser::ASTExprNode*) override;
		unsigned int hash(parser::ASTIdentifierNode*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_bool>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_int>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_float>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_char>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_string>*) override;

	};
}

#endif // !INTERPRETER_HPP
