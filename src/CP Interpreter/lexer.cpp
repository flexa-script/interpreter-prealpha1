#include <stack>
#include <stdexcept>

#include "lexer.hpp"


using namespace lexer;


Lexer::Lexer(std::string& source, std::string name) 
	: source(source), name(name) {
	tokenize();
}

Lexer::~Lexer() = default;

void Lexer::tokenize() {
	current_row = 1;
	current_col = 1;

	while (has_next()) {
		current_char = source[current_index];

		if (current_char == '\n') {
			++current_row;
		}

		if (is_space()) {
			// ignore white spaces
			advance();
		}
		else if (current_char == '/' && (get_next_char() == '/' || get_next_char() == '*')) {
			process_comment();
		}
		else if (current_char == '\'') {
			start_col = current_col;
			tokens.push_back(process_char());
		}
		else if (current_char == '"') {
			start_col = current_col;
			tokens.push_back(process_string());
		}
		else if (std::isalpha(current_char) || current_char == '_') {
			start_col = current_col;
			tokens.push_back(process_identifier());
		}
		else if (std::isdigit(current_char) || current_char == '.') {
			start_col = current_col;
			tokens.push_back(process_number());
		}
		else {
			start_col = current_col;
			tokens.push_back(process_symbol());
		}
	}

	tokens.push_back(Token(TokenType::TOK_EOF, "EOF", current_col, current_row));
}

char Lexer::get_next_char() {
	return current_index + 1 < source.length() ? source[current_index + 1] : -1;
}

bool Lexer::has_next() {
	return current_index < source.length();
}

bool Lexer::is_space() {
	return std::isspace(current_char) || current_char == '\t' || current_char == '\r' || current_char == '\n';
}

void Lexer::advance() {
	if (current_char == '\n') {
		current_col = 1;
	}
	else {
		++current_col;
	}
	before_char = current_char;
	current_char = source[++current_index];
}

Token Lexer::process_comment() {
	std::string comment;
	bool is_block = false;
	int row = current_row;
	int col = current_col;

	comment += current_char;
	advance();

	if (current_char == '*') {
		is_block = true;
	}

	do {
		if (current_char == '\n') {
			++current_row;
		}

		comment += current_char;
		advance();
	} while (has_next() && (is_block && (current_char != '/' || before_char != '*') || !is_block && current_char != '\n'));


	if (current_char == '\n') {
		++current_row;
	}

	comment += current_char;
	advance();

	return Token(TokenType::TOK_COMMENT, comment, row, col);
}

Token Lexer::process_string() {
	std::string str;
	bool spec = false;

	str += current_char;
	advance();

	do {
		if (!spec) {
			if (current_char == '\\') {
				spec = true;
				str += current_char;
			}
			else if (current_char == '"') {
				break;
			}
			else {
				str += current_char;
			}
		}
		else {
			str += current_char;
			spec = false;
		}
		advance();
	} while (has_next());

	str += current_char;
	advance();

	return Token(TokenType::TOK_STRING_LITERAL, str, current_row, start_col);
}

Token Lexer::process_char() {
	std::string chr;

	chr += current_char;
	advance();
	if (has_next() && current_char == '\\') {
		chr += current_char;
		advance();
	}
	chr += current_char;
	advance();
	if (has_next() && current_char != '\'') {
		throw std::runtime_error(msg_header() + "lexical error: expected \"'\" to close character constant.");
	}
	chr += current_char;
	advance();

	return Token(TokenType::TOK_CHAR_LITERAL, chr, current_row, start_col);
}

