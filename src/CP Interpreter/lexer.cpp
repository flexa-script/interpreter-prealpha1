#include <stack>
#include <stdexcept>

#include "lexer.h"


using namespace lexer;


Lexer::Lexer(std::string& source) 
	: source(source) {
	tokenize();
}

void Lexer::tokenize() {
	while (currentIndex < source.length()) {
		char currentChar = source[currentIndex];

		if (std::isspace(currentChar)) {
			// ignore white spaces
			advance();
		}
		else if (currentChar == '\'') {
			// character
			tokens.push_back(processChar());
		}
		else if (currentChar == '"') {
			// string
			tokens.push_back(processString());
		}
		else if (std::isalpha(currentChar) || currentChar == '_') {
			// identifier and reserved words
			tokens.push_back(processIdentifier());
		}
		else if (std::isdigit(currentChar) || currentChar == '.') {
			// number
			tokens.push_back(processNumber());
		}
		else {
			// symbol
			tokens.push_back(processSymbol());
		}
	}

	tokens.push_back(Token(TOKEN::TOK_EOF, "", getLineNumber()));
}

void Lexer::advance() {
	currentIndex++;
}

Token Lexer::processString() {
	std::string str;
	TOKEN type;

	do {
		str += source[currentIndex];
		advance();
	} while (source[currentIndex] != '"' || source[currentIndex - 1] == '\\');
	str += source[currentIndex];
	advance();

	return Token(TOKEN::TOK_STRING_LITERAL, str, getLineNumber());
}

Token Lexer::processChar() {
	std::string chr;
	TOKEN type;

	chr += source[currentIndex];
	advance();
	if (source[currentIndex] == '\\') {
		chr += source[currentIndex];
		advance();
	}
	chr += source[currentIndex];
	advance();
	if (source[currentIndex] != '\'') {
		throw std::runtime_error("Lexical error on line " + std::to_string(getLineNumber()) + ", expected \"'\" to close character constant.");
	}
	chr += source[currentIndex];
	advance();

	return Token(TOKEN::TOK_CHAR_LITERAL, chr, getLineNumber());
}

Token Lexer::processIdentifier() {
	std::string identifier;
	TOKEN type;

	while (std::isalnum(source[currentIndex]) || source[currentIndex] == '_' || source[currentIndex] == '.') {
		identifier += source[currentIndex];
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

	return Token(type, identifier, getLineNumber());
}

Token Lexer::processNumber() {
	std::string number;
	TOKEN type;
	bool hasDot = false;

	while (std::isdigit(source[currentIndex]) || source[currentIndex] == '.') {
		if (source[currentIndex] == '.') {
			if (hasDot){
				throw std::runtime_error("Lexical error on line " + std::to_string(getLineNumber()) + ", found a double dot number.");
			}
			hasDot = true;
		}
		number += source[currentIndex];
		advance();
	}

	if (hasDot) {
		type = TOK_FLOAT_LITERAL;
	}
	else {
		type = TOK_INT_LITERAL;
	}

	return Token(type, number, getLineNumber());
}

Token Lexer::processSymbol() {
	char symbol;
	std::string strSymbol;
	TOKEN type;
	symbol = source[currentIndex];
	strSymbol = source[currentIndex];
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

	return Token(type, strSymbol, getLineNumber());
}

unsigned int Lexer::getLineNumber() {
	unsigned int line = 1;
	for (int i = 0; i < currentIndex; i++) {
		if (source[i] == '\n') {
			line++;
		}
	}
	return line;
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

Lexer::~Lexer() = default;
