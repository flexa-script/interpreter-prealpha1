#include <iostream>

#include "parser.h"


using namespace parser;


Parser::Parser(lexer::Lexer* lex, std::string name) : lex(lex), name(name) {
	currentToken = lex->nextToken();
	nextToken = lex->nextToken();
}

Parser::Parser(lexer::Lexer* lex, std::string name, unsigned int tokens) : lex(lex), name(name) {
	nextToken = lex->nextToken();

	for (unsigned int i = 0; i < tokens; i++)
		consume_token();
}

void Parser::consume_token() {
	currentToken = nextToken;
	nextToken = lex->nextToken();
}

ASTProgramNode* Parser::parse_program() {
	auto statements = new std::vector<ASTNode*>;

	while (currentToken.type == lexer::TOK_USING && currentToken.type != lexer::TOK_EOF) {
		statements->push_back(parse_using_statement());
		consume_token();
	}

	while (currentToken.type != lexer::TOK_EOF) {
		statements->push_back(parse_program_statement());
		consume_token();
	}

	return new ASTProgramNode(*statements, name);
}

ASTUsingNode* Parser::parse_using_statement() {
	// Node attributes
	std::string library;
	unsigned int lineNumber;

	// Determine line number
	lineNumber = currentToken.lineNumber;

	// Consume using
	consume_token();

	library = currentToken.value;

	consume_token();
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after library " + library + " on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	return new ASTUsingNode(library, lineNumber);
}

