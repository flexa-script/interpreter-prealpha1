#include <iostream>

#include "parser.hpp"
#include "util.hpp"


using namespace parser;


Parser::Parser(lexer::Lexer* lex, std::string name) : lex(lex), name(name) {
	current_token = lex->next_token();
	next_token = lex->next_token();
}

Parser::Parser(lexer::Lexer* lex, std::string name, unsigned int tokens) : lex(lex), name(name) {
	next_token = lex->next_token();

	for (unsigned int i = 0; i < tokens; i++) {
		consume_token();
	}
}

void Parser::consume_token() {
	current_token = next_token;
	next_token = lex->next_token();
}

void Parser::consume_token(lexer::TokenType type) {
	consume_token();
	check_current_token(type);
}

void Parser::check_current_token(lexer::TokenType type) {
	if (current_token.type != type) {
		throw std::runtime_error(msg_header() + "expected '" + lexer::Token::token_image(type) + "'");
	}
}

ASTProgramNode* Parser::parse_program() {
	auto statements = new std::vector<ASTNode*>;

	while (current_token.type == lexer::TOK_USING && current_token.type != lexer::TOK_EOF) {
		statements->push_back(parse_using_statement());
		consume_token();
	}

	while (current_token.type != lexer::TOK_EOF) {
		statements->push_back(parse_program_statement());
		consume_token();
	}

	return new ASTProgramNode(*statements, name);
}

ASTUsingNode* Parser::parse_using_statement() {
	// node attributes
	std::vector<std::string> library = std::vector<std::string>();
	std::string alias = "";
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume library identifier
	do {
		consume_token(lexer::TOK_IDENTIFIER);
		library.push_back(current_token.value);
		if (next_token.type == lexer::TOK_DOT) {
			consume_token();
		}
	} while (next_token.type == lexer::TOK_IDENTIFIER);

	if (next_token.type == lexer::TOK_AS) {
		consume_token();
		consume_token();

		alias = current_token.value;
	}

	consume_token(lexer::TOK_SEMICOLON);

	return new ASTUsingNode(library, alias, row, col);
}

ASTNode* Parser::parse_program_statement() {
	switch (current_token.type) {
	case lexer::TOK_DEF:
		return parse_function_definition();
	default:
		consume_semicolon = true;
		parse_block_statement();
	}
}

ASTNode* Parser::parse_block_statement() {
	switch (current_token.type) {
	case lexer::TOK_VAR:
	case lexer::TOK_CONST:
		return parse_declaration_statement();
	case lexer::TOK_STRUCT:
		return parse_struct_definition();
	case lexer::TOK_PRINT:
		return parse_print_statement();
	case lexer::TOK_SWITCH:
		return parse_switch_statement();
	case lexer::TOK_IF:
		return parse_if_statement();
	case lexer::TOK_WHILE:
		return parse_while_statement();
	case lexer::TOK_FOR:
		return parse_for_statement();
	case lexer::TOK_FOREACH:
		return parse_foreach_statement();
	case lexer::TOK_CONTINUE:
		return parse_continue_statement();
	case lexer::TOK_BREAK:
		return parse_break_statement();
	case lexer::TOK_RETURN:
		return parse_return_statement();
	case lexer::TOK_IDENTIFIER:
		return parse_identifier_statement();
	default:
		parse_statement_expression();
	}
}

ASTExprNode* Parser::parse_statement_expression() {
	ASTExprNode* expr = parse_expression();
	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}
	return expr;
}

ASTNode* Parser::parse_identifier_statement() {
	if (next_token.value == "(") {
		ASTFunctionCallNode* expr = parse_function_call_node();
		if (consume_semicolon) {
			consume_semicolon = false;
			consume_token(lexer::TOK_SEMICOLON);
		}
		return expr;
	}
	else if (next_token.value == "[" || next_token.value == "=" || next_token.value == "+="
		|| next_token.value == "-=" || next_token.value == "*=" || next_token.value == "/="
		|| next_token.value == "%=" || next_token.value == ".") {
		return parse_assignment_statement();
	}
	else {
		return parse_statement_expression();
	}
}

