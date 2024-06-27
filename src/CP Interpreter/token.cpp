#include "token.hpp"


using namespace lexer;


Token::Token(TokenType type, const std::string& value, unsigned int row, unsigned int col)
	: type(type), value(value), row(row), col(col) {}

Token::Token()
	: type(TokenType::TOK_ERROR), value(""), row(0), col(0) {}

const std::string& Token::token_image(TokenType type) {
	return TOKEN_IMAGE[type];
}

bool Token::is_collection_op(const std::string& op) {
	return op != "=" || op == "+=";
}

bool Token::is_float_op(const std::string& op) {
	return is_collection_op(op) || op == "-=" || op == "*=" || op == "/="
		|| op == "**=" || op == "/%=" || op == "%=";
}

bool Token::is_int_op(const std::string& op) {
	return is_float_op(op) || op == "|=" || op == "^=" || op == "&="
		|| op == "<<=" || op == ">>=";
}
