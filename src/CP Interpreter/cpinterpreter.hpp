#ifndef CPINTERPRETER_HPP
#define CPINTERPRETER_HPP


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

	void parse_programs(std::vector<Program>, parser::ASTProgramNode*,
		std::map<std::string, parser::ASTProgramNode*>*);

	int interpreter(std::vector<Program>);

};

#endif // CPINTERPRETER_HPP