ASTDeclarationNode* Parser::parse_declaration_statement() {
	Type type = Type::T_UNDEF;
	current_array_type = Type::T_UNDEF;
	std::string identifier;
	std::string type_name = "";
	ASTExprNode* expr;
	auto dim_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	bool is_const = current_token.type == lexer::TOK_CONST;

	consume_token(lexer::TOK_IDENTIFIER);
	auto id = parse_identifier();
	identifier = id.identifier;
	dim_vector = id.access_vector;

	if (dim_vector.size() > 0) {
		type = Type::T_ARRAY;
	}

	if (next_token.type == lexer::TOK_COLON) {
		consume_token();
		consume_token();
		if (type == Type::T_UNDEF) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEF || current_array_type == parser::Type::T_VOID || current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == lexer::TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	if (next_token.type == lexer::TOK_EQUALS) {
		consume_token();
		consume_token();
		expr = parse_expression();
	}
	else {
		expr = nullptr;
	}

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	if (type == Type::T_UNDEF) {
		type = Type::T_ANY;
	}

	return new ASTDeclarationNode(type, type_name, identifier, expr, is_const, current_array_type, dim_vector, row, col);
}

ASTAssignmentNode* Parser::parse_assignment_statement() {
	std::string identifier = std::string();
	std::string op = std::string();
	ASTExprNode* expr = nullptr;
	ASTExprNode* expr_size = nullptr;
	auto identifier_vector = std::vector<Identifier_t>();
	auto access_vector = std::vector<ASTExprNode*>();

	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	identifier_vector = parse_identifier_vector();

	consume_token();


	if (current_token.type != lexer::TOK_ADDITIVE_OP && current_token.type != lexer::TOK_MULTIPLICATIVE_OP && current_token.type != lexer::TOK_EQUALS) {
		throw std::runtime_error(msg_header() + "invalid assignment operator '" + current_token.value + "'");
	}

	op = current_token.value;

	// parse the right hand side
	consume_token();
	expr = parse_expression();

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	return new ASTAssignmentNode(identifier_vector, op, expr, row, col);
}

ASTPrintNode* Parser::parse_print_statement() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(lexer::TOK_LEFT_BRACKET);

	// get expression to print
	consume_token();
	ASTExprNode* expr = parse_expression();

	// ensure right close bracket after fetching parameters
	consume_token(lexer::TOK_RIGHT_BRACKET);

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	return new ASTPrintNode(expr, row, col);
}

ASTReturnNode* Parser::parse_return_statement() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// get expression to return
	consume_token();
	ASTExprNode* expr = parse_expression();

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	// return return node
	return new ASTReturnNode(expr, row, col);
}

ASTBlockNode* Parser::parse_block() {
	auto statements = new std::vector<ASTNode*>();

	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// current token is '{', consume first token of statement
	consume_token();

	// while not reached end of block or end of file
	while (current_token.type != lexer::TOK_RIGHT_CURLY && current_token.type != lexer::TOK_ERROR && current_token.type != lexer::TOK_EOF) {
		consume_semicolon = true;
		// parse the statement
		statements->push_back(parse_block_statement());

		// consume first token of next statement
		consume_token();
	}

	// if block ended by '}', return block
	if (current_token.type == lexer::TOK_RIGHT_CURLY) {
		return new ASTBlockNode(*statements, row, col);
	}
	// otherwise the user left the block open
	throw std::runtime_error(msg_header() + "reached end of file while parsing");
}

ASTBlockNode* Parser::parse_struct_block() {
	auto statements = new std::vector<ASTNode*>;

	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// current token is '{', consume first token of statement
	consume_token();

	// while not reached end of block or end of file
	while (current_token.type != lexer::TOK_RIGHT_CURLY && current_token.type != lexer::TOK_ERROR && current_token.type != lexer::TOK_EOF) {
		// parse the statement
		statements->push_back(parse_struct_block_variables());

		// consume first token of next statement
		consume_token();
	}

	// if block ended by '}', return block
	if (current_token.type == lexer::TOK_RIGHT_CURLY) {
		return new ASTBlockNode(*statements, row, col);
	}
	// otherwise the user left the block open
	throw std::runtime_error(msg_header() + "mismatched scopes: reached end of file while parsing");
}