ASTStatementNode* Parser::parse_program_statement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
		return parse_declaration_statement();
	case lexer::TOK_DEF:
		return parse_function_definition();
	case lexer::TOK_PRINT:
		return parse_print_statement();
	case lexer::TOK_READ:
		return parse_read_statement();
	case lexer::TOK_IF:
		return parse_if_statement();
	case lexer::TOK_WHILE:
		return parse_while_statement();
	case lexer::TOK_RETURN:
		return parse_return_statement();
	case lexer::TOK_IDENTIFIER:
		return parse_identifier();

	default:
		throw std::runtime_error("Invalid global statement starting with '" +
			currentToken.value
			+ "' encountered on line " +
			std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTStatementNode* Parser::parse_block_statement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
		return parse_declaration_statement();
	case lexer::TOK_PRINT:
		return parse_print_statement();
	case lexer::TOK_READ:
		return parse_read_statement();
	case lexer::TOK_IF:
		return parse_if_statement();
	case lexer::TOK_WHILE:
		return parse_while_statement();
	case lexer::TOK_RETURN:
		return parse_return_statement();
	case lexer::TOK_IDENTIFIER:
		return parse_identifier();

	default:
		throw std::runtime_error("Invalid statement starting with '" +
			currentToken.value
			+ "' encountered on line " +
			std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTStatementNode* Parser::parse_identifier() {
	if (nextToken.value == "(") {
		return parse_function_call();
	}
	else if (nextToken.value == "=") {
		return parse_assignment_statement();
	}
	throw std::runtime_error("Expected '=' or '(' after identifier on line "
		+ std::to_string(currentToken.lineNumber) + ".");
}

ASTDeclarationNode* Parser::parse_declaration_statement() {
	// Node attributes
	TYPE type;
	std::string identifier;
	ASTExprNode* expr;
	unsigned int lineNumber;

	// Determine line number
	lineNumber = currentToken.lineNumber;

	// Consume identifier
	consume_token();
	if (currentToken.type != lexer::TOK_IDENTIFIER)
		throw std::runtime_error("Expected variable name after 'var' on line "
			+ std::to_string(currentToken.lineNumber) + ".");
	identifier = currentToken.value;

	consume_token();
	if (currentToken.type != lexer::TOK_COLON)
		throw std::runtime_error("Expected assignment operator '=' for " + identifier + " on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	consume_token();
	type = parse_type(identifier);

	consume_token();
	if (currentToken.type == lexer::TOK_EQUALS) {
		// Parse the right hand side
		expr = parse_expression();

		consume_token();
		if (currentToken.type != lexer::TOK_SEMICOLON)
			throw std::runtime_error("Expected ';' after assignment of " + identifier + " on line "
				+ std::to_string(currentToken.lineNumber) + ".");
	}
	else {
		switch (type) {
		case parser::TYPE::INT:
			expr = new ASTLiteralNode<__int64_t>(0, lineNumber);
			break;
		case parser::TYPE::FLOAT:
			expr = new ASTLiteralNode<long double>(0., lineNumber);
			break;
		case parser::TYPE::BOOL:
			expr = new ASTLiteralNode<bool>(false, lineNumber);
			break;
		case parser::TYPE::STRING:
			expr = new ASTLiteralNode<std::string>("", lineNumber);
			break;
		default:
			throw std::runtime_error("Expected type for " + identifier + " after ':' on line "
				+ std::to_string(currentToken.lineNumber) + ".");
		}
	}

	// Create ASTExpressionNode to return
	return new ASTDeclarationNode(type, identifier, expr, lineNumber);
}

ASTAssignmentNode* Parser::parse_assignment_statement() {
	// Node attributes
	std::string identifier;
	ASTExprNode* expr;

	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	identifier = currentToken.value;

	consume_token();
	if (currentToken.type != lexer::TOK_EQUALS)
		throw std::runtime_error("Expected assignment operator '=' after " + identifier + " on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Parse the right hand side
	expr = parse_expression();

	consume_token();
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after assignment of " + identifier + " on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	return new ASTAssignmentNode(identifier, expr, lineNumber);
}

ASTPrintNode* Parser::parse_print_statement() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Get expression to print
	ASTExprNode* expr = parse_expression();

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	// Consume ';' token
	consume_token();

	// Make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after print statement on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	return new ASTPrintNode(expr, lineNumber);
}

ASTReadNode* Parser::parse_read_statement() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// Left open bracket
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	// Consume ';' token
	consume_token();

	// Make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after print statement on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	return new ASTReadNode(lineNumber);
}

ASTFunctionCallNode* Parser::parse_function_call() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// If next token is not right bracket, we have parameters
	if (nextToken.type != lexer::TOK_RIGHT_BRACKET) {
		parameters = parse_actual_params();
	}
	else {
		// Consume ')'
		consume_token();
	}

	// Ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");


	// Consume ';' token
	consume_token();

	// Make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after print statement on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	return new ASTFunctionCallNode(identifier, *parameters, lineNumber);
}

ASTReturnNode* Parser::parse_return_statement() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// Get expression to return
	ASTExprNode* expr = parse_expression();

	// Consume ';' token
	consume_token();

	// Make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON)
		throw std::runtime_error("Expected ';' after return statement on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Return return node
	return new ASTReturnNode(expr, lineNumber);
}

ASTBlockNode* Parser::parse_block() {
	auto statements = new std::vector<ASTStatementNode*>;

	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// Current token is '{', consume first token of statement
	consume_token();

	// While not reached end of block or end of file
	while (currentToken.type != lexer::TOK_RIGHT_CURLY &&
		currentToken.type != lexer::TOK_ERROR &&
		currentToken.type != lexer::TOK_EOF) {
		// Parse the statement
		statements->push_back(parse_block_statement());

		// Consume first token of next statement
		consume_token();
	}

	// If block ended by '}', return block
	if (currentToken.type == lexer::TOK_RIGHT_CURLY)
		return new ASTBlockNode(*statements, lineNumber);

	// Otherwise the user left the block open
	else
		throw std::runtime_error("Reached end of file while parsing."
			" Mismatched scopes.");
}

ASTIfNode* Parser::parse_if_statement() {
	//Node attributes
	ASTExprNode* condition;
	ASTBlockNode* if_block;
	unsigned int lineNumber = currentToken.lineNumber;

	// Consume '('
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' after 'if' on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Parse the expression
	condition = parse_expression();

	// Consume ')'
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' after if-condition on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Consume '{'
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_CURLY)
		throw std::runtime_error("Expected '{' after if-condition on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Consume if-block and '}'
	if_block = parse_block();

	// Lookahead whether there is an else
	if (nextToken.type != lexer::TOK_ELSE)
		return new ASTIfNode(condition, if_block, lineNumber);

	// Otherwise, consume the else
	consume_token();

	// Consume '{' after else
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_CURLY)
		throw std::runtime_error("Expected '{' after else on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Parse else-block and '}'
	ASTBlockNode* else_block = parse_block();

	// Return if node
	return new ASTIfNode(condition, if_block, lineNumber, else_block);
}

ASTWhileNode* Parser::parse_while_statement() {
	//Node attributes
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int lineNumber = currentToken.lineNumber;

	// Consume '('
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' after 'while' on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Parse the expression
	condition = parse_expression();

	// Consume ')'
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' after while-condition on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Consume '{'
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_CURLY)
		throw std::runtime_error("Expected '{' after while-condition on line " +
			std::to_string(currentToken.lineNumber) + ".");

	// Consume while-block and '}'
	block = parse_block();

	// Return while node
	return new ASTWhileNode(condition, block, lineNumber);
}

ASTFunctionDefinitionNode* Parser::parse_function_definition() {
	// Node attributes
	std::string identifier;
	std::vector<std::pair<std::string, TYPE>> parameters;
	TYPE type;
	ASTBlockNode* block;
	unsigned int lineNumber = currentToken.lineNumber;

	// Consume identifier
	consume_token();

	// Make sure it is an identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER)
		throw std::runtime_error("Expected function identifier after keyword 'def' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	identifier = currentToken.value;

	// Consume '('
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' after '" + identifier + "' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Consume ')' or parameters
	consume_token();

	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		// Parse first parameter
		parameters.push_back(*parse_formal_param());

		// Consume ',' or ')'
		consume_token();

		while (currentToken.type == lexer::TOK_COMMA) {
			// Consume identifier
			consume_token();

			// Parse parameter
			parameters.push_back(*parse_formal_param());

			// Consume ',' or ')'
			consume_token();
		}

		// Exited while-loop, so token must be ')'
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
			throw std::runtime_error("Expected ')' or more parameters on line "
				+ std::to_string(currentToken.lineNumber) + ".");
	}

	// Consume ':' or '{'
	consume_token();

	if (currentToken.type == lexer::TOK_COLON) {
		// Consume type
		consume_token();
		type = parse_type(identifier);

		// Consume '{'
		consume_token();
	}
	else {
		type = TYPE::VOID;
	}

	if (currentToken.type != lexer::TOK_LEFT_CURLY)
		throw std::runtime_error("Expected '{' after function '" + identifier +
			"' definition on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Parse block
	block = parse_block();

	return new ASTFunctionDefinitionNode(identifier, parameters, type, block, lineNumber);
}

std::pair<std::string, TYPE>* Parser::parse_formal_param() {
	std::string identifier;
	TYPE type;

	// Make sure current token is identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER)
		throw std::runtime_error("Expected variable name in function definition on line "
			+ std::to_string(currentToken.lineNumber) + ".");
	identifier = currentToken.value;

	// Consume ':'
	consume_token();

	if (currentToken.type != lexer::TOK_COLON)
		throw std::runtime_error("Expected ':' after '" + identifier + "' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Consume type
	consume_token();
	type = parse_type(identifier);

	return new std::pair<std::string, TYPE>(identifier, type);

};

ASTExprNode* Parser::parse_expression() {
	ASTExprNode* simple_expr = parse_logical_expression();
	return parse_expression_tail(simple_expr);
}

ASTExprNode* Parser::parse_expression_tail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_LOGICAL_OR_OP) {
		consume_token();
		std::string current_token_value = currentToken.value;
		return new ASTBinaryExprNode(current_token_value, lhs, parse_expression(), lineNumber);
	}

	return lhs;
}

ASTExprNode* Parser::parse_logical_expression() {
	ASTExprNode* relational_expr = parse_relational_expression();
	return parse_logical_expression_tail(relational_expr);
}

ASTExprNode* Parser::parse_logical_expression_tail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	while (nextToken.type == lexer::TOK_LOGICAL_AND_OP) {
		consume_token();
		std::string current_token_value = currentToken.value;
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_relational_expression(), currentToken.lineNumber);
		return parse_logical_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_relational_expression() {
	ASTExprNode* relational_expr = parse_simple_expression();
	return parse_relational_expression_tail(relational_expr);
}

ASTExprNode* Parser::parse_relational_expression_tail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	while (nextToken.type == lexer::TOK_RELATIONAL_OP) {
		consume_token();
		std::string current_token_value = currentToken.value;
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_simple_expression(), currentToken.lineNumber);
		return parse_relational_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_simple_expression() {
	ASTExprNode* term = parse_term();
	return parse_simple_expression_tail(term);
}

ASTExprNode* Parser::parse_simple_expression_tail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_ADDITIVE_OP) {
		consume_token();
		std::string current_token_value = currentToken.value;
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_term(), lineNumber);
		return parse_simple_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_term() {
	ASTExprNode* factor = parse_factor();
	return parse_term_tail(factor);
}

