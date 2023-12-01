#include "token.h"


using namespace lexer;


Token::Token() = default;

Token::Token(TOKEN type, std::string value, unsigned int lineNumber) :
	type(type),
	value(value),
	lineNumber(lineNumber) {}