ASTStatementNode* Parser::parse_struct_block_variables() {
	switch (current_token.type) {
	case lexer::TOK_VAR:
		return parse_declaration_statement();

	default:
		throw std::runtime_error(msg_header() + "invalid declaration starting with '" + current_token.value + "' encountered");
	}
}

ASTContinueNode* Parser::parse_continue_statement() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	// return return node
	return new ASTContinueNode(row, col);
}

ASTBreakNode* Parser::parse_break_statement() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	// return return node
	return new ASTBreakNode(row, col);
}

ASTSwitchNode* Parser::parse_switch_statement() {
	// node attributes
	ASTExprNode* condition;
	std::map<ASTExprNode*, unsigned int>* case_blocks = new std::map<ASTExprNode*, unsigned int>();
	long default_block = -1;
	std::vector<ASTNode*>* statements = new std::vector<ASTNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// parse the expression
	consume_token();
	condition = parse_expression();

	// consume ')'
	consume_token(lexer::TOK_RIGHT_BRACKET);

	// consume '{'
	consume_token(lexer::TOK_LEFT_CURLY);

	// consume case/default
	consume_token();

	while (current_token.type == lexer::TOK_CASE) {
		bool is_block = false;
		consume_token();
		ASTExprNode* case_exrp = parse_expression();
		int start_position = statements->size();

		// consume :
		consume_token();

		// consume first token of next statement
		consume_token();

		if (current_token.type == lexer::TOK_LEFT_CURLY) {
			statements->push_back(parse_block());

			// consume first token of next statement
			consume_token();
		}
		else {
			// while not reached end of block or end of file
			while (current_token.type != lexer::TOK_CASE && current_token.type != lexer::TOK_DEFAULT && current_token.type != lexer::TOK_RIGHT_CURLY && current_token.type != lexer::TOK_ERROR && current_token.type != lexer::TOK_EOF) {
				consume_semicolon = true;
				// parse the statement
				statements->push_back(parse_block_statement());

				// consume first token of next statement
				consume_token();
			}
		}

		case_blocks->emplace(case_exrp, start_position);
	}

	if (current_token.type == lexer::TOK_DEFAULT) {
		bool is_block = false;
		default_block = statements->size();

		// consume :
		consume_token();

		// consume first token of next statement
		consume_token();

		if (current_token.type == lexer::TOK_LEFT_CURLY) {
			statements->push_back(parse_block());

			// consume first token of next statement
			consume_token();
		}
		else {
			// while not reached end of block or end of file
			while (current_token.type != lexer::TOK_RIGHT_CURLY && current_token.type != lexer::TOK_ERROR && current_token.type != lexer::TOK_EOF) {
				consume_semicolon = true;
				// parse the statement
				statements->push_back(parse_block_statement());

				// consume first token of next statement
				consume_token();
			}
		}
	}

	return new ASTSwitchNode(condition, statements, case_blocks, default_block, row, col);
}

ASTElseIfNode* Parser::parse_else_if_statement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* if_block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// parse the expression
	consume_token();
	condition = parse_expression();

	// consume ')'
	consume_token(lexer::TOK_RIGHT_BRACKET);

	// consume '{'
	consume_token(lexer::TOK_LEFT_CURLY);

	// consume if-block and '}'
	if_block = parse_block();

	//consume_token();

	// return elif node
	return new ASTElseIfNode(condition, if_block, row, col);
}

ASTIfNode* Parser::parse_if_statement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* if_block;
	std::vector<ASTElseIfNode*> else_ifs = std::vector<ASTElseIfNode*>();
	ASTBlockNode* else_block = nullptr;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// parse the expression
	consume_token();
	condition = parse_expression();

	// consume ')'
	consume_token(lexer::TOK_RIGHT_BRACKET);
	
	// consume '{'
	consume_token(lexer::TOK_LEFT_CURLY);

	// consume if-block and '}'
	if_block = parse_block();

	if (next_token.type == lexer::TOK_ELSE) {
		// consume the if or else
		consume_token();

		if (next_token.type == lexer::TOK_IF) {
			do {
				consume_token(lexer::TOK_IF);
				else_ifs.push_back(parse_else_if_statement());
				if (next_token.type == lexer::TOK_ELSE) {
					consume_token(lexer::TOK_ELSE);
				}
			} while (next_token.type == lexer::TOK_IF);
		}

		if (current_token.type == lexer::TOK_ELSE) {
			// consume '{' after else
			consume_token(lexer::TOK_LEFT_CURLY);

			// parse else-block and '}'
			else_block = parse_block();
		}
	}

	// return if node
	return new ASTIfNode(condition, if_block, else_ifs, else_block, row, col);
}

