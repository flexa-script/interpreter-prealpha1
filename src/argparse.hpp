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

void throw_if_not_parameter(int argc, size_t i, std::string parameter) {
	if (i >= argc) {
		throw std::runtime_error("expected value after " + parameter);
	}
}

CpCliArgs parse_args(int argc, const char* argv[]) {
	CpCliArgs args;
	args.debug = false;
	args.engine = "ast";
	args.workspace = "";
	
	args.sources;

	size_t i = 0;

	// parse optional parameters
	while (++i < argc) {
		if (argv[i] == "-d" || argv[i] == "--debug") {
			args.debug = true;
			continue;
		}
		if (argv[i] == "-e" || argv[i] == "--engine") {
			++i;
			throw_if_not_parameter(argc, i, argv[i - 1]);
			args.engine = argv[i];
			continue;
		}
		if (argv[i] == "-w" || argv[i] == "--workspace") {
			++i;
			throw_if_not_parameter(argc, i, argv[i - 1]);
			args.workspace = argv[i];
			continue;
		}
		if (argv[i] == "-s" || argv[i] == "--source") {
			++i;
			throw_if_not_parameter(argc, i, argv[i - 1]);
			args.sources.push_back(argv[i]);
			continue;
		}
		
		// no more optional parameters
		break;
	}
	
	// parse cp arguments
	while (++i < argc) {
		args.sources.push_back(argv[i]);
	}
}

#endif // !ARGPARSE_HPP
