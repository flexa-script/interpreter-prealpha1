#include <iostream>

#include "parser.hpp"
#include "vendor/util.hpp"


using namespace lexer;
using namespace parser;


Parser::Parser(std::string name, Lexer* lex) : name(name), lex(lex) {
	current_token = lex->next_token();
	next_token = lex->next_token();
}

Parser::Parser(std::string name, Lexer* lex, unsigned int tokens) : name(name), lex(lex) {
	next_token = lex->next_token();

	for (unsigned int i = 0; i < tokens; i++) {
		consume_token();
	}
}

void Parser::consume_token() {
	current_token = next_token;
	next_token = lex->next_token();
}

void Parser::consume_token(TokenType type) {
	consume_token();
	check_current_token(type);
}

void Parser::check_current_token(TokenType type) {
	if (current_token.type != type) {
		throw std::runtime_error(msg_header() + "expected '" + Token::token_image(type) + "'");
	}
}

ASTProgramNode* Parser::parse_program() {
	auto statements = new std::vector<ASTNode*>;
	std::string alias = "";

	if (current_token.type == TOK_NAMESPACE) {
		consume_token(TOK_IDENTIFIER);
		alias = current_token.value;
		consume_token(TOK_SEMICOLON);
		consume_token();
	}

	while (current_token.type != TOK_EOF) {
		statements->push_back(parse_program_statement());
		consume_token();
	}

	return new ASTProgramNode(name, alias, std::move(*statements));
}

ASTUsingNode* Parser::parse_using_statement() {
	std::vector<std::string> library = std::vector<std::string>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	do {
		consume_token(TOK_IDENTIFIER);
		library.push_back(current_token.value);
		if (next_token.type == TOK_DOT) {
			consume_token();
		}
	} while (next_token.type == TOK_IDENTIFIER);

	consume_token(TOK_SEMICOLON);

	return new ASTUsingNode(library, row, col);
}

ASTNode* Parser::parse_program_statement() {
	switch (current_token.type) {
	case TOK_USING:
		return parse_using_statement();
	case TOK_AS:
		return parse_as_namespace_statement();
	case TOK_DEF:
		return parse_function_definition();
	default:
		consume_semicolon = true;
		parse_block_statement();
	}
}

ASTNode* Parser::parse_block_statement() {
	switch (current_token.type) {
	case TOK_ENUM:
		return parse_enum_statement();
	case TOK_VAR:
	case TOK_CONST:
		return parse_declaration_statement();
	case TOK_STRUCT:
		return parse_struct_definition();
	case TOK_TRY:
		return parse_try_catch_statement();
	case TOK_THROW:
		return parse_throw_statement();
	case TOK_SWITCH:
		return parse_switch_statement();
	case TOK_IF:
		return parse_if_statement();
	case TOK_WHILE:
		return parse_while_statement();
	case TOK_DO:
		return parse_do_while_statement();
	case TOK_FOR:
		return parse_for_statement();
	case TOK_FOREACH:
		return parse_foreach_statement();
	case TOK_CONTINUE:
		return parse_continue_statement();
	case TOK_BREAK:
		return parse_break_statement();
	case TOK_EXIT:
		return parse_exit_statement();
	case TOK_RETURN:
		return parse_return_statement();
	case TOK_IDENTIFIER:
		return parse_identifier_statement();
	default:
		try {
			parse_statement_expression();
		}
		catch (std::exception ex) {
			throw std::runtime_error(msg_header() + "expected statement or expression");
		}
	}
}

ASTAsNamespaceNode* Parser::parse_as_namespace_statement() {
	std::string nmspace = "";
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_NAMESPACE);
	consume_token(TOK_IDENTIFIER);
	nmspace = current_token.value;
	consume_token(TOK_SEMICOLON);

	return new ASTAsNamespaceNode(nmspace, row, col);
}

ASTExprNode* Parser::parse_statement_expression() {
	ASTExprNode* expr = parse_expression();
	check_consume_semicolon();
	return expr;
}

ASTReturnNode* Parser::parse_return_statement() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token();
	ASTExprNode* expr = parse_expression();

	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(TOK_SEMICOLON);
	}

	return new ASTReturnNode(expr, row, col);
}

ASTExitNode* Parser::parse_exit_statement() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);

	consume_token();
	ASTExprNode* expr = parse_expression();

	consume_token(TOK_RIGHT_BRACKET);

	check_consume_semicolon();

	return new ASTExitNode(expr, row, col);
}