ASTForNode* Parser::parse_for_statement() {
	// node attributes
	std::array<ASTNode*, 3> dci = { nullptr };
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	for (int i = 0; i < 3; ++i) {
		if (next_token.type != lexer::TOK_SEMICOLON && next_token.type != lexer::TOK_RIGHT_BRACKET) {
			consume_token();
			consume_semicolon = i < 2;
			dci[i] = parse_block_statement();

		}
		else {
			if (i < 2) {
				consume_token();
			}
		}
	}

	consume_token(lexer::TOK_RIGHT_BRACKET);
	consume_token(lexer::TOK_LEFT_CURLY);

	block = parse_block();

	// return for node
	return new ASTForNode(dci, block, row, col);
}

ASTNode* Parser::parse_foreach_collection() {
	consume_semicolon = false;

	switch (current_token.type) {
	case lexer::TOK_LEFT_CURLY:
		return parse_array_constructor_node();
	case lexer::TOK_IDENTIFIER:
		return parse_identifier_statement();
	default:
		throw std::runtime_error(msg_header() + "expected array");
	}
}

ASTForEachNode* Parser::parse_foreach_statement() {
	// node attributes
	ASTNode* itdecl;
	ASTNode* collection;
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// consume decl first token
	consume_token();

	itdecl = parse_declaration_statement();

	check_current_token(lexer::TOK_IN);

	// consume expression first token
	consume_token();

	collection = parse_foreach_collection();

	// consume ')'
	consume_token(lexer::TOK_RIGHT_BRACKET);

	// consume '{'
	consume_token(lexer::TOK_LEFT_CURLY);

	// consume block and '}'
	block = parse_block();

	// return for node
	return new ASTForEachNode(itdecl, collection, block, row, col);
}

ASTWhileNode* Parser::parse_while_statement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// parse the expression
	consume_token();
	condition = parse_expression();

	// consume ')'
	consume_token(lexer::TOK_RIGHT_BRACKET);

	// consume '{'
	consume_token(lexer::TOK_LEFT_CURLY);

	// consume while-block and '}'
	block = parse_block();

	// return while node
	return new ASTWhileNode(condition, block, row, col);
}

ASTFunctionDefinitionNode* Parser::parse_function_definition() {
	// node attributes
	std::string identifier;
	std::vector<VariableDefinition_t> parameters;
	Type type;
	Type array_type = parser::Type::T_UNDEF;
	std::string type_name = "";
	ASTBlockNode* block;
	ASTExprNode* expr;
	ASTExprNode* expr_size;
	auto dim_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume identifier
	consume_token(lexer::TOK_IDENTIFIER);

	identifier = current_token.value;

	// consume '('
	consume_token(lexer::TOK_LEFT_BRACKET);

	// consume ')' or parameters
	consume_token();

	if (current_token.type != lexer::TOK_RIGHT_BRACKET) {
		// parse first parameter
		parameters.push_back(*parse_formal_param());

		// consume ',' or ')'
		consume_token();

		while (current_token.type == lexer::TOK_COMMA) {
			// consume identifier
			consume_token();

			// parse parameter
			parameters.push_back(*parse_formal_param());

			// consume ',' or ')'
			consume_token();
		}

		check_current_token(lexer::TOK_RIGHT_BRACKET);
	}

	// consume ':' or '{'
	consume_token();

	if (current_token.type == lexer::TOK_COLON) {
		// consume type
		consume_token();

		if (current_token.type == lexer::TOK_IDENTIFIER) {
			type_name = current_token.value;
		}

		type = parse_type();

		// consume '{'
		consume_token();

		dim_vector = parse_dimension_vector();
		if (dim_vector.size() > 0) {
			type = Type::T_ARRAY;
			consume_token();
		}
	}
	else {
		type = Type::T_VOID;
	}

	check_current_token(lexer::TOK_LEFT_CURLY);

	// parse block
	block = parse_block();

	return new ASTFunctionDefinitionNode(identifier, parameters, type, type_name, array_type, dim_vector, block, row, col);
}

