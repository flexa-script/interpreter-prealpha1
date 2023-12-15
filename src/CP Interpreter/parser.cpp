#include <iostream>

#include "parser.h"


using namespace parser;


Parser::Parser(lexer::Lexer* lex, std::string name) : lex(lex), name(name) {
	currentToken = lex->nextToken();
	nextToken = lex->nextToken();
}

Parser::Parser(lexer::Lexer* lex, std::string name, unsigned int tokens) : lex(lex), name(name) {
	nextToken = lex->nextToken();

	for (unsigned int i = 0; i < tokens; i++) {
		consumeToken();
	}
}

void Parser::consumeToken() {
	currentToken = nextToken;
	nextToken = lex->nextToken();
}

ASTProgramNode* Parser::parseProgram() {
	auto statements = new std::vector<ASTNode*>;

	while (currentToken.type == lexer::TOK_USING && currentToken.type != lexer::TOK_EOF) {
		statements->push_back(parseUsingStatement());
		consumeToken();
	}

	while (currentToken.type != lexer::TOK_EOF) {
		statements->push_back(parseProgramStatement());
		consumeToken();
	}

	return new ASTProgramNode(*statements, name);
}

ASTUsingNode* Parser::parseUsingStatement() {
	// node attributes
	std::string library;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume using
	consumeToken();

	library = currentToken.value;

	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after library " + library + ".");
	}

	return new ASTUsingNode(library, row, col);
}

ASTStatementNode* Parser::parseProgramStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
	case lexer::TOK_CONST:
		return parseDeclarationStatement();
	case lexer::TOK_DEF:
		return parseFunctionDefinition();
	case lexer::TOK_STRUCT:
		return parseStructDefinition();
	case lexer::TOK_PRINT:
		return parsePrintStatement();
	case lexer::TOK_READ:
		return parseReadStatement();
	case lexer::TOK_IF:
		return parseIfStatement();
	case lexer::TOK_WHILE:
		return parseWhileStatement();
	case lexer::TOK_RETURN:
		return parseReturnStatement();
	case lexer::TOK_IDENTIFIER:
		return parseIdentifier();

	default:
		throw std::runtime_error(msgHeader() + "invalid global statement starting with '" + currentToken.value + "' encountered.");
	}
}

ASTStatementNode* Parser::parseBlockStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
	case lexer::TOK_CONST:
		return parseDeclarationStatement();
	case lexer::TOK_STRUCT:
		return parseStructDefinition();
	case lexer::TOK_PRINT:
		return parsePrintStatement();
	case lexer::TOK_READ:
		return parseReadStatement();
	case lexer::TOK_IF:
		return parseIfStatement();
	case lexer::TOK_WHILE:
		return parseWhileStatement();
	case lexer::TOK_RETURN:
		return parseReturnStatement();
	case lexer::TOK_IDENTIFIER:
		return parseIdentifier();

	default:
		throw std::runtime_error(msgHeader() + "invalid statement starting with '" + currentToken.value + "' encountered.");
	}
}

ASTStatementNode* Parser::parseIdentifier() {
	if (nextToken.value == "(") {
		return parseFunctionCall();
	}
	else if (nextToken.value == "=") {
		return parseAssignmentStatement();
	}
	throw std::runtime_error(msgHeader() + "expected '=' or '(' after identifier.");
}

