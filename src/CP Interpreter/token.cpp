#include "token.hpp"


using namespace lexer;


Token::Token() = default;

Token::Token(TokenType type, std::string value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

bool Token::is_type() {
	return std::find(std::begin(TYPE_TOKENS), std::end(TYPE_TOKENS), type) != std::end(TYPE_TOKENS);
}

std::string Token::token_image(const TokenType type) {
	return TOKEN_IMAGE[type];
}
