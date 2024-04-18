#include <iostream>

#include "parser.h"
#include "util.h"


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
		throw std::runtime_error(msgHeader() + "expected ';' after library " + library + "");
	}

	return new ASTUsingNode(library, row, col);
}

ASTNode* Parser::parseProgramStatement() {
	switch (currentToken.type) {
	case lexer::TOK_DEF:
		return parseFunctionDefinition();
	default:
		parseBlockStatement();
	}
}

ASTNode* Parser::parseBlockStatement() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
	case lexer::TOK_CONST:
		return parseDeclarationStatement();
	case lexer::TOK_STRUCT:
		return parseStructDefinition();
	case lexer::TOK_PRINT:
		return parsePrintStatement();
	case lexer::TOK_IF:
		return parseIfStatement();
	case lexer::TOK_WHILE:
		return parseWhileStatement();
	case lexer::TOK_RETURN:
		return parseReturnStatement();
	case lexer::TOK_IDENTIFIER:
		return parseIdentifier();

	default:
		ASTExprNode* expr = parseExpression();
		consumeToken();
		if (currentToken.type != lexer::TOK_SEMICOLON) {
			throw std::runtime_error(msgHeader() + "expected ';'");
		}
		return expr;
	}
}

ASTNode* Parser::parseIdentifier() {
	if (nextToken.value == "(") {
		ASTFunctionCallNode* expr = parseExprFunctionCall();
		consumeToken();
		if (currentToken.type != lexer::TOK_SEMICOLON) {
			throw std::runtime_error(msgHeader() + "expected ';'");
		}
		return expr;
	}
	else if (nextToken.value == "[" || nextToken.value == "=") {
		return parseAssignmentStatement();
	}
	throw std::runtime_error(msgHeader() + "expected '[' or '=' after identifier");
}

ASTDeclarationNode* Parser::parseDeclarationStatement() {
	TYPE type = TYPE::T_ND;
	currentArrayType = TYPE::T_ND;
	std::string identifier;
	std::string typeName = "";
	ASTExprNode* expr;
	auto dim = std::vector<int>();
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	bool isConst = currentToken.type == lexer::TOK_CONST;

	// consume identifier
	consumeToken();
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected variable name on definition");
	}
	identifier = currentToken.value;

	consumeToken();

	if (currentToken.type == lexer::TOK_LEFT_BRACE) {
		type = TYPE::T_ARRAY;
		do {
			auto size = -1;
			consumeToken();
			if (currentToken.type == lexer::TOK_INT_LITERAL) {
				size = stoi(currentToken.value);
				consumeToken();
			}
			if (currentToken.type != lexer::TOK_RIGHT_BRACE) {
				throw std::runtime_error(msgHeader() + "expected ']'");
			}
			consumeToken();
			dim.push_back(size);

		} while (currentToken.type == lexer::TOK_LEFT_BRACE);
	}

	if (currentToken.type == lexer::TOK_COLON) {
		consumeToken();
		if (type == TYPE::T_ND) {
			type = parseType(identifier);
		}
		else if (type == TYPE::T_ARRAY) {
			currentArrayType = parseType(identifier);

			if (currentArrayType == parser::TYPE::T_ND || currentArrayType == parser::TYPE::T_NULL || currentArrayType == parser::TYPE::T_ARRAY) {
				currentArrayType = parser::TYPE::T_ANY;
			}
		}

		if (currentToken.type == lexer::TOK_IDENTIFIER) {
			typeName = currentToken.value;
		}

		consumeToken();
	}

	if (dim.size() > 0 && dim.at(0) <= 0 && currentToken.type != lexer::TOK_EQUALS) {
		throw std::runtime_error(msgHeader() + "expected array size or assignment on '" + identifier + "' definition");
	}

	if (currentToken.type == lexer::TOK_EQUALS) {
		expr = parseExpression();

		if (type == TYPE::T_ND) {
			type = TYPE::T_ANY;
		}

		consumeToken();
		if (currentToken.type != lexer::TOK_SEMICOLON) {
			throw std::runtime_error(msgHeader() + "expected ';' after assignment of " + identifier + "");
		}
	}
	else {
		expr = nullptr;
	}

	return new ASTDeclarationNode(type, typeName, identifier, expr, isConst, currentArrayType, dim, row, col);
}