ASTDeclarationNode* Parser::parseDeclarationStatement() {
	TYPE type = TYPE::T_ND;
	std::string identifier;
	std::string typeName = "";
	ASTExprNode* expr;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	bool isConst = currentToken.type == lexer::TOK_CONST;

	// consume identifier
	consumeToken();
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected variable name after 'var'.");
	}
	identifier = currentToken.value;

	consumeToken();
	if (currentToken.type == lexer::TOK_COLON) {
		consumeToken();
		type = parseType(identifier);

		if (currentToken.type == lexer::TOK_IDENTIFIER) {
			typeName = currentToken.value;
		}

		consumeToken();
	}

	if (currentToken.type == lexer::TOK_EQUALS) {
		expr = parseExpression();

		if (type == TYPE::T_ND) {
			type = parseType(identifier);
		}
		
		consumeToken();
		if (currentToken.type != lexer::TOK_SEMICOLON) {
			throw std::runtime_error(msgHeader() + "expected ';' after assignment of " + identifier + ".");
		}
	}
	else {
		if (type == TYPE::T_ND) {
			throw std::runtime_error(msgHeader() + "expected assignment or type for " + identifier + ".");
		}

		switch (type) {
		case parser::TYPE::T_BOOL:
			expr = new ASTLiteralNode<bool>(false, row, col);
			break;
		case parser::TYPE::T_INT:
			expr = new ASTLiteralNode<__int64_t>(0, row, col);
			break;
		case parser::TYPE::T_FLOAT:
			expr = new ASTLiteralNode<long double>(0., row, col);
			break;
		case parser::TYPE::T_CHAR:
			expr = new ASTLiteralNode<char>(0, row, col);
			break;
		case parser::TYPE::T_STRING:
			expr = new ASTLiteralNode<std::string>("", row, col);
			break;
		case parser::TYPE::T_ANY:
			expr = new ASTLiteralNode<std::any>(NULL, row, col);
			break;
		case parser::TYPE::T_STRUCT:{
			expr = nullptr;
		}
			break;
		default:
			throw std::runtime_error(msgHeader() + "expected type for " + identifier + " after ':'.");
		}
	}

	return new ASTDeclarationNode(type, typeName, identifier, expr, isConst, row, col);
}

ASTAssignmentNode* Parser::parseAssignmentStatement() {
	std::string identifier;
	ASTExprNode* expr;

	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	identifier = currentToken.value;

	consumeToken();
	if (currentToken.type != lexer::TOK_EQUALS) {
		throw std::runtime_error(msgHeader() + "expected assignment operator '=' after " + identifier + ".");
	}

	// parse the right hand side
	expr = parseExpression();

	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after assignment of " + identifier + ".");
	}

	return new ASTAssignmentNode(identifier, expr, row, col);
}

ASTPrintNode* Parser::parsePrintStatement() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after print statement.");
	}

	return new ASTPrintNode(expr, row, col);
}

ASTReadNode* Parser::parseReadStatement() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// left open bracket
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after print statement.");
	}

	return new ASTReadNode(row, col);
}

ASTFunctionCallNode* Parser::parseFunctionCall() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// if next token is not right bracket, we have parameters
	if (nextToken.type != lexer::TOK_RIGHT_BRACKET) {
		parameters = parseActualParams();
	}
	else {
		// consume ')'
		consumeToken();
	}

	// ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after print statement.");
	}

	return new ASTFunctionCallNode(identifier, *parameters, row, col);
}

ASTReturnNode* Parser::parseReturnStatement() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// get expression to return
	ASTExprNode* expr = parseExpression();

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after return statement.");
	}

	// return return node
	return new ASTReturnNode(expr, row, col);
}

ASTBlockNode* Parser::parseBlock() {
	auto statements = new std::vector<ASTStatementNode*>;

	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// current token is '{', consume first token of statement
	consumeToken();

	// while not reached end of block or end of file
	while (currentToken.type != lexer::TOK_RIGHT_CURLY && currentToken.type != lexer::TOK_ERROR && currentToken.type != lexer::TOK_EOF) {
		// parse the statement
		statements->push_back(parseBlockStatement());

		// consume first token of next statement
		consumeToken();
	}

	// if block ended by '}', return block
	if (currentToken.type == lexer::TOK_RIGHT_CURLY) {
		return new ASTBlockNode(*statements, row, col);
	}
	// otherwise the user left the block open
	else {
		throw std::runtime_error(name + ": Reached end of file while parsing. Mismatched scopes.");
	}
}

ASTBlockNode* Parser::parseStructBlock() {
	auto statements = new std::vector<ASTStatementNode*>;

	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// current token is '{', consume first token of statement
	consumeToken();

	// while not reached end of block or end of file
	while (currentToken.type != lexer::TOK_RIGHT_CURLY && currentToken.type != lexer::TOK_ERROR && currentToken.type != lexer::TOK_EOF) {
		// parse the statement
		statements->push_back(parseStructBlockStatement());

		// consume first token of next statement
		consumeToken();
	}

	// if block ended by '}', return block
	if (currentToken.type == lexer::TOK_RIGHT_CURLY) {
		return new ASTBlockNode(*statements, row, col);
	}
	// otherwise the user left the block open
	else {
		throw std::runtime_error(name + ": Reached end of file while parsing. Mismatched scopes.");
	}
}

