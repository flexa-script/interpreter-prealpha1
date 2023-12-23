#ifndef CPINTERPRETER_H
#define CPINTERPRETER_H

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "interpreter.h"
#include "util.h"


typedef struct Program {
public:
	std::string name;
	std::string source;
	Program(std::string name, std::string source) : name(name), source(source) {}
} Program;

class CPInterpreter {
public:
	int execute(int, const char*[]);

	Program loadSource(std::string, std::string);

	std::vector<Program> loadPrograms(std::string, std::string, std::vector<std::string>);

	int interpreter(std::vector<Program>);

	std::vector<Program> debugPrograms();
};

#endif//CPINTERPRETER_H
