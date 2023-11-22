#include <stack>
#include <stdexcept>

#include "lexer.h"


using namespace lexer;


Lexer::Lexer(std::string& program) {
	unsigned int currentIndex = 0;

	// tokenise the program, ignoring comments
	Token t;
	while (currentIndex <= program.length()) {
		t = nextToken(program, currentIndex);
		if (t.type != TOK_COMMENT) {
			tokens.push_back(t);
		}
	}
}

Token Lexer::nextToken() {
	if (currentToken < tokens.size()) {
		return tokens[currentToken++];
	}
	else {
		std::string error = "Final token surpassed.";
		return Token(TOK_ERROR, error);
	}
}

int Lexer::transitionDelta(int s, char sigma) {
	// check which transition type we have, and then refer to the transition table
	switch (sigma) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return transitions[DIGIT][s];

	case '.':
		return transitions[PERIOD][s];

	case '+':
	case '-':
		return transitions[ADDITIVE_OP][s];

	case '*':
		return transitions[ASTERISK][s];

	case '%':
		return transitions[MODULUS][s];

	case '!':
		return transitions[EXCL_MARK][s];

	case '>':
	case '<':
		return transitions[ORDER_REL][s];

	case '=':
		return transitions[EQUALS][s];

	case '_':
		return transitions[UNDERSCORE][s];

	case '/':
		return transitions[FORWARDSLASH][s];

	case '\\':
		return transitions[BACKSLASH][s];

	case '\"':
		return transitions[QUOTATION_MARK][s];

	case ':':
	case ';':
	case ',':
	case '(':
	case ')':
	case '{':
	case '}':
		return transitions[PUNCTUATION][s];

	case '\t':
	case '\n':
	case '\r':
		return transitions[NEWLINE][s];

	case EOF:
		return transitions[ENDOFFILE][s];

	default:
		auto ascii = (int)sigma;

		// [A-Z] or [a-z]
		if (((0x41 <= ascii) && (ascii <= 0x5A)) ||
			((0x61 <= ascii) && (ascii <= 0x7A))) {
			return transitions[LETTER][s];
		}

		// printable
		if ((0x20 <= ascii) && (ascii <= 0x7E)) {
			return transitions[PRINTABLE][s];
		}

		// other
		return transitions[OTHER][s];
	}
}

Token Lexer::nextToken(std::string& program, unsigned int& currentIndex) {
	// setup stack and lexeme
	int currentState = 0;
	std::stack<int> stateStack;
	char currentSymbol;
	std::string lexeme;

	// push 'BAD' state on the stack
	stateStack.push(-1);

	// ignore whitespaces or newlines in front of lexeme
	while (currentIndex < program.length() && (program[currentIndex] == ' ' || program[currentIndex] == '\n' || program[currentIndex] == '\t')) {
		currentIndex++;
	}

	// check if EOF
	if (currentIndex == program.length()) {
		lexeme = (char)EOF;
		currentIndex++;
		return Token(22, lexeme, getLineNumber(program, currentIndex));
	}

	// while current state is not error state
	while (currentState != e) {
		currentSymbol = program[currentIndex];
		lexeme += currentSymbol;

		// if current state is final, remove previously recorded final states
		if (isFinal[currentState]) {
			while (!stateStack.empty()) {
				stateStack.pop();
			}
		}

		// and push current one on the stack
		stateStack.push(currentState);

		// go to next state using delta function in DFA
		currentState = transitionDelta(currentState, currentSymbol);

		// update current index for next iteration
		currentIndex++;
	}

	// rollback loop
	while (currentState != -1 && !isFinal[currentState]) {
		currentState = stateStack.top();
		stateStack.pop();
		lexeme.pop_back();
		currentIndex--;
	}

	if (currentState == -1) {
		throw std::runtime_error("Lexical error.");
	}

	if (isFinal[currentState]) {
		return Token(currentState, std::move(lexeme), getLineNumber(program, currentIndex));
	}
	else {
		throw std::runtime_error("Lexical error on line " + std::to_string(getLineNumber(program, currentIndex)) + ".");
	}
}

unsigned int Lexer::getLineNumber(std::string& program, unsigned int index) {
	unsigned int line = 1;
	for (int i = 0; i < index; i++) {
		if (program[i] == '\n') {
			line++;
		}
	}
	return line;
}

Lexer::~Lexer() = default;