ASTAssignmentNode* Parser::parseAssignmentStatement() {
	std::string identifier;
	ASTExprNode* expr;
	ASTExprNode* exprSize;
	auto identifierVector = std::vector<std::string>();
	auto accessVector = std::vector<ASTExprNode*>();

	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	identifier = currentToken.value;

	if (axe::contains(identifier, ".")) {
		identifierVector = axe::split(identifier, '.');
	}
	else {
		identifierVector.push_back(identifier);
	}

	consumeToken();

	if (currentToken.type == lexer::TOK_LEFT_BRACE) {
		do {
			exprSize = parseExpression();
			consumeToken();
			if (currentToken.type != lexer::TOK_RIGHT_BRACE) {
				throw std::runtime_error(msgHeader() + "expected ']' after array position expression");
			}
			consumeToken();
			accessVector.push_back(exprSize);

		} while (currentToken.type == lexer::TOK_LEFT_BRACE);
	}

	if (currentToken.type != lexer::TOK_EQUALS) {
		throw std::runtime_error(msgHeader() + "expected assignment operator '=' after " + identifier + "");
	}

	// parse the right hand side
	expr = parseExpression();

	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after assignment of " + identifier + "");
	}

	return new ASTAssignmentNode(identifierVector[0], identifierVector, expr, accessVector, row, col);
}

ASTPrintNode* Parser::parsePrintStatement() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	// consume ';' token
	consumeToken();

	// make sure it's a ';'
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after print statement");
	}

	return new ASTPrintNode(expr, row, col);
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
		throw std::runtime_error(msgHeader() + "expected ';' after return statement");
	}

	// return return node
	return new ASTReturnNode(expr, row, col);
}

ASTBlockNode* Parser::parseBlock() {
	auto statements = new std::vector<ASTNode*>;

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
		throw std::runtime_error(name + ": reached end of file while parsing");
	}
}

ASTBlockNode* Parser::parseStructBlock() {
	auto statements = new std::vector<ASTNode*>;

	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// current token is '{', consume first token of statement
	consumeToken();

	// while not reached end of block or end of file
	while (currentToken.type != lexer::TOK_RIGHT_CURLY && currentToken.type != lexer::TOK_ERROR && currentToken.type != lexer::TOK_EOF) {
		// parse the statement
		statements->push_back(parseStructBlockVariables());

		// consume first token of next statement
		consumeToken();
	}

	// if block ended by '}', return block
	if (currentToken.type == lexer::TOK_RIGHT_CURLY) {
		return new ASTBlockNode(*statements, row, col);
	}
	// otherwise the user left the block open
	else {
		throw std::runtime_error(name + ": Reached end of file while parsing. Mismatched scopes");
	}
}

ASTStatementNode* Parser::parseStructBlockVariables() {
	switch (currentToken.type) {
	case lexer::TOK_VAR:
		return parseDeclarationStatement();

	default:
		throw std::runtime_error(msgHeader() + "invalid declaration starting with '" + currentToken.value + "' encountered");
	}
}

ASTElseIfNode* Parser::parseElseIfStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* ifBlock;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume 'if'
	consumeToken();

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after 'if'");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after if-condition");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after if-condition");
	}

	// consume if-block and '}'
	ifBlock = parseBlock();

	consumeToken();

	// return elif node
	return new ASTElseIfNode(condition, ifBlock, row, col);
}