ASTStructDefinitionNode* Parser::parse_struct_definition() {
	// node attributes
	std::string identifier;
	std::vector<VariableDefinition_t> variables;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// consume identifier
	consume_token(lexer::TOK_IDENTIFIER);

	identifier = current_token.value;

	// consume {
	consume_token(lexer::TOK_LEFT_CURLY);

	do {
		// consume var
		consume_token(lexer::TOK_VAR);
		// consume identifier
		consume_token(lexer::TOK_IDENTIFIER);

		// parse parameter
		variables.push_back(*parse_formal_param());

		// consume ';'
		consume_token();

	} while (current_token.type == lexer::TOK_SEMICOLON && next_token.type != lexer::TOK_RIGHT_CURLY);

	// consume }
	consume_token(lexer::TOK_RIGHT_CURLY);

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(lexer::TOK_SEMICOLON);
	}

	return new ASTStructDefinitionNode(identifier, variables, row, col);
}

VariableDefinition_t* Parser::parse_formal_param() {
	std::string identifier;
	std::string type_name;
	Type type = Type::T_UNDEF;
	ASTExprNode* expr_size;
	auto access_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// make sure current token is identifier
	check_current_token(lexer::TOK_IDENTIFIER);

	auto id = parse_identifier();
	identifier = id.identifier;
	access_vector = id.access_vector;

	if (access_vector.size() > 0) {
		type = Type::T_ARRAY;
	}

	// consume type
	if (next_token.type == lexer::TOK_COLON) {
		consume_token();
		consume_token();
		if (type == Type::T_UNDEF) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEF || current_array_type == parser::Type::T_VOID || current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == lexer::TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	if (type == Type::T_UNDEF) {
		type = Type::T_ANY;
	}

	return new VariableDefinition_t(identifier, type, type_name, type, current_array_type, access_vector, row, col);
};

ASTExprNode* Parser::parse_expression() {
	ASTExprNode* simple_expression = parse_logical_expression();
	return parse_expression_tail(simple_expression);
}

ASTExprNode* Parser::parse_expression_tail(ASTExprNode* lhs) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (next_token.type == lexer::TOK_LOGICAL_OR_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		return new ASTBinaryExprNode(current_token_value, lhs, parse_expression(), row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_logical_expression() {
	ASTExprNode* relational_expression = parse_relational_expression();
	return parse_logical_expression_tail(relational_expression);
}

ASTExprNode* Parser::parse_logical_expression_tail(ASTExprNode* lhs) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == lexer::TOK_LOGICAL_AND_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_relational_expression(), row, col);
		return parse_logical_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_relational_expression() {
	ASTExprNode* relational_expr = parse_simple_expression();
	return parse_relational_expression_tail(relational_expr);
}

ASTExprNode* Parser::parse_relational_expression_tail(ASTExprNode* lhs) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == lexer::TOK_RELATIONAL_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_simple_expression(), row, col);
		return parse_relational_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_simple_expression() {
	ASTExprNode* term = parse_term();
	return parse_simple_expression_tail(term);
}

ASTExprNode* Parser::parse_simple_expression_tail(ASTExprNode* lhs) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (next_token.type == lexer::TOK_ADDITIVE_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_term(), row, col);
		return parse_simple_expression_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_term() {
	ASTExprNode* factor = parse_factor();
	return parse_term_tail(factor);
}

ASTExprNode* Parser::parse_term_tail(ASTExprNode* lhs) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (next_token.type == lexer::TOK_MULTIPLICATIVE_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		ASTExprNode* bin_expr = new ASTBinaryExprNode(current_token_value, lhs, parse_factor(), row, col);
		return parse_term_tail(bin_expr);
	}

	return lhs;
}

