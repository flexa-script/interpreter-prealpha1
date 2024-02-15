#include "cpinterpreter.h"


int CPInterpreter::execute(int argc, const char* argv[]) {
	if (argc == 2) {
		std::string arg = argv[1];
		if (axe::tolower(arg) == "-d" || axe::tolower(arg) == "--debug") {
			std::vector<Program> programs = debugPrograms();
			return interpreter(programs);
		}
	}

	std::vector<std::string> files;
	for (size_t i = 1; i < argc; ++i) {
		files.push_back(argv[i]);
	}
	if (files.size() > 0) {
		auto programs = loadPrograms(files);
		return interpreter(programs);
	}

	return 0;
}

std::vector<Program> CPInterpreter::loadPrograms(std::vector<std::string> files) {
	std::vector<Program> sourcePrograms;
	auto startLibNameIndex = files[0].find_last_of('\\') + 1;

	auto program = Program(CPUtil::getLibName(startLibNameIndex, files[0]), CPUtil::loadSource(files[0]));
	if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
	sourcePrograms.push_back(program);

	for (size_t i = 1; i < files.size(); ++i) {
		program = Program(CPUtil::getLibName(startLibNameIndex, files[i]), CPUtil::loadSource(files[i]));
		if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
		sourcePrograms.push_back(program);
	}

	return sourcePrograms;
}

int CPInterpreter::interpreter(std::vector<Program> sourcePrograms) {
	// create Global Scopes
	visitor::SemanticScope semanticGlobalScope;
	visitor::InterpreterScope interpreterGlobalScope;

	try {
		std::vector<parser::ASTProgramNode*> programs;

		for (auto source : sourcePrograms) {
			// tokenise and initialise parser
			lexer::Lexer lexer(source.source, source.name);
			parser::Parser parser(&lexer, source.name);

			// try to parse as program
			programs.push_back(parser.parseProgram());
		}

		// if this succeeds, perform semantic analysis modifying global scope
		visitor::SemanticAnalyser semanticAnalyser(&semanticGlobalScope, programs);
		semanticAnalyser.start();

		// interpreter
		visitor::Interpreter interpreter(&interpreterGlobalScope, programs);
		interpreter.start();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

std::vector<Program> CPInterpreter::debugPrograms() {
	std::vector<Program> sourcePrograms;

	std::string source =
		"using math;"
		"def main() {"
		"  print(\"Type a number: \");"
		"  var number : float = float(read());"
		"  print(\"Type the min number: \");"
		"  var min : float = float(read());"
		"  print(\"Type the max number: \");"
		"  var max : float = float(read());"
		"  var res : float = clamp(number, min, max);"
		"  print(\"The clamped number is: \" + string(res) + \"\\n\");"
		"}"
		"main();";
	sourcePrograms.push_back(Program("main", source));

	source =
		"def clamp(val : float, min : float, max : float) : float {"
		"  if (val > max) {"
		"    return max;"
		"  }"
		"  if (val < min) {"
		"    return min;"
		"  }"
		"  return val;"
		"}";
	sourcePrograms.push_back(Program("math", source));

	return sourcePrograms;
}
