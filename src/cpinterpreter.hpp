#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP

#include "cpsource.hpp"
#include "semantic_analysis.hpp"

class CPInterpreter {
private:
	std::string project_root;
	std::string cp_root;
	std::vector<std::string> files;
	bool debug;
	std::string engine;

public:
	CPInterpreter(const std::string& project_root, std::vector<std::string>&& files, bool debug, const std::string& engine);

	int execute();

private:
	std::vector<CPSource> load_programs(const std::vector<std::string>& files);

	void parse_programs(const std::vector<CPSource>& source_programs, ASTProgramNode** main_program,
		std::map<std::string, ASTProgramNode*>* programs);

	int interpreter();

};

#endif // !CPINTERPRETER_HPP
