#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "bsl_utils.hpp"
#include "semantic_analysis.hpp"

class CPInterpreter {
private:
	std::string cp_root;
	std::string project_root;
	BSLCliArgs args;

public:
	CPInterpreter(const BSLCliArgs& args);

	int execute();

private:
	BSLSource load_program(const std::string& source);
	std::vector<BSLSource> load_programs(const std::vector<std::string>& sources);

	void parse_programs(const std::vector<BSLSource>& source_programs, std::shared_ptr<ASTProgramNode>* main_program,
		std::map<std::string, std::shared_ptr<ASTProgramNode>>* programs);

	int interpreter();

};

#endif // !CPINTERPRETER_HPP