ASTStatementNode* Parser::parseStructBlockStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
	case lexer::TOK_CONST:
		return parseDeclarationStatement();

	default:
		throw std::runtime_error(msgHeader() + "invalid declaration starting with '" + currentToken.value + "' encountered.");
	}
}

ASTIfNode* Parser::parseIfStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* ifBlock;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after 'if'.");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after if-condition.");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after if-condition.");
	}

	// consume if-block and '}'
	ifBlock = parseBlock();

	// lookahead whether there is an else
	if (nextToken.type != lexer::TOK_ELSE) {
		return new ASTIfNode(condition, ifBlock, row, col);
	}

	// otherwise, consume the else
	consumeToken();

	// consume '{' after else
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after else.");
	}

	// parse else-block and '}'
	ASTBlockNode* elseBlock = parseBlock();

	// return if node
	return new ASTIfNode(condition, ifBlock, row, col, elseBlock);
}

ASTWhileNode* Parser::parseWhileStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after 'while'.");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after while-condition.");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after while-condition.");
	}

	// consume while-block and '}'
	block = parseBlock();

	// return while node
	return new ASTWhileNode(condition, block, row, col);
}

ASTFunctionDefinitionNode* Parser::parseFunctionDefinition() {
	// node attributes
	std::string identifier;
	std::vector<VariableDefinition_t> parameters;
	TYPE type;
	ASTBlockNode* block;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume identifier
	consumeToken();

	// make sure it is an identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected function identifier after keyword 'def'.");
	}

	identifier = currentToken.value;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after '" + identifier + "'.");
	}

	// consume ')' or parameters
	consumeToken();

	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		// parse first parameter
		parameters.push_back(*parseFormalParam());

		// consume ',' or ')'
		consumeToken();

		while (currentToken.type == lexer::TOK_COMMA) {
			// consume identifier
			consumeToken();

			// parse parameter
			parameters.push_back(*parseFormalParam());

			// consume ',' or ')'
			consumeToken();
		}

		// exited while-loop, so token must be ')'
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
			throw std::runtime_error(msgHeader() + "expected ')' or more parameters.");
		}
	}

	// consume ':' or '{'
	consumeToken();

	if (currentToken.type == lexer::TOK_COLON) {
		// consume type
		consumeToken();
		type = parseType(identifier);

		// consume '{'
		consumeToken();
	}
	else {
		type = TYPE::T_VOID;
	}

	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after function '" + identifier + "' definition.");
	}

	// parse block
	block = parseBlock();

	return new ASTFunctionDefinitionNode(identifier, parameters, type, block, row, col);
}

ASTStructDefinitionNode* Parser::parseStructDefinition() {
	// node attributes
	std::string identifier;
	std::vector<VariableDefinition_t> variables;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume identifier
	consumeToken();
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected struct identifier after keyword 'struct'.");
	}

	identifier = currentToken.value;

	// consume {
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after identifier of struct '" + identifier + "' definition.");
	}

	do {
		// consume var
		consumeToken();
		// consume identifier
		consumeToken();

		// parse parameter
		variables.push_back(*parseFormalParam());

		// consume ';'
		consumeToken();

	} while (currentToken.type == lexer::TOK_SEMICOLON && nextToken.type != lexer::TOK_RIGHT_CURLY);

	// consume }
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '}' to close struct.");
	}

	// consume ;
	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after struct definition.");
	}

	return new ASTStructDefinitionNode(identifier, variables, row, col);
}

VariableDefinition_t* Parser::parseFormalParam() {
	std::string identifier;
	std::string typeName;
	TYPE type;

	// make sure current token is identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected variable name in function definition.");
	}
	identifier = currentToken.value;

	// consume ':'
	consumeToken();

	if (currentToken.type != lexer::TOK_COLON) {
		throw std::runtime_error(msgHeader() + "expected ':' after '" + identifier + "'.");
	}

	// consume type
	consumeToken();
	typeName = currentToken.value;
	type = parseType(identifier);

	return new VariableDefinition_t(identifier, type, typeName, type == TYPE::T_ANY, false, currentToken.row, currentToken.col);

};

