#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

#include "lexer.hpp"
#include "parser.hpp"
#include "semantic_analysis.hpp"
#include "interpreter.hpp"
#include "util.hpp"
#include "cputil.hpp"


typedef struct Program {
public:
	std::string name;
	std::string source;
	Program(std::string name, std::string source) : name(name), source(source) {}
} Program;

class CPInterpreter {
public:
	int execute(int, const char*[]);

	std::vector<Program> load_programs(std::vector<std::string>);

	int interpreter(std::vector<Program>);

	std::vector<Program> debug_programs();
};

#endif // CPINTERPRETER_HPP