ASTEnumNode* Parser::parse_enum_statement() {
	auto identifiers = std::vector<std::string>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_CURLY);

	while (next_token.type != TOK_RIGHT_CURLY
		&& next_token.type != TOK_ERROR
		&& next_token.type != TOK_EOF) {
		consume_token(TOK_IDENTIFIER);
		identifiers.push_back(current_token.value);
		if (next_token.type == TOK_COMMA) {
			consume_token();
		}
	}

	consume_token(TOK_RIGHT_CURLY);
	check_consume_semicolon();

	return new ASTEnumNode(std::move(identifiers), row, col);
}

ASTBlockNode* Parser::parse_block() {
	auto statements = new std::vector<ASTNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token();
	while (current_token.type != TOK_RIGHT_CURLY
		&& current_token.type != TOK_ERROR
		&& current_token.type != TOK_EOF) {
		consume_semicolon = true;
		statements->push_back(parse_block_statement());
		consume_token();
	}

	if (current_token.type == TOK_RIGHT_CURLY) {
		return new ASTBlockNode(std::move(*statements), row, col);
	}
	throw std::runtime_error(msg_header() + "reached end of file while parsing");
}

ASTBlockNode* Parser::parse_struct_block() {
	auto statements = new std::vector<ASTNode*>;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token();
	while (current_token.type != TOK_RIGHT_CURLY
		&& current_token.type != TOK_ERROR
		&& current_token.type != TOK_EOF) {
		statements->push_back(parse_struct_block_variables());
		consume_token();
	}

	if (current_token.type == TOK_RIGHT_CURLY) {
		return new ASTBlockNode(std::move(*statements), row, col);
	}
	throw std::runtime_error(msg_header() + "mismatched scopes: reached end of file while parsing");
}

ASTStatementNode* Parser::parse_struct_block_variables() {
	switch (current_token.type) {
	case TOK_VAR:
		return parse_declaration_statement();

	default:
		throw std::runtime_error(msg_header() + "invalid declaration starting with '" + current_token.value + "' encountered");
	}
}

VariableDefinition* Parser::parse_struct_var_def() {
	bool is_rest = false;
	std::string identifier;
	std::string type_name;
	std::string type_name_space;
	Type type = Type::T_UNDEFINED;
	Type array_type = Type::T_UNDEFINED;
	ASTExprNode* expr_size;
	auto dim = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (current_token.type == TOK_RETICENCES) {
		is_rest = true;
		consume_token(TOK_IDENTIFIER);
	}
	else {
		check_current_token(TOK_IDENTIFIER);
	}

	auto id = parse_identifier();
	identifier = id.identifier;
	dim = id.access_vector;

	if (dim.size() > 0) {
		type = Type::T_ARRAY;
	}

	if (next_token.type == TOK_COLON) {
		consume_token();
		consume_token();

		if (next_token.type == TOK_LIB_ACESSOR_OP) {
			type_name_space = current_token.value;
			consume_token();
			consume_token();
		}

		if (type == Type::T_UNDEFINED) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEFINED
				|| current_array_type == parser::Type::T_VOID
				|| current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	if (type == Type::T_UNDEFINED) {
		type = Type::T_ANY;
	}

	array_type = current_array_type;

	if (is_rest) {
		auto ndim = new ASTLiteralNode<cp_int>(0, row, col);
		if (!is_array(type)) {
			array_type = type;
			type = Type::T_ARRAY;
		}
		dim.insert(dim.begin(), ndim);
	}

	return new VariableDefinition(identifier, type, type_name,
		type_name_space, array_type, std::move(dim), nullptr, is_rest, row, col);
};

ASTContinueNode* Parser::parse_continue_statement() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	check_consume_semicolon();

	return new ASTContinueNode(row, col);
}

