#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>

#ifdef __unix__
#define clear_screen() system("clear")
#elif defined(_WIN32) || defined(WIN32)
#define clear_screen() system("cls")
#endif

#include "lexer.h"
#include "parser.h"
#include "semantic_analysis.h"
#include "interpreter.h"
#include "util.h"


#define EMPTY_FILE_FAILURE 666


int repl() {
	// REPL greeting
	std::cout << "CP Lang 1.0.0 [2023]\n";
	std::cout << "Type \"#help\" for more information.\n";

	// create Global Scopes
	visitor::SemanticScope semanticGlobalScope;
	visitor::InterpreterScope interpreterGlobalScope;

	// indefinite User input
	for (;;) {

		// variables for user input
		std::string inputLine;
		std::string program;
		bool file_load = false;
		bool expr = false;

		// user prompt
		std::cout << ">>> ";
		std::getline(std::cin, inputLine);

		// remove leading/trailing whitespaces
		inputLine = std::regex_replace(inputLine, std::regex("^ +| +$"), "$1");

		// quit
		if (inputLine == "#quit") {
			break;
		}

		// help
		else if (inputLine == "#help") {
			std::cout << "\n" << "Welcome to MiniLang 1.0.0! \n";
			std::cout << "To use this interactive REPL, just type in regular CP commands and hit\n";
			std::cout << "enter. You can also make use of the following commands: \n\n";

			std::cout << " #load \"file path\"  Loads variable and function declarations from a specified\n";
			std::cout << std::setw(20);
			std::cout << "" << "file into memory, e.g.\n";
			std::cout << std::setw(20);
			std::cout << "" << ">>> #load ~/hello_world.prog\n\n";

			std::cout << " #quit              Exits the MiniLang REPL.\n";
			std::cout << std::setw(20);
			std::cout << "" << "functions and variables in the global scope.\n\n";

			std::cout << " #clear             Clears the terminal window.\n\n";
		}

		// load File
		else if (inputLine.substr(0, 5) == "#load") {
			std::cout << inputLine << std::endl;

			// if length <= 6, then the user specified no file
			if (inputLine.size() <= 6) {
				std::cout << "File path expected after '#load'." << std::endl;
			}

			else {

				// get file directory
				std::string fileDir = inputLine.substr(6);

				// remove any whitespaces from that
				fileDir = std::regex_replace(fileDir, std::regex("^ +| +$"), "$1");

				// read the file
				std::ifstream file;
				file.open(fileDir);

				if (!file) {
					std::cout << "Could not load file from \"" + fileDir + "\"." << std::endl;
				}
				else {
					// convert whole program to std::string
					std::string line;
					while (std::getline(file, line))
						program.append(line + "\n");

					// flag to indicate that this statement is for file loading
					file_load = true;
				}

				file.close();
			}
		}

		// clear Screen
		else if (inputLine == "#clear") {
			clear_screen();
		}

		// parse as program
		else {

			// add line to program
			program += inputLine;

			// count number of open scopes
			unsigned int openScopes = 0;
			openScopes += std::count(inputLine.begin(), inputLine.end(), '{');
			openScopes -= std::count(inputLine.begin(), inputLine.end(), '}');

			while (openScopes) {
				std::cout << "... ";

				// read next line
				inputLine.clear();
				getline(std::cin, inputLine);

				// update scope count
				openScopes += std::count(inputLine.begin(), inputLine.end(), '{');
				openScopes -= std::count(inputLine.begin(), inputLine.end(), '}');

				// add line to program
				program += inputLine + "\n";
			}
		}

		try {

			// tokenise and initialise parser
			lexer::Lexer lexer(program, "main");
			parser::Parser parser(&lexer, "main");
			parser::ASTProgramNode* prog;

			// try to parse as program
			try {
				prog = parser.parseProgram();
			}

			// catch by trying to parse as expression
			catch (const std::exception& e) {

				try {
					// if expression ends with ';', get rid of it
					if (program.back() == ';') {
						program.pop_back();
					}

					// parse again, create program node manually
					lexer::Lexer expr_lexer(program, "main");
					parser = parser::Parser(&expr_lexer, 0);  // do not consume first token
					prog = new parser::ASTProgramNode(std::vector<parser::ASTNode*>({ parser.parseExpression() }), "main");

					expr = true;
				}
				catch (const std::exception& expr_e) {
					// throw original error
					throw std::runtime_error(e.what());
				}
			}

			// try to analyse in a temporary copy of the global scope (just in case the program is invalid)
			auto programs = std::vector<parser::ASTProgramNode*>({ prog });
			visitor::SemanticScope temp = semanticGlobalScope;
			visitor::SemanticAnalyser tempSemanticAnalyser(&temp, programs);
			tempSemanticAnalyser.start();

			// if this succeeds, perform semantic analysis modifying global scope
			visitor::SemanticAnalyser semantic_analyser(&semanticGlobalScope, programs);
			tempSemanticAnalyser.start();

			// interpreter
			visitor::Interpreter interpreter(&interpreterGlobalScope, programs);
			interpreter.visit(prog);

			// if loading file, show user that everything went well
			if (file_load)
				std::cout << "File loaded successfully." << std::endl;

			// if expression, show user output
			else if (expr) {
				auto current = interpreter.currentExpr();
				switch (current.first) {
				case parser::TYPE::T_INT:
					std::cout << current.second.i;
					break;
				case parser::TYPE::T_FLOAT:
					std::cout << current.second.f;
					break;
				case parser::TYPE::T_BOOL:
					std::cout << ((current.second.b) ? "true" : "false");
					break;
				case parser::TYPE::T_STRING:
					std::cout << current.second.s;
					break;
				}
			}

			std::cout << std::endl;
		}

		// catch exception and print error
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	return EXIT_SUCCESS;
}


class Program {
public:
	std::string name;
	std::string source;
	Program(std::string name, std::string source)
		:name(name), source(source) {}
};

Program loadSource(std::string workspace, std::string fileName) {
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

std::vector<Program> loadPrograms(std::string workspace, std::string mainFile, std::vector<std::string> files) {
	std::vector<Program> sourcePrograms;

	Program program = loadSource(workspace, mainFile);
	if (program.source.empty()) throw EMPTY_FILE_FAILURE;
	sourcePrograms.push_back(program);

	for (auto file_name : files) {
		program = loadSource(workspace, file_name);
		if (program.source.empty()) throw EMPTY_FILE_FAILURE;
		sourcePrograms.push_back(program);
	}

	return sourcePrograms;
}

int interpreter(std::vector<Program> sourcePrograms) {
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

std::vector<Program> debugPrograms() {
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

int main(int argc, const char* argv[]) {
	int result = 0;
	std::vector<std::string> args;

	for (size_t i = 0; i < argc; ++i) {
		args.push_back(argv[i]);
	}

	if (argc == 1) {
		return repl();
	}

	if (argc == 2) {
		std::string arg = argv[1];
		if (tolower(arg) == "-d" || tolower(arg) == "--debug") {
			std::vector<Program> programs = debugPrograms();
			result = interpreter(programs);
		}
	}

	// check if it has arguments
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
		result = interpreter(programs);
	}

	system("pause");
	return result;
}
