#include "token.hpp"
//#include "util.hpp"


using namespace lexer;


Token::Token() = default;

Token::Token(TOKEN_TYPE type, std::string value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

bool Token::isType() {
	return std::find(std::begin(TYPE_TOKENS), std::end(TYPE_TOKENS), type) != std::end(TYPE_TOKENS);// axe::ccontains(TYPE_TOKENS, type);
}

std::string Token::tokenImage(const TOKEN_TYPE type) {
	return TOKEN_IMAGE[type];
}