ASTIfNode* Parser::parseIfStatement() {
	// node attributes
	ASTExprNode* condition;
	ASTBlockNode* ifBlock;
	std::vector<ASTElseIfNode*> elseIfs = std::vector<ASTElseIfNode*>();
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after 'if'");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after if-condition");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after if-condition");
	}

	// consume if-block and '}'
	ifBlock = parseBlock();

	// lookahead whether there is an else
	if (nextToken.type != lexer::TOK_ELSE) {
		return new ASTIfNode(condition, ifBlock, elseIfs, row, col);
	}

	// otherwise, consume the else
	consumeToken();

	if (nextToken.type == lexer::TOK_IF) {
		while (nextToken.type == lexer::TOK_IF) {
			elseIfs.push_back(parseElseIfStatement());
		}
	}

	// consume '{' after else
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after else");
	}

	// parse else-block and '}'
	ASTBlockNode* elseBlock = parseBlock();

	// return if node
	return new ASTIfNode(condition, ifBlock, elseIfs, row, col, elseBlock);
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
		throw std::runtime_error(msgHeader() + "expected '(' after 'while'");
	}

	// parse the expression
	condition = parseExpression();

	// consume ')'
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after while-condition");
	}

	// consume '{'
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after while-condition");
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
	std::string typeName = "";
	ASTBlockNode* block;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// consume identifier
	consumeToken();

	// make sure it is an identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected function identifier after keyword 'def'");
	}

	identifier = currentToken.value;

	// consume '('
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '(' after '" + identifier + "'");
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
			throw std::runtime_error(msgHeader() + "expected ')' or more parameters");
		}
	}

	// consume ':' or '{'
	consumeToken();

	if (currentToken.type == lexer::TOK_COLON) {
		// consume type
		consumeToken();

		if (currentToken.type == lexer::TOK_IDENTIFIER) {
			typeName = currentToken.value;
		}

		type = parseType(identifier);

		// consume '{'
		consumeToken();
	}
	else {
		type = TYPE::T_VOID;
	}

	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after function '" + identifier + "' definition");
	}

	// parse block
	block = parseBlock();

	return new ASTFunctionDefinitionNode(identifier, parameters, type, typeName, block, row, col);
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
		throw std::runtime_error(msgHeader() + "expected struct identifier after keyword 'struct'");
	}

	identifier = currentToken.value;

	// consume {
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '{' after identifier of struct '" + identifier + "' definition");
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
		throw std::runtime_error(msgHeader() + "expected '}' to close struct");
	}

	// consume ;
	consumeToken();
	if (currentToken.type != lexer::TOK_SEMICOLON) {
		throw std::runtime_error(msgHeader() + "expected ';' after struct definition");
	}

	return new ASTStructDefinitionNode(identifier, variables, row, col);
}

