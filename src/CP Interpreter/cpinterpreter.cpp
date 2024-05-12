#include "cpinterpreter.hpp"


int CPInterpreter::execute(int argc, const char* argv[]) {
	if (argc == 2) {
		//std::string arg = argv[1];
		//if (axe::tolower(arg) == "-d" || axe::tolower(arg) == "--debug") {
		//	std::vector<Program> programs = debug_programs();
		//	return interpreter(programs);
		//}
	}

	std::vector<std::string> files;
	for (size_t i = 1; i < argc; ++i) {
		files.push_back(argv[i]);
	}
	if (files.size() > 0) {
		auto programs = load_programs(files);
		return interpreter(programs);
	}

	return 0;
}

std::vector<Program> CPInterpreter::load_programs(std::vector<std::string> files) {
	std::vector<Program> source_programs;
	auto start_lib_name_index = files[0].find_last_of('\\') + 1;

	auto program = Program(CPUtil::getLibName(start_lib_name_index, files[0]), CPUtil::load_source(files[0]));
	if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
	source_programs.push_back(program);

	for (size_t i = 1; i < files.size(); ++i) {
		program = Program(CPUtil::getLibName(start_lib_name_index, files[i]), CPUtil::load_source(files[i]));
		if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
		source_programs.push_back(program);
	}

	return source_programs;
}

int CPInterpreter::interpreter(std::vector<Program> source_programs) {
	// create Global Scopes
	visitor::SemanticScope semantic_global_scope;
	visitor::InterpreterScope interpreter_global_scope;

	try {
		parser::ASTProgramNode* main_program = nullptr;
		std::map<std::string, parser::ASTProgramNode*> programs;

		for (auto source : source_programs) {
			// tokenise and initialise parser
			lexer::Lexer lexer(source.source, source.name);
			parser::Parser parser(&lexer, source.name);

			parser::ASTProgramNode* program = parser.parse_program();

			if (!main_program) {
				main_program = program;
			}

			// try to parse as program
			programs[program->name] = program;
		}

		// if this succeeds, perform semantic analysis modifying global scope
		visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, main_program, programs);
		semantic_analyser.start();

		// interpreter
		visitor::Interpreter interpreter(&interpreter_global_scope, main_program, programs);
		interpreter.start();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//std::vector<Program> CPInterpreter::debug_programs() {
//	std::vector<Program> source_programs;
//
//	std::string source =
//		"using math;"
//		"def main() {"
//		"  print(\"Type a number: \");"
//		"  var number : float = float(read());"
//		"  print(\"Type the min number: \");"
//		"  var min : float = float(read());"
//		"  print(\"Type the max number: \");"
//		"  var max : float = float(read());"
//		"  var res : float = clamp(number, min, max);"
//		"  print(\"The clamped number is: \" + string(res) + \"\\n\");"
//		"}"
//		"main();";
//	source_programs.push_back(Program("main", source));
//
//	source =
//		"def clamp(val : float, min : float, max : float) : float {"
//		"  if (val > max) {"
//		"    return max;"
//		"  }"
//		"  if (val < min) {"
//		"    return min;"
//		"  }"
//		"  return val;"
//		"}";
//	source_programs.push_back(Program("math", source));
//
//	return source_programs;
//}
