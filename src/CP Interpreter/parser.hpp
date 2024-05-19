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
		Type current_array_type = Type::T_UNDEF;
		bool consume_semicolon = false;

	public:
		std::string name;

	public:
		explicit Parser(lexer::Lexer*, std::string);
		Parser(lexer::Lexer*, std::string, unsigned int);

		ASTProgramNode* parse_program();
		ASTExprNode* parse_expression();  // public for repl
		ASTExprNode* parse_statement_expression();  // public for repl

	private:
		void consume_token();
		void consume_token(lexer::TokenType);
		void check_current_token(lexer::TokenType);

		// parse types and parameters
		Type parse_type();

		std::string msg_header();

		// statement nodes
		ASTNode* parse_program_statement();

		ASTUsingNode* parse_using_statement();

		ASTNode* parse_block_statement();

		ASTDeclarationNode* parse_declaration_statement();

		ASTNode* parse_assignment_or_increment_node();

		ASTAssignmentNode* parse_assignment_statement(ASTIdentifierNode*);

		ASTUnaryExprNode* parse_increment_expression(ASTIdentifierNode*);

		ASTPrintNode* parse_print_statement();

		ASTReturnNode* parse_return_statement();

		ASTBlockNode* parse_block();

		ASTBlockNode* parse_struct_block();

		ASTStatementNode* parse_struct_block_variables();

		ASTContinueNode* parse_continue_statement();

		ASTBreakNode* parse_break_statement();

		ASTSwitchNode* parse_switch_statement();

		ASTElseIfNode* parse_else_if_statement();

		ASTIfNode* parse_if_statement();

		ASTForNode* parse_for_statement();

		ASTNode* parse_foreach_collection();

		ASTForEachNode* parse_foreach_statement();

		ASTWhileNode* parse_while_statement();

		ASTFunctionDefinitionNode* parse_function_definition();

		ASTStructDefinitionNode* parse_struct_definition();

		ASTNode* parse_identifier_statement();

		ASTArrayConstructorNode* parse_array_constructor_node();

		ASTStructConstructorNode* parse_struct_constructor_node(std::string = "");

		ASTFunctionCallNode* parse_function_call_node(std::string = "");

		ASTFunctionCallNode* parse_function_call_parameters_node(std::string, std::string);

		ASTReadNode* parse_read_node();

		ASTTypeParseNode* parse_type_parse_node();

		ASTThisNode* parse_this_node();

		ASTTypeNode* parse_type_node();

		ASTLenNode* parse_len_node();

		ASTRoundNode* parse_round_node();

		// expression nodes

		ASTExprNode* parse_expression_tail(ASTExprNode*);

		ASTExprNode* parse_logical_expression();

		ASTExprNode* parse_logical_expression_tail(ASTExprNode*);

		ASTExprNode* parse_relational_expression();

		ASTExprNode* parse_relational_expression_tail(ASTExprNode*);

		ASTExprNode* parse_simple_expression();

		ASTExprNode* parse_simple_expression_tail(ASTExprNode*);

		ASTExprNode* parse_term();

		ASTExprNode* parse_term_tail(ASTExprNode*);

		ASTExprNode* parse_factor();

		// special

		std::vector<ASTExprNode*>* parse_actual_params();

		VariableDefinition_t* parse_formal_param();

		ASTIdentifierNode* parse_identifier_node(std::string = "");

		cp_bool parse_bool_literal();

		cp_int parse_int_literal();

		cp_float parse_float_literal();

		cp_char parse_char_literal();

		cp_string parse_string_literal();

		std::vector<ASTExprNode*> parse_dimension_vector();

		Identifier_t parse_identifier();

		std::vector<Identifier_t> parse_identifier_vector();

	};
}

#endif // PARSER_HPP
