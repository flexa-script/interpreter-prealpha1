#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "cpsource.hpp"

class CPInterpreter {
private:
	std::string root;
	std::vector<std::string> files;

public:
	CPInterpreter(const std::string& root, std::vector<std::string>&& files);

	int execute();

private:
	std::vector<CPSource> load_programs(const std::string& root, const std::vector<std::string>& files);

	void parse_programs(const std::vector<CPSource>& source_programs, ASTProgramNode** main_program,
		std::map<std::string, ASTProgramNode*>* programs);

	int interpreter(const std::vector<CPSource>& source_programs);

};

#endif // !CPINTERPRETER_HPP
