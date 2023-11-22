#ifndef TOKEN_H
#define TOKEN_H

#include <string>


namespace lexer {

	enum TOKEN {
		TOK_INT,
		TOK_FLOAT,
		TOK_ADDITIVE_OP,
		TOK_MULTIPLICATIVE_OP,
		TOK_RELATIONAL_OP,
		TOK_LOGICAL_AND_OP,
		TOK_LOGICAL_OR_OP,
		TOK_EQUALS,
		TOK_USING,
		TOK_VAR,
		TOK_DEF,
		TOK_RETURN,
		TOK_IF,
		TOK_ELSE,
		TOK_WHILE,
		TOK_PRINT,
		TOK_READ,
		TOK_VOID_TYPE,
		TOK_INT_TYPE,
		TOK_FLOAT_TYPE,
		TOK_BOOL_TYPE,
		TOK_STRING_TYPE,
		TOK_BOOL,
		TOK_NOT,
		TOK_IDENTIFIER,
		TOK_COMMENT,
		TOK_STRING,
		TOK_LEFT_CURLY,
		TOK_RIGHT_CURLY,
		TOK_LEFT_BRACKET,
		TOK_RIGHT_BRACKET,
		TOK_COMMA,
		TOK_SEMICOLON,
		TOK_COLON,
		TOK_EOF,
		TOK_ERROR
	};

	class Token {
	public:
		Token();

		Token(int, std::string, unsigned int lineNumber = 0);

		TOKEN type;
		std::string value;
		unsigned int lineNumber;

	private:
		TOKEN determineTokenType(int, std::string&);
	};
};

#endif //TOKEN_H