VariableDefinition_t* Parser::parseFormalParam() {
	std::string identifier;
	std::string typeName;
	TYPE type = TYPE::T_ND;
	auto dim = std::vector<int>();

	// make sure current token is identifier
	if (currentToken.type != lexer::TOK_IDENTIFIER) {
		throw std::runtime_error(msgHeader() + "expected variable name in function definition");
	}
	identifier = currentToken.value;

	consumeToken();

	if (currentToken.type == lexer::TOK_LEFT_BRACE) {
		type = TYPE::T_ARRAY;
		do {
			auto size = -1;
			consumeToken();
			if (currentToken.type == lexer::TOK_INT_LITERAL) {
				size = stoi(currentToken.value);
				consumeToken();
			}
			if (currentToken.type != lexer::TOK_RIGHT_BRACE) {
				throw std::runtime_error(msgHeader() + "expected ']' after array size constant");
			}
			consumeToken();
			dim.push_back(size);

		} while (currentToken.type == lexer::TOK_LEFT_BRACE);
	}

	if (currentToken.type != lexer::TOK_COLON) {
		throw std::runtime_error(msgHeader() + "expected ':' after '" + identifier + "'");
	}

	// consume type
	consumeToken();

	if (type == TYPE::T_ARRAY) {
		currentArrayType = parseType(identifier);
	}
	else {
		type = parseType(identifier);
	}
	if (currentToken.type == lexer::TOK_IDENTIFIER) {
		typeName = currentToken.value;
	}

	return new VariableDefinition_t(identifier, type, typeName, currentArrayType, dim, false, false, currentToken.row, currentToken.col, true);
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

	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	switch (currentToken.type) {
		// literal cases
	case lexer::TOK_BOOL_LITERAL:
		return new ASTLiteralNode<cp_bool>(parseBoolLiteral(), row, col);

	case lexer::TOK_INT_LITERAL:
		return new ASTLiteralNode<cp_int>(parseIntLiteral(), row, col);

	case lexer::TOK_FLOAT_LITERAL:
		return new ASTLiteralNode<cp_float>(parseFloatLiteral(), row, col);

	case lexer::TOK_CHAR_LITERAL:
		return new ASTLiteralNode<cp_char>(parseCharLiteral(), row, col);

	case lexer::TOK_STRING_LITERAL:
		return new ASTLiteralNode<cp_string>(parseStringLiteral(), row, col);

	case lexer::TOK_LEFT_CURLY:
		return parseArrayConstructorNode();

	case lexer::TOK_BOOL_TYPE:
	case lexer::TOK_INT_TYPE:
	case lexer::TOK_FLOAT_TYPE:
	case lexer::TOK_CHAR_TYPE:
	case lexer::TOK_STRING_TYPE:
		return parseExprTypeParse();

	case lexer::TOK_TYPE:
		return parseTypeNode();

	case lexer::TOK_LEN:
		return parseLenNode();

	case lexer::TOK_ROUND:
		return parseRoundNode();

	case lexer::TOK_NULL:
		return new ASTNullNode(row, col);

	case lexer::TOK_THIS:
		return parseExprThis();

	case lexer::TOK_IDENTIFIER: // identifier or function call case
		switch (nextToken.type)
		{
		case lexer::TOK_LEFT_BRACKET:
			return parseExprFunctionCall();
		case lexer::TOK_LEFT_CURLY:
			return parseStructConstructorNode();
		default:
			return parseIdentifierNode();
		}

	case lexer::TOK_READ: // read case
		return parseExprRead();

	case lexer::TOK_LEFT_BRACKET: { // subexpression case
		ASTExprNode* subExpr = parseExpression();
		consumeToken();
		if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
			throw std::runtime_error(msgHeader() + "expected ')' after expression");
		}
		return subExpr;
	}

	case lexer::TOK_ADDITIVE_OP: // unary expression case
	case lexer::TOK_NOT: {
		std::string currentTokenValue = currentToken.value;
		return new ASTUnaryExprNode(currentTokenValue, parseExpression(), row, col);
	}

	default:
		throw std::runtime_error(msgHeader() + "expected expression");
	}
}

ASTTypeNode* Parser::parseTypeNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	ASTExprNode* expr = nullptr;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
	}

	expr = parseExpression();
	consumeToken();

	// ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTTypeNode(expr, row, col);
}

ASTLenNode* Parser::parseLenNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	ASTExprNode* expr = nullptr;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
	}

	expr = parseExpression();
	consumeToken();

	// ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTLenNode(expr, row, col);
}

ASTRoundNode* Parser::parseRoundNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	unsigned int ndigits = 0;
	ASTExprNode* expr = nullptr;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('.");
	}

	expr = parseExpression();
	consumeToken();

	// ensure right close bracket after fetching parameters
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTRoundNode(expr, ndigits, row, col);
}

ASTIdentifierNode* Parser::parseIdentifierNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	auto identifierVector = std::vector<std::string>();
	auto accessVector = std::vector<unsigned int>();
	if (axe::contains(currentToken.value, ".")) {
		identifierVector = axe::split(currentToken.value, '.');
	}
	else {
		identifierVector.push_back(currentToken.value);
	}

	if (nextToken.type == lexer::TOK_LEFT_BRACE) {
		consumeToken();
		do {
			auto pos = 0;
			consumeToken();
			if (currentToken.type != lexer::TOK_INT_LITERAL) {
				throw std::runtime_error(msgHeader() + "expected int literal");
			}
			pos = stoi(currentToken.value);
			consumeToken();
			if (currentToken.type != lexer::TOK_RIGHT_BRACE) {
				throw std::runtime_error(msgHeader() + "expected ']' after array position constant");
			}
			accessVector.push_back(pos);

			if (nextToken.type == lexer::TOK_LEFT_BRACE) {
				consumeToken();
			}

		} while (currentToken.type == lexer::TOK_LEFT_BRACE);
	}

	return new ASTIdentifierNode(identifierVector[0], identifierVector, accessVector, row, col);
}

