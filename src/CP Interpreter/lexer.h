#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "token.h"


namespace lexer {

    class Lexer {

    public:
        Lexer(std::string&, std::string);
        ~Lexer();

        Token nextToken();

    private:
        char beforeChar;
        char currentChar;
        unsigned long long currentToken = 0;
        unsigned long long currentIndex = 0;
        unsigned long long currentRow = 0;
        unsigned long long startCol = 0;
        unsigned long long currentCol = 0;
        std::string source;
        std::string name;
        std::vector<Token> tokens;

        void tokenize();
        bool hasNext();
        char getNextChar();
        bool isSpace();
        void advance();
        Token processIdentifier();
        Token processNumber();
        Token processChar();
        Token processString();
        Token processSymbol();
        Token processComment();

        std::string msgHeader();
    };
};

#endif //LEXER_H
