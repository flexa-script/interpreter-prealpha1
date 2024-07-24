#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast.hpp"
#include "lexer.hpp"


namespace parser {

	class Parser {
	private:
		lexer::Lexer* lex;
		lexer::Token current_token;
		lexer::Token next_token;
		Type current_array_type = Type::T_UNDEFINED;
		bool consume_semicolon = false;

	public:
		std::string name;

	public:
		explicit Parser(const std::string& name, lexer::Lexer* lex);

		ASTProgramNode* parse_program();

	private:
		void consume_token();
		void consume_token(lexer::TokenType type);
		void check_current_token(lexer::TokenType type);
		void check_consume_semicolon();

		// parse types and parameters
		Type parse_type();

		std::string msg_header();

		// statement nodes
		ASTNode* parse_program_statement();
		ASTAsNamespaceNode* parse_as_namespace_statement();
		ASTUsingNode* parse_using_statement();
		ASTNode* parse_block_statement();
		ASTDeclarationNode* parse_declaration_statement();
		ASTStatementNode* parse_undef_declaration_statement();
		ASTAssignmentNode* parse_assignment_statement(ASTIdentifierNode* idnode);
		ASTUnaryExprNode* parse_increment_expression(ASTIdentifierNode* idnode);
		ASTReturnNode* parse_return_statement();
		ASTExitNode* parse_exit_statement();
		ASTEnumNode* parse_enum_statement();
		ASTBlockNode* parse_block();
		ASTBlockNode* parse_struct_block();
		ASTStatementNode* parse_struct_block_variables();
		ASTContinueNode* parse_continue_statement();
		ASTBreakNode* parse_break_statement();
		ASTSwitchNode* parse_switch_statement();
		ASTElseIfNode* parse_else_if_statement();
		ASTIfNode* parse_if_statement();
		ASTTryCatchNode* parse_try_catch_statement();
		ASTThrowNode* parse_throw_statement();
		ASTForNode* parse_for_statement();
		ASTNode* parse_foreach_collection();
		ASTForEachNode* parse_foreach_statement();
		ASTWhileNode* parse_while_statement();
		ASTDoWhileNode* parse_do_while_statement();
		ASTFunctionExpression* parse_function_expression();
		ASTFunctionDefinitionNode* parse_function_statement();
		ASTFunctionDefinitionNode* parse_function_definition(const std::string& identifier);
		ASTStructDefinitionNode* parse_struct_definition();

		// expression nodes
		ASTExprNode* parse_statement_expression();
		ASTExprNode* parse_expression();
		ASTExprNode* parse_ternary_expression();
		ASTExprNode* parse_in_expression();
		ASTExprNode* parse_logical_or_expression();
		ASTExprNode* parse_logical_and_expression();
		ASTExprNode* parse_bitwise_or_expression(); 
		ASTExprNode* parse_bitwise_xor_expression();
		ASTExprNode* parse_bitwise_and_expression();
		ASTExprNode* parse_equality_expression();
		ASTExprNode* parse_relational_expression();
		ASTExprNode* parse_spaceship_expression();
		ASTExprNode* parse_bitwise_shift_expression();
		ASTExprNode* parse_simple_expression();
		ASTExprNode* parse_term();
		ASTExprNode* parse_exponentiation();
		ASTExprNode* parse_factor();

		// factor expression nodes
		ASTNode* parse_identifier_statement();
		ASTArrayConstructorNode* parse_array_constructor_node();
		ASTStructConstructorNode* parse_struct_constructor_node(ASTIdentifierNode* idnode);
		ASTFunctionCallNode* parse_function_call_node(ASTIdentifierNode* idnode);
		ASTThisNode* parse_this_node();
		ASTTypeParseNode* parse_type_parse_node();
		ASTTypingNode* parse_typing_node();

		// special
		std::vector<ASTExprNode*>* parse_actual_params();
		VariableDefinition* parse_struct_var_def();
		VariableDefinition* parse_formal_param();
		ASTExprNode* parse_identifier_expression();
		ASTIdentifierNode* parse_identifier_node();
		cp_bool parse_bool_literal();
		cp_int parse_int_literal();
		cp_float parse_float_literal();
		cp_char parse_char_literal();
		cp_string parse_string_literal();
		std::vector<ASTExprNode*> parse_dimension_vector();
		Identifier parse_identifier();
		std::vector<Identifier> parse_identifier_vector();

	};
}

#endif // !PARSER_HPP
