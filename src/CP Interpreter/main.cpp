#include "cprepl.hpp"
#include "cpinterpreter.hpp"
#include "vendor/watch.h"
//#include "vendor/util.hpp"
//#include "vendor/logging.hpp"

int main(int argc, const char* argv[]) {
	//auto logger = axe::Logger(axe::INFO, ".\\logs.log");
	//std::string str = "";
	//for (int i = 0; i < argc; ++i) {
	//	str.append(argv[i]);
	//}
	//logger.debug(str);

	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto sw = ChronoStopwatch();
	sw.start();

	auto interpreter = CPInterpreter();
	result = interpreter.execute(argc, argv);

	sw.stop();
	std::cout << std::endl << "execution time: " << sw.get_elapsed_formatted() << std::endl;
	std::cout << "process finished with exit code " << result << std::endl;
	system("pause");
	return result;
}