Token Lexer::process_identifier() {
	std::string identifier;
	TokenType type;

	while (has_next() && (std::isalnum(current_char) || current_char == '_' || current_char == '.')) {
		identifier += current_char;
		advance();
	}

	if (identifier == "using")
		type = TOK_USING;
	else if (identifier == "const")
		type = TOK_CONST;
	else if (identifier == "var")
		type = TOK_VAR;
	else if (identifier == "struct")
		type = TOK_STRUCT;
	else if (identifier == "def")
		type = TOK_DEF;
	else if (identifier == "this")
		type = TOK_THIS;
	else if (identifier == "return")
		type = TOK_RETURN;
	else if (identifier == "break")
		type = TOK_BREAK;
	else if (identifier == "switch")
		type = TOK_SWITCH;
	else if (identifier == "default")
		type = TOK_DEFAULT;
	else if (identifier == "case")
		type = TOK_CASE;
	else if (identifier == "in")
		type = TOK_IN;
	else if (identifier == "if")
		type = TOK_IF;
	else if (identifier == "else")
		type = TOK_ELSE;
	else if (identifier == "for")
		type = TOK_FOR;
	else if (identifier == "foreach")
		type = TOK_FOREACH;
	else if (identifier == "while")
		type = TOK_WHILE;
	else if (identifier == "print")
		type = TOK_PRINT;
	else if (identifier == "read")
		type = TOK_READ;
	else if (identifier == "len")
		type = TOK_LEN;
	else if (identifier == "type")
		type = TOK_TYPE;
	else if (identifier == "round")
		type = TOK_ROUND;
	else if (identifier == "void")
		type = TOK_VOID_TYPE;
	else if (identifier == "bool")
		type = TOK_BOOL_TYPE;
	else if (identifier == "int")
		type = TOK_INT_TYPE;
	else if (identifier == "float")
		type = TOK_FLOAT_TYPE;
	else if (identifier == "char")
		type = TOK_CHAR_TYPE;
	else if (identifier == "string")
		type = TOK_STRING_TYPE;
	else if (identifier == "any")
		type = TOK_ANY_TYPE;
	else if (identifier == "true" || identifier == "false")
		type = TOK_BOOL_LITERAL;
	else if (identifier == "null")
		type = TOK_NULL;
	else if (identifier == "and")
		type = TOK_LOGICAL_AND_OP;
	else if (identifier == "or")
		type = TOK_LOGICAL_OR_OP;
	else if (identifier == "not")
		type = TOK_NOT;
	else type = TOK_IDENTIFIER;

	return Token(type, identifier, current_row, start_col);
}

Token Lexer::process_number() {
	std::string number;
	TokenType type;
	bool has_dot = false;

	while (has_next() && (std::isdigit(current_char) || current_char == '.')) {
		if (current_char == '.') {
			if (has_dot){
				throw std::runtime_error(msg_header() + "found '" + current_char + "' defining float");
			}
			has_dot = true;
		}
		number += current_char;
		advance();
	}

	if (has_dot) {
		type = TOK_FLOAT_LITERAL;
	}
	else if (current_char == 'f') {
		type = TOK_FLOAT_LITERAL;
		advance();
	}
	else {
		type = TOK_INT_LITERAL;
	}

	return Token(type, number, current_row, start_col);
}

Token Lexer::process_symbol() {
	char symbol;
	std::string str_symbol;
	TokenType type;
	symbol = current_char;
	str_symbol = current_char;
	advance();

	switch (symbol) {
	case '-':
	case '+':
		type = TOK_ADDITIVE_OP;
		break;

	case '*':
	case '/':
	case '%':
		type = TOK_MULTIPLICATIVE_OP;
		break;

	case '<':
	case '>':
		if (current_char == '=') {
			str_symbol += current_char;
			advance();
		}
		type = TOK_RELATIONAL_OP;
		break;

	case '=':
		if (current_char == '=') {
			str_symbol += current_char;
			advance();
			type = TOK_RELATIONAL_OP;
		}
		else {
			type = TOK_EQUALS;
		}
		break;

	case '!':
		if (current_char != '=') {
			throw std::runtime_error(msg_header() + "lexical error: expected '!'.");
		}
		str_symbol += current_char;
		advance();
		type = TOK_RELATIONAL_OP;
		break;

	case '{':
		type = TOK_LEFT_CURLY;
		break;

	case '}':
		type = TOK_RIGHT_CURLY;
		break;

	case '[':
		type = TOK_LEFT_BRACE;
		break;

	case ']':
		type = TOK_RIGHT_BRACE;
		break;

	case '(':
		type = TOK_LEFT_BRACKET;
		break;

	case ')':
		type = TOK_RIGHT_BRACKET;
		break;

	case ',':
		type = TOK_COMMA;
		break;

	case ';':
		type = TOK_SEMICOLON;
		break;

	case ':':
		type = TOK_COLON;
		break;

	default:
		type = TOK_ERROR;
	}

	return Token(type, str_symbol, current_row, start_col);
}

Token Lexer::next_token() {
	if (current_token < tokens.size()) {
		return tokens[current_token++];
	}
	else {
		std::string error = "Final token surpassed.";
		return Token(TOK_ERROR, error);
	}
}

std::string Lexer::msg_header() {
	return "(LERR) " + name + '[' + std::to_string(current_row) + ':' + std::to_string(current_col) + "]: ";
}
