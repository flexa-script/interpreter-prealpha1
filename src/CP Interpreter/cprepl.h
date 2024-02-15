#ifndef CPREPL_H
#define CPREPL_H

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "interpreter.h"


#ifdef __unix__
#define clear_screen() system("clear")
#elif defined(_WIN32) || defined(WIN32)
#define clear_screen() system("cls")
#endif

class CPRepl {
public:
	static int execute();
};

#endif//CPREPL_H