ASTExprNode* Parser::parse_term_tail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_MULTIPLICATIVE_OP) {
		consume_token();
		std::string current_token_value = currentToken.value;
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_factor(), lineNumber);
		return parse_term_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_factor() {
	consume_token();

	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	switch (currentToken.type) {
		// Literal Cases
	case lexer::TOK_INT:
		return new ASTLiteralNode<__int64_t>(std::stoll(currentToken.value), lineNumber);

	case lexer::TOK_FLOAT:
		return new ASTLiteralNode<long double>(std::stold(currentToken.value), lineNumber);

	case lexer::TOK_BOOL:
		return new ASTLiteralNode<bool>(currentToken.value == "true", lineNumber);

	case lexer::TOK_STRING: {
		// Remove " character from front and end of lexeme
		std::string str = currentToken.value.substr(1, currentToken.value.size() - 2);

		// Replace \" with quote
		size_t pos = str.find("\\\"");
		while (pos != std::string::npos) {
			// Replace
			str.replace(pos, 2, "\"");
			// Get next occurrence from current position
			pos = str.find("\\\"", pos + 2);
		}

		// Replace \n with newline
		pos = str.find("\\n");
		while (pos != std::string::npos) {
			// Replace
			str.replace(pos, 2, "\n");
			// Get next occurrence from current position
			pos = str.find("\\n", pos + 2);
		}

		// Replace \t with tab
		pos = str.find("\\t");
		while (pos != std::string::npos) {
			// Replace
			str.replace(pos, 2, "\t");
			// Get next occurrence from current position
			pos = str.find("\\t", pos + 2);
		}

		// Replace \b with backslash
		pos = str.find("\\b");
		while (pos != std::string::npos) {
			// Replace
			str.replace(pos, 2, "\\");
			// Get next occurrence from current position
			pos = str.find("\\b", pos + 2);
		}

		return new ASTLiteralNode<std::string>(std::move(str), lineNumber);
	}

	case lexer::TOK_FLOAT_TYPE:
		return parse_float_parse_exp();

	case lexer::TOK_INT_TYPE:
		return parse_int_parse_exp();

	case lexer::TOK_STRING_TYPE:
		return parse_string_parse_exp();

	case lexer::TOK_IDENTIFIER: // Identifier or function call case
		if (nextToken.type == lexer::TOK_LEFT_BRACKET)
			return parse_expr_function_call();
		else return new ASTIdentifierNode(currentToken.value, lineNumber);

	case lexer::TOK_READ: // Read case
		return parse_expr_read_node();

	case lexer::TOK_LEFT_BRACKET: // Subexpression case
	{
		ASTExprNode* sub_expr = parse_expression();
		consume_token();
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
			throw std::runtime_error("Expected ')' after expression on line "
				+ std::to_string(currentToken.lineNumber) + ".");
		return sub_expr;
	}

	case lexer::TOK_ADDITIVE_OP: // Unary expression case
	case lexer::TOK_NOT: {
		std::string current_token_value = currentToken.value;
		return new ASTUnaryExprNode(current_token_value, parse_expression(), lineNumber);
	}

	default:
		throw std::runtime_error("Expected expression on line "
			+ std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTExprFunctionCallNode* Parser::parse_expr_function_call() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// If next token is not right bracket, we have parameters
	if (nextToken.type != lexer::TOK_RIGHT_BRACKET) {
		parameters = parse_actual_params();
	}
	else {
		// Consume ')'
		consume_token();
	}

	// Ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	return new ASTExprFunctionCallNode(identifier, *parameters, lineNumber);
}

ASTFloatParseNode* Parser::parse_float_parse_exp() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Get expression to print
	ASTExprNode* expr = parse_expression();

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	return new ASTFloatParseNode(expr, lineNumber);
}

ASTIntParseNode* Parser::parse_int_parse_exp() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Get expression to print
	ASTExprNode* expr = parse_expression();

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	return new ASTIntParseNode(expr, lineNumber);
}

