#include "token.h"


using namespace lexer;


Token::Token() = default;

Token::Token(TOKEN_TYPE type, std::string value, unsigned long long row, unsigned long long col)
	: type(type), value(value), row(row), col(col) {}
