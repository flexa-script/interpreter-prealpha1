#include "token.hpp"


using namespace lexer;


Token::Token() = default;

Token::Token(TokenType type, std::string value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

std::string Token::token_image(const TokenType type) {
	return TOKEN_IMAGE[type];
}