ASTExprNode* Parser::parse_factor() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	switch (current_token.type) {
		// literal cases
	case lexer::TOK_BOOL_LITERAL:
		return new ASTLiteralNode<cp_bool>(parse_bool_literal(), row, col);

	case lexer::TOK_INT_LITERAL:
		return new ASTLiteralNode<cp_int>(parse_int_literal(), row, col);

	case lexer::TOK_FLOAT_LITERAL:
		return new ASTLiteralNode<cp_float>(parse_float_literal(), row, col);

	case lexer::TOK_CHAR_LITERAL:
		return new ASTLiteralNode<cp_char>(parse_char_literal(), row, col);

	case lexer::TOK_STRING_LITERAL:
		return new ASTLiteralNode<cp_string>(parse_string_literal(), row, col);

	case lexer::TOK_LEFT_CURLY:
		return parse_array_constructor_node();

	case lexer::TOK_BOOL_TYPE:
	case lexer::TOK_INT_TYPE:
	case lexer::TOK_FLOAT_TYPE:
	case lexer::TOK_CHAR_TYPE:
	case lexer::TOK_STRING_TYPE:
		return parse_type_parse_node();

	case lexer::TOK_TYPE:
		return parse_type_node();

	case lexer::TOK_LEN:
		return parse_len_node();

	case lexer::TOK_ROUND:
		return parse_round_node();

	case lexer::TOK_NULL:
		return new ASTNullNode(row, col);

	case lexer::TOK_THIS:
		return parse_this_node();

	case lexer::TOK_IDENTIFIER: // identifier or function call case
		switch (next_token.type)
		{
		case lexer::TOK_LEFT_BRACKET:
			return parse_function_call_node();
		case lexer::TOK_LEFT_CURLY:
			return parse_struct_constructor_node();
		case lexer::TOK_ADDITIVE_UN_OP: { // unary expression case
			auto id = parse_identifier_node();
			consume_token();
			std::string current_token_value = current_token.value;
			return new ASTUnaryExprNode(current_token_value, id, row, col);
		}
		default:
			return parse_identifier_node();
		}

	case lexer::TOK_READ: // read case
		return parse_read_node();

	case lexer::TOK_LEFT_BRACKET: { // subexpression case
		consume_token();
		ASTExprNode* sub_expr = parse_expression();
		consume_token(lexer::TOK_RIGHT_BRACKET);
		return sub_expr;
	}

	case lexer::TOK_ADDITIVE_OP: // unary expression case
	case lexer::TOK_NOT: {
		std::string current_token_value = current_token.value;
		consume_token();
		return new ASTUnaryExprNode(current_token_value, parse_expression(), row, col);
	}

	default:
		throw std::runtime_error(msg_header() + "expected expression");
	}
}

ASTTypeNode* Parser::parse_type_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	ASTExprNode* expr = nullptr;

	consume_token(lexer::TOK_LEFT_BRACKET);

	consume_token();
	expr = parse_expression();
	consume_token(lexer::TOK_RIGHT_BRACKET);

	return new ASTTypeNode(expr, row, col);
}

ASTLenNode* Parser::parse_len_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	ASTExprNode* expr = nullptr;

	consume_token(lexer::TOK_LEFT_BRACKET);

	consume_token();
	expr = parse_expression();
	consume_token(lexer::TOK_RIGHT_BRACKET);

	return new ASTLenNode(expr, row, col);
}

ASTRoundNode* Parser::parse_round_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	unsigned int ndigits = 0;
	ASTExprNode* expr = nullptr;

	consume_token(lexer::TOK_LEFT_BRACKET);

	consume_token();
	expr = parse_expression();
	consume_token(lexer::TOK_RIGHT_BRACKET);

	return new ASTRoundNode(expr, ndigits, row, col);
}

ASTIdentifierNode* Parser::parse_identifier_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	auto identifier_vector = std::vector<Identifier_t>();
	auto access_vector = std::vector<ASTExprNode*>();

	identifier_vector = parse_identifier_vector();

	return new ASTIdentifierNode(identifier_vector, row, col);
}

