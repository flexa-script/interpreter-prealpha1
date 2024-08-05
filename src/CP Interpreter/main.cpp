#include "cprepl.hpp"
#include "cpinterpreter.hpp"
#include "vendor/axewatch.hpp"

int main(int argc, const char* argv[]) {
	bool debug = false;
	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	std::vector<std::string> files;
	std::string root = "";
	for (size_t i = 1; i < argc; ++i) {
		if (argv[i] == "-d" || argv[i] == "--debug") {
			debug = true;
			continue;
		}
		if (root == "") {
			root = argv[i];
			continue;
		}
		files.push_back(argv[i]);
	}

	auto interpreter = CPInterpreter(root, std::move(files));

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
