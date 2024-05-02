#include "cprepl.hpp"
#include "cpinterpreter.hpp"
//#include "logging.hpp"
//#include "watch.h"


int main(int argc, const char* argv[]) {
	//auto log = axe::Logger(axe::DEBUG, ".\\data.log");

	//for (int i = 0; i < argc; ++i) {
	//	log.debug(argv[i]);
	//}

	//auto sw = ChronoStopwatch();
	//sw.start();

	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto interpreter = CPInterpreter();
	result = interpreter.execute(argc, argv);

	//sw.stop();
	//std::cout << sw.get_elapsed_formatted() << std::endl;

	system("pause");
	return result;
}
