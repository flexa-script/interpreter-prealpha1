#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>


namespace lexer {

	enum TokenType {
		TOK_BOOL_LITERAL,
		TOK_INT_LITERAL,
		TOK_FLOAT_LITERAL,
		TOK_CHAR_LITERAL,
		TOK_STRING_LITERAL,
		TOK_ADDITIVE_OP,
		TOK_MULTIPLICATIVE_OP,
		TOK_RELATIONAL_OP,
		TOK_EQUALS,
		TOK_LOGICAL_AND_OP,
		TOK_LOGICAL_OR_OP,
		TOK_NOT,
		TOK_VOID_TYPE,
		TOK_BOOL_TYPE,
		TOK_INT_TYPE,
		TOK_FLOAT_TYPE,
		TOK_CHAR_TYPE,
		TOK_STRING_TYPE,
		TOK_ANY_TYPE,
		TOK_USING,
		TOK_CONST,
		TOK_VAR,
		TOK_STRUCT,
		TOK_DEF,
		TOK_NULL,
		TOK_THIS,
		TOK_RETURN,
		TOK_BREAK,
		TOK_SWITCH,
		TOK_CASE,
		TOK_DEFAULT,
		TOK_IF,
		TOK_ELSE,
		TOK_FOR,
		TOK_FOREACH,
		TOK_WHILE,
		TOK_PRINT,
		TOK_READ,
		TOK_LEN,
		TOK_TYPE,
		TOK_ROUND,
		TOK_IDENTIFIER,
		TOK_COMMENT,
		TOK_LEFT_CURLY,
		TOK_RIGHT_CURLY,
		TOK_LEFT_BRACKET,
		TOK_RIGHT_BRACKET,
		TOK_LEFT_BRACE,
		TOK_RIGHT_BRACE,
		TOK_COMMA,
		TOK_SEMICOLON,
		TOK_COLON,
		TOK_EOF,
		TOK_ERROR
	};

	const std::string TOKEN_IMAGE[] = {
		"bool literal",
		"int literal",
		"float literal",
		"char literal",
		"string literal",
		"additive operator",
		"multiplicative operator",
		"relational operator",
		"=",
		"and",
		"or",
		"not",
		"void",
		"bool",
		"int",
		"float",
		"char",
		"string",
		"any",
		"using",
		"const",
		"var",
		"sruct",
		"def",
		"null",
		"this",
		"return",
		"break",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"for",
		"foreach",
		"while",
		"print",
		"read",
		"len",
		"type",
		"round",
		"identifier",
		"",
		"{",
		"}",
		"(",
		")",
		"[",
		"]",
		",",
		";",
		":",
		"error token",
		"EOF token"
	};

	const TokenType TYPE_TOKENS[] = {
		TOK_VOID_TYPE,
		TOK_BOOL_TYPE,
		TOK_INT_TYPE,
		TOK_FLOAT_TYPE,
		TOK_CHAR_TYPE,
		TOK_STRING_TYPE,
		TOK_ANY_TYPE
	};

	class Token {
	public:
		TokenType type;
		std::string value;
		unsigned int row;
		unsigned int col;

		Token();
		Token(TokenType, std::string, unsigned int row = 0, unsigned int col = 0);

		bool is_type();
		static std::string token_image(const TokenType);
	};
};

#endif // TOKEN_HPP
