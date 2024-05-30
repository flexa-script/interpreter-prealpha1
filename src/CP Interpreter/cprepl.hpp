#ifndef CPREPL_HPP
#define CPREPL_HPP

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

#include "lexer.hpp"
#include "parser.hpp"
#include "semantic_analysis.hpp"
#include "interpreter.hpp"


#ifdef __unix__
#define clear_screen() system("clear")
#elif defined(_WIN32) || defined(WIN32)
#define clear_screen() system("cls")
#endif // __unix__

class CPRepl {
public:
	static int execute();
};

#endif // !CPREPL_HPP