ASTArrayConstructorNode* Parser::parseArrayConstructorNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	std::vector<ASTExprNode*> values = std::vector<ASTExprNode*>();

	do {
		values.push_back(parseExpression());

		consumeToken();

	} while (nextToken.type == lexer::TOK_LEFT_CURLY || nextToken.type == lexer::TOK_NULL
		|| nextToken.type == lexer::TOK_BOOL_LITERAL || nextToken.type == lexer::TOK_INT_LITERAL 
		|| nextToken.type == lexer::TOK_IDENTIFIER   || nextToken.type == lexer::TOK_FLOAT_LITERAL 
		|| nextToken.type == lexer::TOK_CHAR_LITERAL || nextToken.type == lexer::TOK_STRING_LITERAL);


	if (currentToken.type != lexer::TOK_RIGHT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '}'");
	}

	return new ASTArrayConstructorNode(values, row, col);
}

ASTStructConstructorNode* Parser::parseStructConstructorNode() {
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	std::map<std::string, ASTExprNode*> values = std::map<std::string, ASTExprNode*>();
	std::string typeName = currentToken.value;

	consumeToken();
	consumeToken();

	while (currentToken.type == lexer::TOK_IDENTIFIER) {
		auto varIdentifier = currentToken.value;

		consumeToken();
		if (currentToken.type != lexer::TOK_EQUALS) {
			throw std::runtime_error(msgHeader() + "expected '='");
		}

		values[varIdentifier] = parseExpression();

		consumeToken();

		if (nextToken.type == lexer::TOK_IDENTIFIER) {
			consumeToken();
		}
	}

	if (currentToken.type != lexer::TOK_RIGHT_CURLY) {
		throw std::runtime_error(msgHeader() + "expected '}'");
	}

	return new ASTStructConstructorNode(typeName, values, row, col);
}

cp_bool Parser::parseBoolLiteral() {
	return currentToken.value == "true";
}

cp_int Parser::parseIntLiteral() {
	return std::stoll(currentToken.value);
}

cp_float Parser::parseFloatLiteral() {
	return std::stold(currentToken.value);
}

cp_char Parser::parseCharLiteral() {
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
	return chr;
}

cp_string Parser::parseStringLiteral() {
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

	return str;
}

ASTFunctionCallNode* Parser::parseExprFunctionCall() {
	// current token is the function identifier
	std::string identifier = currentToken.value;
	auto* parameters = new std::vector<ASTExprNode*>;
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
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
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTFunctionCallNode(identifier, *parameters, row, col);
}

ASTTypeParseNode* Parser::parseExprTypeParse() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;
	TYPE type = parseType("type");

	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
	}

	// get expression to print
	ASTExprNode* expr = parseExpression();

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTTypeParseNode(type, expr, row, col);
}

ASTThisNode* Parser::parseExprThis() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	return new ASTThisNode(row, col);
}

ASTReadNode* Parser::parseExprRead() {
	// determine line number
	unsigned int row = currentToken.row;
	unsigned int col = currentToken.col;

	// left open bracket
	consumeToken();
	if (currentToken.type != lexer::TOK_LEFT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected '('");
	}

	// ensure right close bracket after fetching parameters
	consumeToken();
	if (currentToken.type != lexer::TOK_RIGHT_BRACKET) {
		throw std::runtime_error(msgHeader() + "expected ')' after function parameters");
	}

	return new ASTReadNode(row, col);
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
	return "(PERR) " + name + '[' + std::to_string(currentToken.row) + ':' + std::to_string(currentToken.col) + "]: ";
}

TYPE Parser::parseType(std::string identifier) {
	switch (currentToken.type) {
	case lexer::TOK_VOID_TYPE:
		return TYPE::T_VOID;

	case lexer::TOK_NULL:
		return TYPE::T_NULL;

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

	case lexer::TOK_LEFT_CURLY:
		return TYPE::T_ARRAY;

	default:
		throw std::runtime_error(msgHeader() + "invalid type");
	}
}
