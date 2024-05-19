#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
#include <stack>
#include <any>

#include "visitor.hpp"
#include "ast.hpp"
#include "interpreter_scope.hpp"


namespace visitor {

	class Interpreter : public Visitor {
	private:
		std::map<std::string, std::vector<InterpreterScope*>> scopes;
		std::vector<std::string> libs;
		Value_t current_expression_value;
		std::vector<std::string> current_function_parameters;
		std::vector<std::pair<parser::Type, Value_t*>> current_function_arguments;
		std::string current_function_name;
		std::string current_function_nmspace;
		bool is_function_context;
		std::string return_from_function_name;
		bool return_from_function = false;
		bool is_switch = false;
		bool is_loop = false;
		bool continue_block = false;
		bool break_block = false;
		bool executed_elif = false;
		bool has_string_access = false;

	private:
		//bool is_namespace(std::string);

		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		void declare_new_structure(std::string, Value_t);
		void declare_structure(cp_struct*, std::string);
		void print_value(Value_t);
		void print_array(cp_array);
		void print_struct(cp_struct);

		cp_int do_operation(cp_int, cp_int, std::string);
		cp_float do_operation(cp_float, cp_float, std::string);
		std::string parse_value_to_string(Value_t);
		std::string parse_array_to_string(cp_array);
		std::string parse_struct_to_string(cp_struct);

		Value_t* access_value(InterpreterScope*, Value_t*, std::vector<parser::Identifier_t>, size_t i = 0);

		std::string msg_header(unsigned int, unsigned int);

	public:
		Interpreter() = default;
		Interpreter(InterpreterScope*, parser::ASTProgramNode*, std::map<std::string, parser::ASTProgramNode*>);
		~Interpreter() = default;

		void start();

		std::string get_namespace(std::string = "") override;
		std::string get_namespace(parser::ASTProgramNode*, std::string = "") override;

		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTPrintNode*) override;
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
		void visit(parser::ASTTypeNode*) override;
		void visit(parser::ASTLenNode*) override;
		void visit(parser::ASTRoundNode*) override;
		void visit(parser::ASTReadNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;

		unsigned int hash(parser::ASTExprNode*) override;
		unsigned int hash(parser::ASTIdentifierNode*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_bool>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_int>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_float>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_char>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_string>*) override;

		std::pair<parser::Type, Value_t*> current_expr();
		Value_t get_current_expression_value() {
			return current_expression_value;
		}
	};
}

#endif // INTERPRETER_HPP
