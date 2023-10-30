#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"


namespace parser {
	class Parser {
	private:
		lexer::Lexer* lex;
		lexer::Token current_token;
		lexer::Token next_token;

	public:
		std::string name;

	public:
		explicit Parser(lexer::Lexer*, std::string);

		Parser(lexer::Lexer*, std::string, unsigned int);

		ASTProgramNode* parse_program();

		ASTExprNode* parse_expression();  // public for repl

	private:
		void consume_token();

		// Statement Nodes
		ASTStatementNode* parse_program_statement();

		ASTUsingNode* parse_using_statement();

		ASTStatementNode* parse_block_statement();

		ASTDeclarationNode* parse_declaration_statement();

		ASTAssignmentNode* parse_assignment_statement();

		ASTPrintNode* parse_print_statement();

		ASTReadNode* parse_read_statement();

		ASTFunctionCallNode* parse_function_call();

		ASTReturnNode* parse_return_statement();

		ASTBlockNode* parse_block();

		ASTIfNode* parse_if_statement();

		ASTWhileNode* parse_while_statement();

		ASTFunctionDefinitionNode* parse_function_definition();

		ASTStatementNode* parse_identifier();

		// Expression Nodes
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

		ASTExprFunctionCallNode* parse_expr_function_call();

		ASTExprReadNode* parse_expr_read_node();

		ASTFloatParseNode* parse_float_parse_exp();

		ASTIntParseNode* parse_int_parse_exp();

		ASTStringParseNode* parse_string_parse_exp();

		// Parse Types and parameters
		TYPE parse_type(std::string&);

		std::vector<ASTExprNode*>* parse_actual_params();

		std::pair<std::string, TYPE>* parse_formal_param();
	};
}

#endif //PARSER_H
