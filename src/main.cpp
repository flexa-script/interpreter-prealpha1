#include "cprepl.hpp"
#include "cpinterpreter.hpp"
#include "vendor/axewatch.hpp"
#include "argparse.hpp"

#include <Windows.h>

int main(int argc, const char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto args = parse_args(argc, argv);

	if (args.debug) {
#ifndef __CP_DEBUG__
#define __CP_DEBUG__
#endif // !__CP_DEBUG__
	}

	auto interpreter = CPInterpreter(args);

	auto sw = axe::ChronoStopwatch();
	sw.start();
	int result = interpreter.execute();
	sw.stop();

	if (args.debug) {
		std::cout << std::endl << "execution time: " << sw.get_elapsed_formatted() << std::endl;
		std::cout << "process finished with exit code " << result << std::endl;
		system("pause");
	}

	return result;
}