ASTArrayConstructorNode* Parser::parse_array_constructor_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	std::vector<ASTExprNode*> values = std::vector<ASTExprNode*>();

	do {
		consume_token();
		values.push_back(parse_expression());

		consume_token();

	} while (next_token.type == lexer::TOK_LEFT_CURLY || next_token.type == lexer::TOK_NULL
		|| next_token.type == lexer::TOK_BOOL_LITERAL || next_token.type == lexer::TOK_INT_LITERAL
		|| next_token.type == lexer::TOK_IDENTIFIER || next_token.type == lexer::TOK_FLOAT_LITERAL
		|| next_token.type == lexer::TOK_CHAR_LITERAL || next_token.type == lexer::TOK_STRING_LITERAL);

	check_current_token(lexer::TOK_RIGHT_CURLY);

	return new ASTArrayConstructorNode(values, row, col);
}

ASTStructConstructorNode* Parser::parse_struct_constructor_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	std::map<std::string, ASTExprNode*> values = std::map<std::string, ASTExprNode*>();
	std::string type_name = current_token.value;

	consume_token();
	consume_token();

	while (current_token.type == lexer::TOK_IDENTIFIER) {
		auto var_identifier = current_token.value;

		consume_token(lexer::TOK_EQUALS);

		consume_token();
		values[var_identifier] = parse_expression();

		consume_token();

		if (next_token.type == lexer::TOK_IDENTIFIER) {
			consume_token();
		}
	}

	check_current_token(lexer::TOK_RIGHT_CURLY);

	return new ASTStructConstructorNode(type_name, values, row, col);
}

cp_bool Parser::parse_bool_literal() {
	return current_token.value == "true";
}

cp_int Parser::parse_int_literal() {
	return std::stoll(current_token.value);
}

cp_float Parser::parse_float_literal() {
	return std::stold(current_token.value);
}

cp_char Parser::parse_char_literal() {
	char chr = 0;
	if (current_token.value == "'\\\\'") {
		chr = '\\';
	}
	else if (current_token.value == "'\\n'") {
		chr = '\n';
	}
	else if (current_token.value == "'\\r'") {
		chr = '\r';
	}
	else if (current_token.value == "'\\''") {
		chr = '\'';
	}
	else if (current_token.value == "'\\t'") {
		chr = '\t';
	}
	else if (current_token.value == "'\\b'") {
		chr = '\b';
	}
	else if (current_token.value == "'\\0'") {
		chr = '\0';
	}
	else {
		chr = current_token.value.c_str()[1];
	}
	return chr;
}

cp_string Parser::parse_string_literal() {
	std::string str = current_token.value.substr(1, current_token.value.size() - 2);

	size_t pos = 0;
	while (pos < str.size()) {
		auto sc = str.substr(pos, 2);

		if (sc == "\\\"") {
			str.replace(pos, 2, "\"");
			++pos;
			continue;
		}

		if (sc == "\\n") {
			str.replace(pos, 2, "\n");
			++pos;
			continue;
		}

		if (sc == "\\r") {
			str.replace(pos, 2, "\r");
			++pos;
			continue;
		}

		if (sc == "\\t") {
			str.replace(pos, 2, "\t");
			++pos;
			continue;
		}

		if (sc == "\\b") {
			str.replace(pos, 2, "\b");
			++pos;
			continue;
		}

		if (sc == "\\0") {
			str.replace(pos, 2, "\0");
			++pos;
			continue;
		}

		if (sc == "\\\\") {
			str.replace(pos, 2, "\\");
			++pos;
			continue;
		}

		++pos;
	}

	return str;
}

ASTFunctionCallNode* Parser::parse_function_call_node() {
	// current token is the function identifier
	std::string identifier = current_token.value;
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(lexer::TOK_LEFT_BRACKET);

	// if next token is not right bracket, we have parameters
	if (next_token.type != lexer::TOK_RIGHT_BRACKET) {
		parameters = parse_actual_params();
	}
	else {
		// consume ')'
		consume_token();
	}

	// ensure right close bracket after fetching parameters
	check_current_token(lexer::TOK_RIGHT_BRACKET);

	if (next_token.type == lexer::TOK_LEFT_BRACE) {
		consume_token();
		access_vector = parse_dimension_vector();
	}

	return new ASTFunctionCallNode(identifier, access_vector, *parameters, row, col);
}

