#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "token.h"


namespace lexer {

    class Lexer {

    public:
        Lexer(std::string&);
        Token nextToken();
        ~Lexer();

    private:
        unsigned int currentToken = 0;
        unsigned int currentIndex = 0;
        std::string source;
        std::vector<Token> tokens;

        void tokenize();
        void advance();
        Token processIdentifier();
        Token processNumber();
        Token processChar();
        Token processString();
        Token processSymbol();

        unsigned int getLineNumber();
    };
};

#endif //LEXER_H
