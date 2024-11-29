#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <iostream>
#include <vector>

struct CpCliArgs {
	bool debug;
	std::string engine;
	std::string workspace;
	std::vector<std::string> sources;
	std::vector<std::string> cpargs;
};

extern void throw_if_not_parameter(int argc, size_t i, std::string parameter);

extern CpCliArgs parse_args(int argc, const char* argv[]);

#endif // !ARGPARSE_HPP
