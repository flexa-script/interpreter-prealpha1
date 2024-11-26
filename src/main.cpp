#include "cprepl.hpp"
#include "cpinterpreter.hpp"
#include "vendor/axewatch.hpp"

#include <Windows.h>

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

int main(int argc, const char* argv[]) {
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	bool debug = false;
	std::string engine = "ast";
	int result = 0;

	SetConsoleOutputCP(CP_UTF8);
	
	if (argc == 1) {
		return CPRepl::execute();
	}

	std::vector<std::string> files;
	std::string project_root = "";
	size_t i = 0;
	while (++i < argc) {
		if (argv[i] == "-d" || argv[i] == "--debug") {
			debug = true;
			continue;
		}
		if (argv[i] == "-e" || argv[i] == "--engine") {
			++i;
			if (i >= argc) {
				throw std::runtime_error("invalid engine command operand");
			}
			engine = argv[i];
			continue;
		}
		if (project_root == "") {
			project_root = argv[i];
			continue;
		}
		files.push_back(argv[i]);
	}

	auto interpreter = CPInterpreter(project_root, std::move(files), debug, engine);

	auto sw = axe::ChronoStopwatch();
	sw.start();
	result = interpreter.execute();
	sw.stop();

	if (debug) {
		std::cout << std::endl << "execution time: " << sw.get_elapsed_formatted() << std::endl;
		std::cout << "process finished with exit code " << result << std::endl;
		system("pause");
	}

	return result;
}