ASTBreakNode* Parser::parse_break_statement() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	check_consume_semicolon();

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

	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	condition = parse_expression();
	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);
	consume_token();

	while (current_token.type == TOK_CASE) {
		bool is_block = false;
		consume_token();
		ASTExprNode* case_exrp = parse_expression();
		int start_position = statements->size();
		consume_token();
		consume_token();

		if (current_token.type == TOK_LEFT_CURLY) {
			statements->push_back(parse_block());
			consume_token();
		}
		else {
			while (current_token.type != TOK_CASE && current_token.type != TOK_DEFAULT
				&& current_token.type != TOK_RIGHT_CURLY && current_token.type != TOK_ERROR
				&& current_token.type != TOK_EOF) {
				consume_semicolon = true;
				statements->push_back(parse_block_statement());
				consume_token();
			}
		}
		case_blocks->emplace(case_exrp, start_position);
	}

	if (current_token.type == TOK_DEFAULT) {
		bool is_block = false;
		default_block = statements->size();
		consume_token();
		consume_token();
		if (current_token.type == TOK_LEFT_CURLY) {
			statements->push_back(parse_block());
			consume_token();
		}
		else {
			while (current_token.type != TOK_RIGHT_CURLY && current_token.type != TOK_ERROR && current_token.type != TOK_EOF) {
				consume_semicolon = true;
				statements->push_back(parse_block_statement());
				consume_token();
			}
		}
	}

	return new ASTSwitchNode(condition, std::move(*statements), std::move(*case_blocks), default_block, row, col);
}

ASTElseIfNode* Parser::parse_else_if_statement() {
	ASTExprNode* condition;
	ASTBlockNode* if_block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	condition = parse_expression();
	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);
	if_block = parse_block();

	return new ASTElseIfNode(condition, if_block, row, col);
}

ASTIfNode* Parser::parse_if_statement() {
	ASTExprNode* condition;
	ASTBlockNode* if_block;
	std::vector<ASTElseIfNode*> else_ifs = std::vector<ASTElseIfNode*>();
	ASTBlockNode* else_block = nullptr;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	condition = parse_expression();
	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);
	if_block = parse_block();

	if (next_token.type == TOK_ELSE) {
		consume_token();

		if (next_token.type == TOK_IF) {
			do {
				consume_token(TOK_IF);
				else_ifs.push_back(parse_else_if_statement());
				if (next_token.type == TOK_ELSE) {
					consume_token(TOK_ELSE);
				}
			} while (next_token.type == TOK_IF);
		}

		if (current_token.type == TOK_ELSE) {
			consume_token(TOK_LEFT_CURLY);
			else_block = parse_block();
		}
	}

	return new ASTIfNode(condition, if_block, std::move(else_ifs), else_block, row, col);
}

ASTTryCatchNode* Parser::parse_try_catch_statement() {
	ASTStatementNode* decl;
	ASTBlockNode* try_block;
	ASTBlockNode* catch_block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_CURLY);
	try_block = parse_block();
	check_current_token(TOK_RIGHT_CURLY);

	consume_token(TOK_CATCH);
	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	if (current_token.type == TOK_RETICENCES) {
		decl = new ASTReticencesNode(row, col);
	}
	else {
		consume_semicolon = false;
		decl = parse_undef_declaration_statement();
	}
	consume_token(TOK_RIGHT_BRACKET);

	consume_token(TOK_LEFT_CURLY);
	catch_block = parse_block();
	check_current_token(TOK_RIGHT_CURLY);

	return new ASTTryCatchNode(decl, try_block, catch_block, row, col);
}

ASTThrowNode* Parser::parse_throw_statement() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	ASTExprNode* expr;

	consume_token();
	expr = parse_expression();
	check_consume_semicolon();

	return new ASTThrowNode(expr, row, col);
}

ASTForNode* Parser::parse_for_statement() {
	std::array<ASTNode*, 3> dci = { nullptr };
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);

	for (int i = 0; i < 3; ++i) {
		if (next_token.type != TOK_SEMICOLON && next_token.type != TOK_RIGHT_BRACKET) {
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

	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);

	block = parse_block();

	return new ASTForNode(std::move(dci), block, row, col);
}

ASTNode* Parser::parse_foreach_collection() {
	consume_semicolon = false;

	switch (current_token.type) {
	case TOK_LEFT_CURLY:
		return parse_array_constructor_node();
	case TOK_IDENTIFIER:
		return parse_identifier_statement();
	default:
		throw std::runtime_error(msg_header() + "expected array");
	}
}

ASTForEachNode* Parser::parse_foreach_statement() {
	ASTDeclarationNode* itdecl;
	ASTNode* collection;
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	auto consaux = consume_semicolon;

	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	consume_semicolon = false;
	itdecl = parse_undef_declaration_statement();
	consume_token(TOK_IN);
	consume_token();
	collection = parse_foreach_collection();
	consume_semicolon = consaux;
	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);
	block = parse_block();

	return new ASTForEachNode(itdecl, collection, block, row, col);
}

ASTWhileNode* Parser::parse_while_statement() {
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);
	consume_token();
	condition = parse_expression();
	consume_token(TOK_RIGHT_BRACKET);
	consume_token(TOK_LEFT_CURLY);
	block = parse_block();

	return new ASTWhileNode(condition, block, row, col);
}

