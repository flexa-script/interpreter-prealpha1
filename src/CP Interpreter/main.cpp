#include "cprepl.hpp"
#include "cpinterpreter.hpp"


int main(int argc, const char* argv[]) {
	//std::ofstream of;
	//of.open("data.log", std::ios::out);
	//if (of.is_open()) {
	//	for (size_t i = 0; i < argc; ++i) {
	//		of << argv[i] << std::endl;
	//	}
	//}
	//of.close();

	int result = 0;

	if (argc == 1) {
		return CPRepl::execute();
	}

	auto interpreter = CPInterpreter();
	result = interpreter.execute(argc, argv);

	system("pause");
	return result;
}
