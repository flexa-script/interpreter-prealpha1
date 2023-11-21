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
	unsigned int lineNumber;

	// determine line number
	lineNumber = currentToken.lineNumber;

	// consume using
	consumeToken();

	library = currentToken.value;

	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after library " + library + " on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	return new ASTUsingNode(library, lineNumber);
}

ASTStatementNode* Parser::parseProgramStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
		return parseDeclarationStatement();
	case lexer::TOK_DEF:
		return parseFunctionDefinition();
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
		throw std::runtime_error("Invalid global statement starting with '" + currentToken.value + "' encountered on line " + std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTStatementNode* Parser::parseBlockStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
		return parseDeclarationStatement();
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
		throw std::runtime_error("Invalid statement starting with '" + currentToken.value + "' encountered on line " + std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTStatementNode* Parser::parseIdentifier() {
	if (nextToken.value == "(") {
		return parseFunctionCall();
	}
	else if (nextToken.value == "=") {
		return parseAssignmentStatement();
	}
	throw std::runtime_error("Expected '=' or '(' after identifier on line " + std::to_string(currentToken.lineNumber) + ".");
}

ASTDeclarationNode* Parser::parseDeclarationStatement() {
	// node attributes
	TYPE type;
	std::string identifier;
	ASTExprNode* expr;
	unsigned int lineNumber;

	// determine line number
	lineNumber = currentToken.lineNumber;

	// consume identifier
	consumeToken();
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error("Expected variable name after 'var' on line " + std::to_string(currentToken.lineNumber) + ".");
	}
	identifier = currentToken.value;

	consumeToken();
	if (currentToken.type != lexer::TOK_COLON) {
		throw std::runtime_error("Expected assignment operator '=' for " + identifier + " on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	consumeToken();
	type = parseType(identifier);

	consumeToken();
	if (currentToken.type == lexer::TOK_EQUALS) {
		// parse the right hand side
		expr = parseExpression();

		consumeToken();
		if (currentToken.type != lexer::TOK_SEMICOLON) {
			throw std::runtime_error("Expected ';' after assignment of " + identifier + " on line " + std::to_string(currentToken.lineNumber) + ".");
		}
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
			throw std::runtime_error("Expected type for " + identifier + " after ':' on line " + std::to_string(currentToken.lineNumber) + ".");
		}
	}

	// create ASTExpressionNode to return
	return new ASTDeclarationNode(type, identifier, expr, lineNumber);
}

ASTAssignmentNode* Parser::parseAssignmentStatement() {
	// node attributes
	std::string identifier;
	ASTExprNode* expr;

	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	identifier = currentToken.value;

	consumeToken();
	if (currentToken.type != lexer::TOK_EQUALS) {
		throw std::runtime_error("Expected assignment operator '=' after " + identifier + " on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// parse the right hand side
	expr = parseExpression();

	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after assignment of " + identifier + " on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	return new ASTAssignmentNode(identifier, expr, lineNumber);
}

ASTPrintNode* Parser::parsePrintStatement() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after print statement on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	return new ASTPrintNode(expr, lineNumber);
}

ASTReadNode* Parser::parseReadStatement() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// left open bracket
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after print statement on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	return new ASTReadNode(lineNumber);
}

ASTFunctionCallNode* Parser::parseFunctionCall() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
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
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after print statement on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	return new ASTFunctionCallNode(identifier, *parameters, lineNumber);
}

ASTReturnNode* Parser::parseReturnStatement() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// get expression to return
	ASTExprNode* expr = parseExpression();

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error("Expected ';' after return statement on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// return return node
	return new ASTReturnNode(expr, lineNumber);
}

ASTBlockNode* Parser::parseBlock() {
	auto statements = new std::vector<ASTStatementNode*>;

	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

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
		return new ASTBlockNode(*statements, lineNumber);
	}
	// otherwise the user left the block open
	else {
		throw std::runtime_error("Reached end of file while parsing. Mismatched scopes.");
	}
}

