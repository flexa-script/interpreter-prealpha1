#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "cpsource.hpp"

class CPInterpreter {
public:
	int execute(int, const char*[]);

	std::vector<CPSource> load_programs(std::vector<std::string>);
	std::vector<CPSource> load_cp_libs(std::vector<std::string>);

	void parse_programs(std::vector<CPSource>, parser::ASTProgramNode**,
		std::map<std::string, parser::ASTProgramNode*>*);

	int interpreter(std::vector<CPSource>);

};

#endif // CPINTERPRETER_HPP
