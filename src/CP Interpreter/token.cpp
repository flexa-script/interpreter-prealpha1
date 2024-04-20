#include "token.h"

#include "util.h"


using namespace lexer;


Token::Token() = default;

Token::Token(TOKEN_TYPE type, std::string value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

bool Token::isType() {
	return std::find(std::begin(TYPE_TOKENS), std::end(TYPE_TOKENS), type) != std::end(TYPE_TOKENS);// axe::ccontains(TYPE_TOKENS, type);
}
