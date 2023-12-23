#include "CPInterpreter.h"


int CPInterpreter::execute(int argc, const char* argv[]) {
	if (argc == 2) {
		std::string arg = argv[1];
		if (tolower(arg) == "-d" || tolower(arg) == "--debug") {
			std::vector<Program> programs = debugPrograms();
			return interpreter(programs);
		}
	}

	//check if it has arguments
	if (argc >= 3) {
		std::string workspace = argv[1];
		std::string mainFile = argv[2];
		std::vector<std::string> files;

		if (argc >= 4) {
			std::string strfiles = argv[3];
			strfiles = strfiles.substr(1, strfiles.length() - 2);
			std::vector<std::string> auxFiles = split(strfiles, ',');
			for (auto file : auxFiles) {
				files.push_back(trim(file));
			}
		}
		auto programs = loadPrograms(workspace, mainFile, files);
		return interpreter(programs);
	}
}

Program CPInterpreter::loadSource(std::string workspace, std::string fileName) {
	std::string source;
	std::string path = workspace + '\\' + fileName;

	// read the file
	std::ifstream file;
	file.open(path);

	if (!file) {
		std::cout << "Could not load file from \"" << path << "\"." << std::endl;
	}
	else {
		// convert whole program to std::string
		std::string line;
		while (std::getline(file, line)) {
			source.append(line + "\n");
		}
	}

	// skips the Byte Order Mark (BOM) that defines UTF-8 in some text files.
	if ((unsigned char)source[0] == 0xEF &&
		(unsigned char)source[1] == 0xBB &&
		(unsigned char)source[2] == 0xBF) {
		source = source.substr(3, source.size());
	}

	std::string libName = fileName.substr(0, fileName.length() - 3);
	std::replace(libName.begin(), libName.end(), '/', '.');
	std::replace(libName.begin(), libName.end(), '\\', '.');
	return Program(libName, source);
}

std::vector<Program> CPInterpreter::loadPrograms(std::string workspace, std::string mainFile, std::vector<std::string> files) {
	std::vector<Program> sourcePrograms;

	Program program = loadSource(workspace, mainFile);
	if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
	sourcePrograms.push_back(program);

	for (auto file_name : files) {
		program = loadSource(workspace, file_name);
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
