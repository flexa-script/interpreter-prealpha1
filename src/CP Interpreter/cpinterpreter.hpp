#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "cpsource.hpp"

class CPInterpreter {
private:
	std::string project_root;
	std::string cp_root;
	std::vector<std::string> files;

public:
	CPInterpreter(const std::string& project_root, std::vector<std::string>&& files);

	int execute();

private:
	std::vector<CPSource> load_programs(const std::vector<std::string>& files);

	void parse_programs(const std::vector<CPSource>& source_programs, ASTProgramNode** main_program,
		std::map<std::string, ASTProgramNode*>* programs);

	int interpreter();

};

#endif // !CPINTERPRETER_HPP