ASTDoWhileNode* Parser::parse_do_while_statement() {
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_CURLY);
	block = parse_block();
	check_current_token(TOK_RIGHT_CURLY);
	consume_token(TOK_WHILE);
	consume_token();
	condition = parse_expression();
	check_current_token(TOK_RIGHT_BRACKET);
	consume_semicolon = true;
	check_consume_semicolon();

	return new ASTDoWhileNode(condition, block, row, col);
}

ASTFunctionDefinitionNode* Parser::parse_function_definition() {
	std::string identifier;
	std::vector<VariableDefinition> parameters;
	Type type;
	Type array_type = parser::Type::T_UNDEFINED;
	std::string type_name = "";
	std::string type_name_space = "";
	ASTBlockNode* block = nullptr;
	auto dim_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_IDENTIFIER);
	identifier = current_token.value;
	consume_token(TOK_LEFT_BRACKET);

	if (next_token.type != TOK_RIGHT_BRACKET) {
		do {
			consume_token();
			parameters.push_back(*parse_formal_param());
			consume_token();
		} while (current_token.type == TOK_COMMA);
		check_current_token(TOK_RIGHT_BRACKET);
	}
	else {
		consume_token();
	}

	consume_token();
	if (current_token.type == TOK_COLON) {
		consume_token();
		if (current_token.type != TOK_BOOL_TYPE && current_token.type != TOK_INT_TYPE
			&& current_token.type != TOK_FLOAT_TYPE && current_token.type != TOK_CHAR_TYPE
			&& current_token.type != TOK_STRING_TYPE && current_token.type != TOK_IDENTIFIER
			&& current_token.type != TOK_FUNCTION_TYPE && current_token.type != TOK_VOID_TYPE
			&& current_token.type != TOK_ANY_TYPE) {
			throw std::runtime_error(msg_header() + "expected type");
		}
		if (next_token.type == TOK_LIB_ACESSOR_OP) {
			type_name_space = current_token.value;
			consume_token();
			consume_token();
		}
		if (current_token.type == TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
		type = parse_type();
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

	if (current_token.type == TOK_LEFT_CURLY) {
		block = parse_block();
	}
	else {
		if (current_token.type == TOK_COMMA) {
			throw std::runtime_error(msg_header() + "expected { or ;");
		}
	}

	return new ASTFunctionDefinitionNode(identifier, std::move(parameters), type,
		type_name, type_name_space, array_type, std::move(dim_vector), block, row, col);
}

ASTStructDefinitionNode* Parser::parse_struct_definition() {
	std::string identifier;
	std::vector<VariableDefinition> variables;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_IDENTIFIER);
	identifier = current_token.value;
	consume_token(TOK_LEFT_CURLY);

	do {
		consume_token(TOK_VAR);
		consume_token(TOK_IDENTIFIER);
		variables.push_back(*parse_struct_var_def());
		consume_token();

	} while (current_token.type == TOK_SEMICOLON && next_token.type != TOK_RIGHT_CURLY);

	consume_token(TOK_RIGHT_CURLY);

	check_consume_semicolon();

	return new ASTStructDefinitionNode(identifier, std::move(variables), row, col);
}

ASTExprNode* Parser::parse_expression() {
	return parse_ternary_expression();
}

ASTExprNode* Parser::parse_ternary_expression() {
	auto expr = parse_in_expression();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (next_token.type == TOK_QMARK) {
		ASTExprNode* value_if_true;
		ASTExprNode* value_if_false;
		consume_token();
		consume_token();
		value_if_true = parse_expression();
		consume_token(TOK_COLON);
		consume_token();
		value_if_false = parse_expression();
		return new ASTTernaryNode(expr, value_if_true, value_if_false, row, col);
	}

	return expr;
}

ASTExprNode* Parser::parse_in_expression() {
	bool vbv = current_token.type == TOK_DSIGN;
	if (vbv) {
		consume_token();
	}

	auto expr = parse_logical_or_expression();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (next_token.type == TOK_IN) {
		ASTExprNode* collection;
		consume_token();
		consume_token();
		collection = parse_expression();
		return new ASTInNode(expr, collection, vbv, row, col);
	}

	return expr;
}

