#include "vendor/util.hpp"
#include "cprepl.hpp"
#include "cpinterpreter.hpp"
#include "vendor/watch.h"

int main(int argc, const char* argv[]) {
	auto sw = ChronoStopwatch();
	sw.start();

	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto interpreter = CPInterpreter();
	result = interpreter.execute(argc, argv);

	sw.stop();
	std::cout << std::endl << "execution time: " << sw.get_elapsed_formatted() << std::endl;
	std::cout << "process finished with exit code " << result << std::endl;
	system("pause");
	return result;
}