ASTExprNode* Parser::parseExpression() {
	ASTExprNode* simpleExpression = parseLogicalExpression();
	return parseExpressionTail(simpleExpression);
}

ASTExprNode* Parser::parseExpressionTail(ASTExprNode* lhs) {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	if (nextToken.type == lexer::TOK_LOGICAL_OR_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		return new ASTBinaryExprNode(currentTokenValue, lhs, parseExpression(), row, col);
	}

	return lhs;
}

ASTExprNode* Parser::parseLogicalExpression() {
	ASTExprNode* relationalExpression = parseRelationalExpression();
	return parseLogicalExpressionTail(relationalExpression);
}

ASTExprNode* Parser::parseLogicalExpressionTail(ASTExprNode* lhs) {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	while (nextToken.type == lexer::TOK_LOGICAL_AND_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseRelationalExpression(), row, col);
		return parseLogicalExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseRelationalExpression() {
	ASTExprNode* relationalExpr = parseSimpleExpression();
	return parseRelationalExpressionTail(relationalExpr);
}

ASTExprNode* Parser::parseRelationalExpressionTail(ASTExprNode* lhs) {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	while (nextToken.type == lexer::TOK_RELATIONAL_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseSimpleExpression(), row, col);
		return parseRelationalExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseSimpleExpression() {
	ASTExprNode* term = parseTerm();
	return parseSimpleExpressionTail(term);
}

ASTExprNode* Parser::parseSimpleExpressionTail(ASTExprNode* lhs) {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	if (nextToken.type == lexer::TOK_ADDITIVE_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseTerm(), row, col);
		return parseSimpleExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseTerm() {
	ASTExprNode* factor = parseFactor();
	return parseTermTail(factor);
}

ASTExprNode* Parser::parseTermTail(ASTExprNode* lhs) {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	if (nextToken.type == lexer::TOK_MULTIPLICATIVE_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseFactor(), row, col);
		return parseTermTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseFactor() {
	consumeToken();

	// Determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	switch (currentToken.type) {
		// Literal Cases
	case lexer::TOK_BOOL_LITERAL:
		return new ASTLiteralNode<bool>(currentToken.value == "true", row, col);

	case lexer::TOK_INT_LITERAL:
		return new ASTLiteralNode<__int64_t>(std::stoll(currentToken.value), row, col);

	case lexer::TOK_FLOAT_LITERAL:
		return new ASTLiteralNode<long double>(std::stold(currentToken.value), row, col);

	case lexer::TOK_CHAR_LITERAL: {
		char chr = 0;
		if (currentToken.value == "'\\\\'") {
			chr = '\\';
		}
		else if (currentToken.value == "'\\n'") {
			chr = '\n';
		}
		else if (currentToken.value == "'\\''") {
			chr = '\'';
		}
		else if (currentToken.value == "'\\t'") {
			chr = '\t';
		}
		else if (currentToken.value == "'\\b'") {
			chr = '\b';
		}
		else if (currentToken.value == "'\\0'") {
			chr = '\0';
		}
		else {
			chr = currentToken.value.c_str()[1];
		}
		return new ASTLiteralNode<char>(chr, row, col);
	}

	case lexer::TOK_STRING_LITERAL: {
		// remove " character from front and end of lexeme
		std::string str = currentToken.value.substr(1, currentToken.value.size() - 2);

		// replace \" with quote
		size_t pos = str.find("\\\"");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\"");
			// get next occurrence from current position
			pos = str.find("\\\"", pos + 1);
		}

		// replace \n with newline
		pos = str.find("\\n");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\n");
			// get next occurrence from current position
			pos = str.find("\\n", pos + 1);
		}

		// replace \t with tab
		pos = str.find("\\t");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\t");
			// get next occurrence from current position
			pos = str.find("\\t", pos + 1);
		}

		// replace \b with backslash
		pos = str.find("\\b");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\\");
			// get next occurrence from current position
			pos = str.find("\\b", pos + 1);
		}

		// replace \b with backslash
		pos = str.find("\\0");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\\");
			// get next occurrence from current position
			pos = str.find("\\0", pos + 1);
		}

		return new ASTLiteralNode<std::string>(std::move(str), row, col);
	}

	case lexer::TOK_FLOAT_TYPE:
		return parseExprToFloat();

	case lexer::TOK_INT_TYPE:
		return parseExprToInt();

	case lexer::TOK_STRING_TYPE:
		return parseExprToString();

	case lexer::TOK_THIS:
		return parseExprThis();

	case lexer::TOK_IDENTIFIER: // Identifier or function call case
		if (nextToken.type == lexer::TOK_LEFT_BRACKET)
			return parseExprFunctionCall();
		else return new ASTIdentifierNode(currentToken.value, row, col);

	case lexer::TOK_READ: // Read case
		return parseExprRead();

	case lexer::TOK_LEFT_BRACKET: // Subexpression case
	{
		ASTExprNode* sub_expr = parseExpression();
		consumeToken();
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
			throw std::runtime_error(msgHeader() + "expected ')' after expression.");
		}
		return sub_expr;
	}

	case lexer::TOK_ADDITIVE_OP: // Unary expression case
	case lexer::TOK_NOT: {
		std::string currentTokenValue = currentToken.value;
		return new ASTUnaryExprNode(currentTokenValue, parseExpression(), row, col);
	}

	default:
		throw std::runtime_error(msgHeader() + "expected expression.");
	}
}

