#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "token.h"


namespace lexer {

    class Lexer {

    public:
        Lexer(std::string&, std::string);
        Token nextToken();
        ~Lexer();

    private:
        char beforeChar;
        char currentChar;
        unsigned int currentToken = 0;
        unsigned int currentIndex = 0;
        unsigned int currentRow = 0;
        unsigned int startCol = 0;
        unsigned int currentCol = 0;
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