ASTExprNode* Parser::parse_logical_or_expression() {
	ASTExprNode* lhs = parse_logical_and_expression();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == TOK_LOGICAL_OR_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		auto rhs = parse_logical_and_expression();
		lhs = new ASTBinaryExprNode(current_token_value, lhs, rhs, row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_logical_and_expression() {
	ASTExprNode* lhs = parse_relational_expression();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == TOK_LOGICAL_AND_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		auto rhs = parse_relational_expression();
		lhs = new ASTBinaryExprNode(current_token_value, lhs, rhs, row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_relational_expression() {
	ASTExprNode* lhs = parse_simple_expression();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == TOK_RELATIONAL_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		auto rhs = parse_simple_expression();
		lhs = new ASTBinaryExprNode(current_token_value, lhs, rhs, row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_simple_expression() {
	ASTExprNode* lhs = parse_term();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == TOK_ADDITIVE_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		auto rhs = parse_term();
		lhs = new ASTBinaryExprNode(current_token_value, lhs, rhs, row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_term() {
	ASTExprNode* lhs = parse_factor();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	while (next_token.type == TOK_MULTIPLICATIVE_OP) {
		consume_token();
		std::string current_token_value = current_token.value;
		consume_token();
		auto rhs = parse_factor();
		lhs = new ASTBinaryExprNode(current_token_value, lhs, rhs, row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parse_factor() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	switch (current_token.type) {
		// literal cases
	case TOK_BOOL_LITERAL:
		return new ASTLiteralNode<cp_bool>(parse_bool_literal(), row, col);

	case TOK_INT_LITERAL:
		return new ASTLiteralNode<cp_int>(parse_int_literal(), row, col);

	case TOK_FLOAT_LITERAL:
		return new ASTLiteralNode<cp_float>(parse_float_literal(), row, col);

	case TOK_CHAR_LITERAL:
		return new ASTLiteralNode<cp_char>(parse_char_literal(), row, col);

	case TOK_STRING_LITERAL:
		return new ASTLiteralNode<cp_string>(parse_string_literal(), row, col);

	case TOK_LEFT_CURLY:
		return parse_array_constructor_node();

	case TOK_BOOL_TYPE:
	case TOK_INT_TYPE:
	case TOK_FLOAT_TYPE:
	case TOK_CHAR_TYPE:
	case TOK_STRING_TYPE:
		return parse_type_parse_node();

	case TOK_NULL:
		return new ASTNullNode(row, col);

	case TOK_THIS:
		return parse_this_node();

	case TOK_TYPEOF:
	case TOK_TYPEID:
		return parse_typing_node();

	case TOK_IDENTIFIER:
		return parse_identifier_expression();

	case TOK_LEFT_BRACKET: { // subexpression case
		consume_token();
		ASTExprNode* sub_expr = parse_expression();
		consume_token(TOK_RIGHT_BRACKET);
		return sub_expr;
	}

	case TOK_ADDITIVE_OP: // unary expression case
	case TOK_NOT: {
		std::string current_token_value = current_token.value;
		consume_token();
		return new ASTUnaryExprNode(current_token_value, parse_factor(), row, col);
	}

	default:
		throw std::runtime_error(msg_header() + "expected expression");
	}
}

ASTExprNode* Parser::parse_identifier_expression() {
	ASTIdentifierNode* identifier = parse_identifier_node();

	switch (next_token.type) {
	case TOK_LEFT_BRACKET:
		return parse_function_call_node(identifier);

	case TOK_LEFT_CURLY:
		return parse_struct_constructor_node(identifier);

	case TOK_ADDITIVE_UN_OP: {
		consume_token();
		std::string op = current_token.value;
		return new ASTUnaryExprNode(op, identifier, identifier->row, identifier->col);
	}
	default:
		return identifier;
	}
}

ASTNode* Parser::parse_identifier_statement() {
	ASTIdentifierNode* identifier = parse_identifier_node();

	switch (next_token.type) {
	case TOK_LEFT_BRACKET: {
		if (identifier->identifier_vector.size() > 1) {
			throw std::runtime_error(msg_header() + "unexpected token '" + next_token.value + "'");
		}
		ASTFunctionCallNode* expr = parse_function_call_node(identifier);
		check_consume_semicolon();
		return expr;
	}
	case TOK_ADDITIVE_UN_OP:
		return parse_increment_expression(identifier);

	case TOK_ADDITIVE_OP:
	case TOK_EQUALS:
		return parse_assignment_statement(identifier);

	default:
		return parse_statement_expression();
	}
}

ASTFunctionCallNode* Parser::parse_function_call_node(ASTIdentifierNode* idnode) {
	std::string identifier = std::move(idnode->identifier_vector[0].identifier);
	std::string nmspace = std::move(idnode->nmspace);
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();
	auto* parameters = new std::vector<std::pair<bool, ASTExprNode*>>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	consume_token(TOK_LEFT_BRACKET);

	if (next_token.type != TOK_RIGHT_BRACKET) {
		parameters = parse_actual_params();
	}
	else {
		consume_token();
	}

	check_current_token(TOK_RIGHT_BRACKET);

	if (next_token.type == TOK_LEFT_BRACE) {
		consume_token();
		access_vector = parse_dimension_vector();
	}

	return new ASTFunctionCallNode(identifier, nmspace, std::move(access_vector), std::move(*parameters), row, col);
}

ASTUnaryExprNode* Parser::parse_increment_expression(ASTIdentifierNode* identifier) {
	consume_token();
	std::string op = current_token.value;

	check_consume_semicolon();

	return new ASTUnaryExprNode(op, identifier, identifier->row, identifier->col);
}

ASTIdentifierNode* Parser::parse_identifier_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	std::string nmspace = "";
	auto identifier_vector = std::vector<Identifier>();

	if (next_token.type == TOK_LIB_ACESSOR_OP) {
		nmspace = current_token.value;
		consume_token();
		consume_token();
	}

	identifier_vector = parse_identifier_vector();

	return new ASTIdentifierNode(std::move(identifier_vector), nmspace, row, col);
}

std::vector<ASTExprNode*> Parser::parse_dimension_vector() {
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();

	if (current_token.type == TOK_LEFT_BRACE) {
		do {
			ASTExprNode* expr_size = nullptr;
			consume_token();
			if (current_token.type != TOK_RIGHT_BRACE) {
				expr_size = parse_expression();
				consume_token(TOK_RIGHT_BRACE);
			}
			access_vector.push_back(expr_size);

			if (next_token.type == TOK_LEFT_BRACE) {
				consume_token();
			}

		} while (current_token.type == TOK_LEFT_BRACE);
	}

	return access_vector;
}

Identifier Parser::parse_identifier() {
	std::string identifier = "";
	std::vector<ASTExprNode*> access_vector = std::vector<ASTExprNode*>();

	identifier = current_token.value;

	if (next_token.type == TOK_LEFT_BRACE) {
		consume_token();
		access_vector = parse_dimension_vector();
	}

	return Identifier(identifier, std::move(access_vector));
}

std::vector<Identifier> Parser::parse_identifier_vector() {
	auto identifier_vector = std::vector<Identifier>();

	while (current_token.type == TOK_IDENTIFIER) {
		identifier_vector.push_back(parse_identifier());

		if (next_token.type == TOK_DOT) {
			consume_token();
			consume_token(TOK_IDENTIFIER);
		}
		else {
			break;
		}
	}

	return identifier_vector;
}

ASTAssignmentNode* Parser::parse_assignment_statement(ASTIdentifierNode* identifier) {
	std::string op = std::string();
	ASTExprNode* expr = nullptr;
	ASTExprNode* expr_size = nullptr;
	auto access_vector = std::vector<ASTExprNode*>();

	consume_token();
	if (current_token.type != TOK_ADDITIVE_OP && current_token.type != TOK_MULTIPLICATIVE_OP && current_token.type != TOK_EQUALS) {
		throw std::runtime_error(msg_header() + "invalid assignment operator '" + current_token.value + "'");
	}

	op = current_token.value;
	consume_token();
	expr = parse_expression();

	check_consume_semicolon();

	return new ASTAssignmentNode(std::move(identifier->identifier_vector), identifier->nmspace, op, expr, identifier->row, identifier->col);
}

ASTDeclarationNode* Parser::parse_declaration_statement() {
	Type type = Type::T_UNDEFINED;
	current_array_type = Type::T_UNDEFINED;
	std::string identifier;
	std::string type_name = "";
	std::string type_name_space = "";
	ASTExprNode* expr;
	auto dim_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	bool is_const = current_token.type == TOK_CONST;

	consume_token(TOK_IDENTIFIER);
	auto id = parse_identifier();
	identifier = id.identifier;
	dim_vector = id.access_vector;

	if (dim_vector.size() > 0) {
		type = Type::T_ARRAY;
	}

	if (next_token.type == TOK_COLON) {
		consume_token();
		consume_token();

		if (next_token.type == TOK_LIB_ACESSOR_OP) {
			type_name_space = current_token.value;
			consume_token();
			consume_token();
		}

		if (type == Type::T_UNDEFINED) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEFINED
				|| current_array_type == parser::Type::T_VOID
				|| current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	if (next_token.type == TOK_EQUALS) {
		consume_token();
		consume_token();
		expr = parse_expression();
	}
	else {
		expr = nullptr;
	}

	check_consume_semicolon();

	if (type == Type::T_UNDEFINED) {
		type = Type::T_ANY;
	}

	return new ASTDeclarationNode(identifier, type, current_array_type, std::move(dim_vector), type_name, type_name_space, expr, is_const, row, col);
}

ASTDeclarationNode* Parser::parse_undef_declaration_statement() {
	Type type = Type::T_UNDEFINED;
	current_array_type = Type::T_UNDEFINED;
	std::string identifier;
	std::string type_name = "";
	std::string type_name_space = "";
	ASTExprNode* expr;
	auto dim_vector = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	bool is_const = current_token.type == TOK_CONST;

	consume_token(TOK_IDENTIFIER);
	auto id = parse_identifier();
	identifier = id.identifier;
	dim_vector = id.access_vector;

	if (dim_vector.size() > 0) {
		type = Type::T_ARRAY;
	}

	if (next_token.type == TOK_COLON) {
		consume_token();
		consume_token();

		if (next_token.type == TOK_LIB_ACESSOR_OP) {
			type_name_space = current_token.value;
			consume_token();
			consume_token();
		}

		if (type == Type::T_UNDEFINED) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEFINED
				|| current_array_type == parser::Type::T_VOID
				|| current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	expr = nullptr;

	check_consume_semicolon();

	if (type == Type::T_UNDEFINED) {
		type = Type::T_ANY;
	}

	return new ASTDeclarationNode(identifier, type, current_array_type, std::move(dim_vector),
		type_name, type_name_space, expr, is_const, row, col);
}

VariableDefinition* Parser::parse_formal_param() {
	bool is_rest = false;
	std::string identifier;
	std::string type_name;
	std::string type_name_space;
	Type type = Type::T_UNDEFINED;
	Type array_type = Type::T_UNDEFINED;
	ASTExprNode* def_expr;
	auto dim = std::vector<ASTExprNode*>();
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	if (current_token.type == TOK_RETICENCES) {
		is_rest = true;
		consume_token(TOK_IDENTIFIER);
	}
	else {
		check_current_token(TOK_IDENTIFIER);
	}

	auto id = parse_identifier();
	identifier = id.identifier;
	dim = id.access_vector;

	if (dim.size() > 0) {
		type = Type::T_ARRAY;
	}

	if (next_token.type == TOK_COLON) {
		consume_token();
		consume_token();

		if (next_token.type == TOK_LIB_ACESSOR_OP) {
			type_name_space = current_token.value;
			consume_token();
			consume_token();
		}

		if (type == Type::T_UNDEFINED) {
			type = parse_type();
		}
		else if (type == Type::T_ARRAY) {
			current_array_type = parse_type();

			if (current_array_type == parser::Type::T_UNDEFINED
				|| current_array_type == parser::Type::T_VOID
				|| current_array_type == parser::Type::T_ARRAY) {
				current_array_type = parser::Type::T_ANY;
			}
		}

		if (current_token.type == TOK_IDENTIFIER) {
			type_name = current_token.value;
		}
	}

	if (type == Type::T_UNDEFINED) {
		type = Type::T_ANY;
	}

	array_type = current_array_type;

	if (is_rest) {
		auto ndim = new ASTLiteralNode<cp_int>(0, row, col);
		if (!is_array(type)) {
			array_type = type;
			type = Type::T_ARRAY;
		}
		dim.insert(dim.begin(), ndim);
	}

	if (next_token.type == TOK_EQUALS) {
		consume_token();
		consume_token();
		def_expr = parse_expression();
	}
	else {
		def_expr = nullptr;
	}

	return new VariableDefinition(identifier, type, type_name,
		type_name_space, array_type, std::move(dim), def_expr, is_rest, row, col);
};

std::vector<std::pair<bool, ASTExprNode*>>* Parser::parse_actual_params() {
	auto parameters = new std::vector<std::pair<bool, ASTExprNode*>>();

	do {
		consume_token();
		bool isref = false;
		if (current_token.type == TOK_REF) {
			consume_token();
			isref = true;
		}
		parameters->emplace_back(isref, parse_expression());
		consume_token();
	} while (current_token.type == TOK_COMMA);

	return parameters;
}

ASTTypingNode* Parser::parse_typing_node() {
	std::string image = current_token.value;
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	ASTExprNode* expr = nullptr;

	consume_token(TOK_LEFT_BRACKET);

	consume_token();

	switch (current_token.type) {
	case TOK_BOOL_TYPE:
	case TOK_INT_TYPE:
	case TOK_FLOAT_TYPE:
	case TOK_CHAR_TYPE:
	case TOK_STRING_TYPE: {
		auto id = parse_identifier();
		expr = new ASTIdentifierNode(std::vector{ id }, std::string(), row, col);
		break;
	}
	default:
		expr = parse_expression();
	}

	consume_token(TOK_RIGHT_BRACKET);

	return new ASTTypingNode(image, expr, row, col);
}

ASTArrayConstructorNode* Parser::parse_array_constructor_node() {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	std::vector<ASTExprNode*> values = std::vector<ASTExprNode*>();

	do {
		consume_token();
		values.push_back(parse_expression());

		consume_token();

	} while (next_token.type == TOK_LEFT_CURLY || next_token.type == TOK_NULL
		|| next_token.type == TOK_BOOL_LITERAL || next_token.type == TOK_INT_LITERAL
		|| next_token.type == TOK_IDENTIFIER || next_token.type == TOK_FLOAT_LITERAL
		|| next_token.type == TOK_CHAR_LITERAL || next_token.type == TOK_STRING_LITERAL);

	check_current_token(TOK_RIGHT_CURLY);

	return new ASTArrayConstructorNode(values, row, col);
}

ASTStructConstructorNode* Parser::parse_struct_constructor_node(ASTIdentifierNode* idnode) {
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	std::map<std::string, ASTExprNode*> values = std::map<std::string, ASTExprNode*>();
	std::string nmspace = std::move(idnode->nmspace);
	std::string type_name = std::move(idnode->identifier_vector[0].identifier);

	consume_token();
	consume_token();

	while (current_token.type == TOK_IDENTIFIER) {
		auto& var_identifier = current_token.value;

		consume_token(TOK_EQUALS);

		consume_token();
		values[var_identifier] = parse_expression();

		consume_token();

		if (next_token.type == TOK_IDENTIFIER) {
			consume_token();
		}
	}

	check_current_token(TOK_RIGHT_CURLY);

	return new ASTStructConstructorNode(type_name, nmspace, values, row, col);
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

ASTTypeParseNode* Parser::parse_type_parse_node() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;
	Type type = parse_type();

	consume_token(TOK_LEFT_BRACKET);

	// get expression
	consume_token();
	ASTExprNode* expr = parse_expression();

	// ensure right close bracket after fetching parameters
	consume_token(TOK_RIGHT_BRACKET);

	return new ASTTypeParseNode(type, expr, row, col);
}

ASTThisNode* Parser::parse_this_node() {
	// determine line number
	unsigned int row = current_token.row;
	unsigned int col = current_token.col;

	return new ASTThisNode(row, col);
}

void Parser::check_consume_semicolon() {
	if (consume_semicolon) {
		consume_semicolon = false;
		consume_token(TOK_SEMICOLON);
	}
}

std::string Parser::msg_header() {
	return "(PERR) " + name + '[' + std::to_string(current_token.row) + ':' + std::to_string(current_token.col) + "]: ";
}

Type Parser::parse_type() {
	switch (current_token.type) {
	case TOK_VOID_TYPE:
	case TOK_NULL:
		return Type::T_VOID;

	case TOK_BOOL_TYPE:
	case TOK_BOOL_LITERAL:
		return Type::T_BOOL;

	case TOK_INT_TYPE:
	case TOK_INT_LITERAL:
		return Type::T_INT;

	case TOK_FLOAT_TYPE:
	case TOK_FLOAT_LITERAL:
		return Type::T_FLOAT;

	case TOK_CHAR_TYPE:
	case TOK_CHAR_LITERAL:
		return Type::T_CHAR;

	case TOK_STRING_TYPE:
	case TOK_STRING_LITERAL:
		return Type::T_STRING;

	case TOK_ANY_TYPE:
		return Type::T_ANY;

	case TOK_FUNCTION_TYPE:
		return Type::T_FUNCTION;

	case TOK_IDENTIFIER:
		return Type::T_STRUCT;

	case TOK_LEFT_CURLY:
		return Type::T_ARRAY;

	default:
		throw std::runtime_error(msg_header() + "invalid type");
	}
}
