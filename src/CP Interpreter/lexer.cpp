#include <stack>
#include <stdexcept>

#include "lexer.h"


using namespace lexer;


Lexer::Lexer(std::string& source, std::string name) 
	: source(source), name(name) {
	tokenize();
}

void Lexer::tokenize() {
	currentRow = 1;
	currentCol = 1;

	while (hasNext()) {
		currentChar = source[currentIndex];

		if (currentChar == '\n') {
			++currentRow;
		}

		if (isSpace()) {
			// ignore white spaces
			advance();
		}
		else if (currentChar == '/' && (getNextChar() == '/' || getNextChar() == '*')) {
			processComment();
		}
		else if (currentChar == '\'') {
			startCol = currentCol;
			tokens.push_back(processChar());
		}
		else if (currentChar == '"') {
			startCol = currentCol;
			tokens.push_back(processString());
		}
		else if (std::isalpha(currentChar) || currentChar == '_') {
			startCol = currentCol;
			tokens.push_back(processIdentifier());
		}
		else if (std::isdigit(currentChar) || currentChar == '.') {
			startCol = currentCol;
			tokens.push_back(processNumber());
		}
		else {
			startCol = currentCol;
			tokens.push_back(processSymbol());
		}
	}

	tokens.push_back(Token(TOKEN_TYPE::TOK_EOF, "", 0, 0));
}

char Lexer::getNextChar() {
	return currentIndex + 1 < source.length() ? source[currentIndex + 1] : -1;
}

bool Lexer::hasNext() {
	return currentIndex < source.length();
}

bool Lexer::isSpace() {
	return std::isspace(currentChar) || currentChar == '\t' || currentChar == '\r' || currentChar == '\n';
}

void Lexer::advance() {
	if (currentChar == '\n') {
		currentCol = 1;
	}
	else {
		++currentCol;
	}
	beforeChar = currentChar;
	currentChar = source[++currentIndex];
}

Token Lexer::processComment() {
	std::string comment;
	bool isBlock = false;

	comment += currentChar;
	advance();

	if (currentChar == '*') {
		isBlock = true;
	}

	do {
		comment += currentChar;
		advance();
	} while (hasNext() && (isBlock && (currentChar != '/' || beforeChar != '*') || !isBlock && currentChar != '\n'));

	comment += currentChar;
	advance();

	return Token(TOKEN_TYPE::TOK_COMMENT, comment, currentRow, startCol);
}

Token Lexer::processString() {
	std::string str;
	TOKEN_TYPE type;

	do {
		str += currentChar;
		advance();
	} while (hasNext() && (currentChar != '"' || beforeChar == '\\'));
	str += currentChar;
	advance();

	return Token(TOKEN_TYPE::TOK_STRING_LITERAL, str, currentRow, startCol);
}

Token Lexer::processChar() {
	std::string chr;
	TOKEN_TYPE type;

	chr += currentChar;
	advance();
	if (hasNext() && currentChar == '\\') {
		chr += currentChar;
		advance();
	}
	chr += currentChar;
	advance();
	if (hasNext() && currentChar != '\'') {
		throw std::runtime_error(msgHeader() + "lexical error: expected \"'\" to close character constant.");
	}
	chr += currentChar;
	advance();

	return Token(TOKEN_TYPE::TOK_CHAR_LITERAL, chr, currentRow, startCol);
}

Token Lexer::processIdentifier() {
	std::string identifier;
	TOKEN_TYPE type;

	while (hasNext() && (std::isalnum(currentChar) || currentChar == '_' || currentChar == '.')) {
		identifier += currentChar;
		advance();
	}

	if (identifier == "using")
		type = TOK_USING;
	else if (identifier == "var")
		type = TOK_VAR;
	else if (identifier == "def")
		type = TOK_DEF;
	else if (identifier == "return")
		type = TOK_RETURN;
	else if (identifier == "if")
		type = TOK_IF;
	else if (identifier == "else")
		type = TOK_ELSE;
	else if (identifier == "while")
		type = TOK_WHILE;
	else if (identifier == "print")
		type = TOK_PRINT;
	else if (identifier == "read")
		type = TOK_READ;
	else if (identifier == "void")
		type = TOK_VOID_TYPE;
	else if (identifier == "bool")
		type = TOK_BOOL_TYPE;
	else if (identifier == "int")
		type = TOK_INT_TYPE;
	else if (identifier == "float")
		type = TOK_FLOAT_TYPE;
	else if (identifier == "char")
		type = TOK_CHAR_TYPE;
	else if (identifier == "string")
		type = TOK_STRING_TYPE;
	else if (identifier == "true" || identifier == "false")
		type = TOK_BOOL_LITERAL;
	else if (identifier == "and")
		type = TOK_LOGICAL_AND_OP;
	else if (identifier == "or")
		type = TOK_LOGICAL_OR_OP;
	else if (identifier == "not")
		type = TOK_NOT;
	else type = TOK_IDENTIFIER;

	return Token(type, identifier, currentRow, startCol);
}

Token Lexer::processNumber() {
	std::string number;
	TOKEN_TYPE type;
	bool hasDot = false;

	while (hasNext() && (std::isdigit(currentChar) || currentChar == '.')) {
		if (currentChar == '.') {
			if (hasDot){
				throw std::runtime_error(msgHeader() + "lexical error: found a double dot float.");
			}
			hasDot = true;
		}
		number += currentChar;
		advance();
	}

	if (hasDot) {
		type = TOK_FLOAT_LITERAL;
	}
	else if (currentChar == 'f') {
		type = TOK_FLOAT_LITERAL;
		advance();
	}
	else {
		type = TOK_INT_LITERAL;
	}

	return Token(type, number, currentRow, startCol);
}

Token Lexer::processSymbol() {
	char symbol;
	std::string strSymbol;
	TOKEN_TYPE type;
	symbol = currentChar;
	strSymbol = currentChar;
	advance();

	switch (symbol) {
	case '-':
	case '+':
		type = TOK_ADDITIVE_OP;
		break;

	case '*':
	case '/':
	case '%':
		type = TOK_MULTIPLICATIVE_OP;
		break;

	case '<':
	case '>':
		type = TOK_RELATIONAL_OP;
		break;

	case '=':
		type = TOK_EQUALS;
		break;

	case '{':
		type = TOK_LEFT_CURLY;
		break;

	case '}':
		type = TOK_RIGHT_CURLY;
		break;

	case '[':
		type = TOK_LEFT_BRACE;
		break;

	case ']':
		type = TOK_RIGHT_BRACE;
		break;

	case '(':
		type = TOK_LEFT_BRACKET;
		break;

	case ')':
		type = TOK_RIGHT_BRACKET;
		break;

	case ',':
		type = TOK_COMMA;
		break;

	case ';':
		type = TOK_SEMICOLON;
		break;

	case ':':
		type = TOK_COLON;
		break;

	default:
		type = TOK_ERROR;
	}

	return Token(type, strSymbol, currentRow, startCol);
}

Token Lexer::nextToken() {
	if (currentToken < tokens.size()) {
		return tokens[currentToken++];
	}
	else {
		std::string error = "Final token surpassed.";
		return Token(TOK_ERROR, error);
	}
}

std::string Lexer::msgHeader() {
	return name + '[' + std::to_string(currentRow) + ':' + std::to_string(currentCol) + "]: ";
}

Lexer::~Lexer() = default;
