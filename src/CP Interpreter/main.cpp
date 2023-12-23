#include "CPRepl.h"
#include "CPInterpreter.h"


int main(int argc, const char* argv[]) {
	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto interpreter = CPInterpreter();
	result = interpreter.execute(argc, argv);

	system("pause");
	return 0;
}
