#include "argparse.hpp"

#include <iostream>
#include <vector>

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
		std::string arg = argv[i];
		if (arg == "-d" || arg == "--debug") {
			args.debug = true;

			continue;
		}
		if (arg == "-e" || arg == "--engine") {
			++i;
			throw_if_not_parameter(argc, i, arg);
			std::string p = argv[i];
			if (p != "ast" && p != "vm") {
				throw std::runtime_error("invalid " + arg + " parameter value: '" + p + "'");
			}
			args.engine = argv[i];
			continue;
		}
		if (arg == "-w" || arg == "--workspace") {
			++i;
			throw_if_not_parameter(argc, i, arg);
			args.workspace = argv[i];
			continue;
		}
		if (arg == "-s" || arg == "--source") {
			++i;
			throw_if_not_parameter(argc, i, arg);
			args.sources.push_back(argv[i]);
			continue;
		}
		--i;
		// no more optional parameters
		break;
	}
	
	// parse cp arguments
	while (++i < argc) {
		args.cpargs.push_back(argv[i]);
	}

	return args;
}