ASTStringParseNode* Parser::parse_string_parse_exp() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Get expression to print
	ASTExprNode* expr = parse_expression();

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	return new ASTStringParseNode(expr, lineNumber);
}

ASTExprReadNode* Parser::parse_expr_read_node() {
	// Determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// Left open bracket
	consume_token();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET)
		throw std::runtime_error("Expected '(' on line "
			+ std::to_string(currentToken.lineNumber) + ".");

	// Ensure right close bracket after fetching parameters
	consume_token();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET)
		throw std::runtime_error("Expected ')' on line "
			+ std::to_string(currentToken.lineNumber)
			+ " after function parameters.");

	return new ASTExprReadNode(lineNumber);
}

std::vector<ASTExprNode*>* Parser::parse_actual_params() {
	auto parameters = new std::vector<ASTExprNode*>;

	parameters->push_back(parse_expression());
	consume_token();

	// If there are more
	while (currentToken.type == lexer::TOK_COMMA) {
		parameters->push_back(parse_expression());
		consume_token();
	}

	return parameters;
}

TYPE Parser::parse_type(std::string& identifier) {
	switch (currentToken.type) {
	case lexer::TOK_VOID_TYPE:
		return TYPE::VOID;

	case lexer::TOK_INT_TYPE:
		return TYPE::INT;

	case lexer::TOK_FLOAT_TYPE:
		return TYPE::FLOAT;

	case lexer::TOK_BOOL_TYPE:
		return TYPE::BOOL;

	case lexer::TOK_STRING_TYPE:
		return TYPE::STRING;

	default:
		throw std::runtime_error("Expected type for " + identifier + " after ':' on line "
			+ std::to_string(currentToken.lineNumber) + ".");
	}
}
