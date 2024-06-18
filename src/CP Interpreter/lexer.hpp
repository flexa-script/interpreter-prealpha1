#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

#include "token.hpp"


namespace lexer {

    class Lexer {

    public:
        Lexer(const std::string& name, const std::string& source);
        ~Lexer();

        Token next_token();

    private:
        char before_char;
        char current_char;
        char next_char;
        unsigned int current_token = 0;
        unsigned int current_index = 0;
        unsigned int current_row = 0;
        unsigned int start_col = 0;
        unsigned int current_col = 0;
        std::string source;
        std::string name;
        std::vector<Token> tokens;

        void tokenize();
        bool has_next();
        bool is_space();
        void advance();
        Token process_identifier();
        Token process_number();
        Token process_char();
        Token process_string();
        Token process_symbol();
        Token process_comment();

        std::string msg_header();
    };
};

#endif // !LEXER_HPP