ASTExprFunctionCallNode* Parser::parseExprFunctionCall() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// if next token is not right bracket, we have parameters
	if (nextToken.type != lexer::TOK_RIGHT_BRACKET) {
		parameters = parseActualParams();
	}
	else {
		// consume ')'
		consumeToken();
	}

	// ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	return new ASTExprFunctionCallNode(identifier, *parameters, row, col);
}

ASTFloatParseNode* Parser::parseExprToFloat() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	return new ASTFloatParseNode(expr, row, col);
}

ASTIntParseNode* Parser::parseExprToInt() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	return new ASTIntParseNode(expr, row, col);
}

ASTThisNode* Parser::parseExprThis() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	return new ASTThisNode(row, col);
}

ASTStringParseNode* Parser::parseExprToString() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	return new ASTStringParseNode(expr, row, col);
}

ASTExprReadNode* Parser::parseExprRead() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// left open bracket
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters.");
	}

	return new ASTExprReadNode(row, col);
}

std::vector<ASTExprNode*>* Parser::parseActualParams() {
	auto parameters = new std::vector<ASTExprNode*>;

	parameters->push_back(parseExpression());
	consumeToken();

	// if there are more
	while (currentToken.type == lexer::TOK_COMMA) {
		parameters->push_back(parseExpression());
		consumeToken();
	}

	return parameters;
}

std::string Parser::msgHeader() {
	return name + '[' + std::to_string(currentToken.row) + ':' + std::to_string(currentToken.col) + "]: ";
}

TYPE Parser::parseType(std::string& identifier) {
	switch (currentToken.type) {
	case lexer::TOK_VOID_TYPE:
		return TYPE::T_VOID;

	case lexer::TOK_BOOL_TYPE:
	case lexer::TOK_BOOL_LITERAL:
		return TYPE::T_BOOL;

	case lexer::TOK_INT_TYPE:
	case lexer::TOK_INT_LITERAL:
		return TYPE::T_INT;

	case lexer::TOK_FLOAT_TYPE:
	case lexer::TOK_FLOAT_LITERAL:
		return TYPE::T_FLOAT;

	case lexer::TOK_CHAR_TYPE:
	case lexer::TOK_CHAR_LITERAL:
		return TYPE::T_CHAR;

	case lexer::TOK_STRING_TYPE:
	case lexer::TOK_STRING_LITERAL:
		return TYPE::T_STRING;

	case lexer::TOK_ANY_TYPE:
		return TYPE::T_ANY;

	case lexer::TOK_IDENTIFIER:
		return TYPE::T_STRUCT;

	default:
		throw std::runtime_error(msgHeader() + "expected type for " + identifier + " after ':'.");
	}
}