ASTIfNode* Parser::parseIfStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* ifBlock;
	unsigned int lineNumber = currentToken.lineNumber;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' after 'if' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' after if-condition on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error("Expected '{' after if-condition on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// consume if-block and '}'
	ifBlock = parseBlock();

	// lookahead whether there is an else
	if (nextToken.type != lexer::TOK_ELSE) {
		return new ASTIfNode(condition, ifBlock, lineNumber);
	}

	// otherwise, consume the else
	consumeToken();

	// consume '{' after else
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error("Expected '{' after else on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// parse else-block and '}'
	ASTBlockNode* elseBlock = parseBlock();

	// return if node
	return new ASTIfNode(condition, ifBlock, lineNumber, elseBlock);
}

ASTWhileNode* Parser::parseWhileStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* block;
	unsigned int lineNumber = currentToken.lineNumber;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' after 'while' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' after while-condition on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error("Expected '{' after while-condition on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// consume while-block and '}'
	block = parseBlock();

	// return while node
	return new ASTWhileNode(condition, block, lineNumber);
}

ASTFunctionDefinitionNode* Parser::parseFunctionDefinition() {
	// node attributes
	std::string identifier;
	std::vector<std::pair<std::string, TYPE>> parameters;
	TYPE type;
	ASTBlockNode* block;
	unsigned int lineNumber = currentToken.lineNumber;

	// consume identifier
	consumeToken();

	// make sure it is an identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error("Expected function identifier after keyword 'def' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	identifier = currentToken.value;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' after '" + identifier + "' on line " + std::to_string(currentToken.lineNumber) + ".");
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
			throw std::runtime_error("Expected ')' or more parameters on line " + std::to_string(currentToken.lineNumber) + ".");
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
		type = TYPE::VOID;
	}

	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error("Expected '{' after function '" + identifier + "' definition on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// parse block
	block = parseBlock();

	return new ASTFunctionDefinitionNode(identifier, parameters, type, block, lineNumber);
}

std::pair<std::string, TYPE>* Parser::parseFormalParam() {
	std::string identifier;
	TYPE type;

	// make sure current token is identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error("Expected variable name in function definition on line " + std::to_string(currentToken.lineNumber) + ".");
	}
	identifier = currentToken.value;

	// consume ':'
	consumeToken();

	if (currentToken.type != lexer::TOK_COLON) {
		throw std::runtime_error("Expected ':' after '" + identifier + "' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// consume type
	consumeToken();
	type = parseType(identifier);

	return new std::pair<std::string, TYPE>(identifier, type);

};

ASTExprNode* Parser::parseExpression() {
	ASTExprNode* simple_expr = parseLogicalExpression();
	return parseExpressionTail(simple_expr);
}

ASTExprNode* Parser::parseExpressionTail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_LOGICAL_OR_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		return new ASTBinaryExprNode(currentTokenValue, lhs, parseExpression(), lineNumber);
	}

	return lhs;
}

ASTExprNode* Parser::parseLogicalExpression() {
	ASTExprNode* relational_expr = parseRelationalExpression();
	return parseLogicalExpressionTail(relational_expr);
}

ASTExprNode* Parser::parseLogicalExpressionTail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	while (nextToken.type == lexer::TOK_LOGICAL_AND_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseRelationalExpression(), currentToken.lineNumber);
		return parseLogicalExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseRelationalExpression() {
	ASTExprNode* relationalExpr = parseSimpleExpression();
	return parseRelationalExpressionTail(relationalExpr);
}

ASTExprNode* Parser::parseRelationalExpressionTail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	while (nextToken.type == lexer::TOK_RELATIONAL_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseSimpleExpression(), currentToken.lineNumber);
		return parseRelationalExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseSimpleExpression() {
	ASTExprNode* term = parseTerm();
	return parseSimpleExpressionTail(term);
}

ASTExprNode* Parser::parseSimpleExpressionTail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_ADDITIVE_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseTerm(), lineNumber);
		return parseSimpleExpressionTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseTerm() {
	ASTExprNode* factor = parseFactor();
	return parseTermTail(factor);
}

