#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "argparse.hpp"
#include "cpsource.hpp"
#include "semantic_analysis.hpp"

class CPInterpreter {
private:
	std::string cp_root;
	std::string project_root;
	CpCliArgs args;

public:
	CPInterpreter(const CpCliArgs& args);

	int execute();

private:
	std::vector<CPSource> load_programs(const std::vector<std::string>& sources);

	void parse_programs(const std::vector<CPSource>& source_programs, std::shared_ptr<ASTProgramNode>* main_program,
		std::map<std::string, std::shared_ptr<ASTProgramNode>>* programs);

	int interpreter();

};

#endif // !CPINTERPRETER_HPP
