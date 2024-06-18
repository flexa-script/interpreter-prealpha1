#include "token.hpp"


using namespace lexer;


Token::Token(TokenType type, const std::string& value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

Token::Token()
	: type(TokenType::TOK_ERROR), value(""), row(0), col(0) {}

const std::string& Token::token_image(TokenType type) {
	return TOKEN_IMAGE[type];
}