ASTExprNode* Parser::parseTermTail(ASTExprNode* lhs) {
	unsigned int lineNumber = currentToken.lineNumber;

	if (nextToken.type == lexer::TOK_MULTIPLICATIVE_OP) {
		consumeToken();
		std::string currentTokenValue = currentToken.value;
		ASTExprNode* binExpr = new ASTBinaryExprNode(currentTokenValue, lhs, parseFactor(), lineNumber);
		return parseTermTail(binExpr);
	}

	return lhs;
}

ASTExprNode* Parser::parseFactor() {
	consumeToken();

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
		// remove " character from front and end of lexeme
		std::string str = currentToken.value.substr(1, currentToken.value.size() - 2);

		// replace \" with quote
		size_t pos = str.find("\\\"");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\"");
			// get next occurrence from current position
			pos = str.find("\\\"", pos + 2);
		}

		// replace \n with newline
		pos = str.find("\\n");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\n");
			// get next occurrence from current position
			pos = str.find("\\n", pos + 2);
		}

		// replace \t with tab
		pos = str.find("\\t");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\t");
			// get next occurrence from current position
			pos = str.find("\\t", pos + 2);
		}

		// replace \b with backslash
		pos = str.find("\\b");
		while (pos != std::string::npos) {
			// replace
			str.replace(pos, 2, "\\");
			// get next occurrence from current position
			pos = str.find("\\b", pos + 2);
		}

		return new ASTLiteralNode<std::string>(std::move(str), lineNumber);
	}

	case lexer::TOK_FLOAT_TYPE:
		return parseFloatParseExp();

	case lexer::TOK_INT_TYPE:
		return parseIntParseExp();

	case lexer::TOK_STRING_TYPE:
		return parseStringParseExp();

	case lexer::TOK_IDENTIFIER: // Identifier or function call case
		if (nextToken.type == lexer::TOK_LEFT_BRACKET)
			return parseExprFunctionCall();
		else return new ASTIdentifierNode(currentToken.value, lineNumber);

	case lexer::TOK_READ: // Read case
		return parseExprReadNode();

	case lexer::TOK_LEFT_BRACKET: // Subexpression case
	{
		ASTExprNode* sub_expr = parseExpression();
		consumeToken();
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
			throw std::runtime_error("Expected ')' after expression on line " + std::to_string(currentToken.lineNumber) + ".");
		}
		return sub_expr;
	}

	case lexer::TOK_ADDITIVE_OP: // Unary expression case
	case lexer::TOK_NOT: {
		std::string currentTokenValue = currentToken.value;
		return new ASTUnaryExprNode(currentTokenValue, parseExpression(), lineNumber);
	}

	default:
		throw std::runtime_error("Expected expression on line " + std::to_string(currentToken.lineNumber) + ".");
	}
}

ASTExprFunctionCallNode* Parser::parseExprFunctionCall() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
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
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	return new ASTExprFunctionCallNode(identifier, *parameters, lineNumber);
}

ASTFloatParseNode* Parser::parseFloatParseExp() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	return new ASTFloatParseNode(expr, lineNumber);
}

ASTIntParseNode* Parser::parseIntParseExp() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	return new ASTIntParseNode(expr, lineNumber);
}

ASTStringParseNode* Parser::parseStringParseExp() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	return new ASTStringParseNode(expr, lineNumber);
}

ASTExprReadNode* Parser::parseExprReadNode() {
	// determine line number
	unsigned int lineNumber = currentToken.lineNumber;

	// left open bracket
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error("Expected '(' on line " + std::to_string(currentToken.lineNumber) + ".");
	}

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error("Expected ')' on line " + std::to_string(currentToken.lineNumber) + " after function parameters.");
	}

	return new ASTExprReadNode(lineNumber);
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

TYPE Parser::parseType(std::string& identifier) {
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
		throw std::runtime_error("Expected type for " + identifier + " after ':' on line " + std::to_string(currentToken.lineNumber) + ".");
	}
}