ASTTypeParseNode* Parser::parse_type_parse_node() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	Type type = parse_type();

	consume_token(lexer::TOK_LEFT_BRACKET);

	// get expression to print
	consume_token();
	ASTExprNode* expr = parse_expression();

	// ensure right close bracket after fetching parameters
	consume_token(lexer::TOK_RIGHT_BRACKET);

	return new ASTTypeParseNode(type, expr, row, col);
}

ASTThisNode* Parser::parse_this_node() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	return new ASTThisNode(row, col);
}

ASTReadNode* Parser::parse_read_node() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	// left open bracket
	consume_token(lexer::TOK_LEFT_BRACKET);

	// ensure right close bracket after fetching parameters
	consume_token(lexer::TOK_RIGHT_BRACKET);

	return new ASTReadNode(row, col);
}

std::vector<ASTExprNode*>* Parser::parse_actual_params() {
	auto parameters = new std::vector<ASTExprNode*>;

	consume_token();
	parameters->push_back(parse_expression());
	consume_token();

	// if there are more
	while (current_token.type == lexer::TOK_COMMA) {
		consume_token();
		parameters->push_back(parse_expression());
		consume_token();
	}

	return parameters;
}

std::vector<ASTExprNode*> Parser::parse_dimension_vector() {
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();

	if (current_token.type == lexer::TOK_LEFT_BRACE) {
		do {
			ASTExprNode* expr_size = nullptr;
			consume_token();
			if (current_token.type != lexer::TOK_RIGHT_BRACE) {
				expr_size = parse_expression();
				consume_token(lexer::TOK_RIGHT_BRACE);
			}
			access_vector.push_back(expr_size);

			if (next_token.type == lexer::TOK_LEFT_BRACE) {
				consume_token();
			}

		} while (current_token.type == lexer::TOK_LEFT_BRACE);
	}

	return access_vector;
}

Identifier_t Parser::parse_identifier() {
	std::string identifier = "";
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();

	identifier = current_token.value;

	if (next_token.type == lexer::TOK_LEFT_BRACE) {
		consume_token();
		access_vector = parse_dimension_vector();
	}

	return Identifier_t(identifier, access_vector);
}

std::vector<Identifier_t> Parser::parse_identifier_vector() {
	auto identifier_vector = std::vector<Identifier_t>();

	while (current_token.type == lexer::TOK_IDENTIFIER) {
		identifier_vector.push_back(parse_identifier());

		if (next_token.type == lexer::TOK_DOT) {
			consume_token();
			consume_token(lexer::TOK_IDENTIFIER);
		}
		else {
			break;
		}
	}

	return identifier_vector;
}

std::string Parser::msg_header() {
	return "(PERR) " + name + '[' + std::to_string(current_token.row) + ':' + std::to_string(current_token.col) + "]: ";
}

Type Parser::parse_type() {
	switch (current_token.type) {
	case lexer::TOK_VOID_TYPE:
	case lexer::TOK_NULL:
		return Type::T_VOID;

	case lexer::TOK_BOOL_TYPE:
	case lexer::TOK_BOOL_LITERAL:
		return Type::T_BOOL;

	case lexer::TOK_INT_TYPE:
	case lexer::TOK_INT_LITERAL:
		return Type::T_INT;

	case lexer::TOK_FLOAT_TYPE:
	case lexer::TOK_FLOAT_LITERAL:
		return Type::T_FLOAT;

	case lexer::TOK_CHAR_TYPE:
	case lexer::TOK_CHAR_LITERAL:
		return Type::T_CHAR;

	case lexer::TOK_STRING_TYPE:
	case lexer::TOK_STRING_LITERAL:
		return Type::T_STRING;

	case lexer::TOK_ANY_TYPE:
		return Type::T_ANY;

	case lexer::TOK_IDENTIFIER:
		return Type::T_STRUCT;

	case lexer::TOK_LEFT_CURLY:
		return Type::T_ARRAY;

	default:
		throw std::runtime_error(msg_header() + "invalid type");
	}
}
