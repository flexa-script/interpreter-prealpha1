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
	visitor::SemanticScope semantic_global_scope;
	visitor::InterpreterScope interpreter_global_scope;

	// indefinite User input
	for (;;) {

		// variables for user input
		std::string input_line;
		std::string program;
		bool file_load = false;
		bool expr = false;

		// user prompt
		std::cout << ">>> ";
		std::getline(std::cin, input_line);

		// remove leading/trailing whitespaces
		input_line = std::regex_replace(input_line, std::regex("^ +| +$"), "$1");

		// quit
		if (input_line == "#quit") {
			break;
		}

		// help
		else if (input_line == "#help") {
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
		else if (input_line.substr(0, 5) == "#load") {
			std::cout << input_line << std::endl;

			// if length <= 6, then the user specified no file
			if (input_line.size() <= 6) {
				std::cout << "File path expected after '#load'." << std::endl;
			}

			else {

				// get file directory
				std::string file_dir = input_line.substr(6);

				// remove any whitespaces from that
				file_dir = std::regex_replace(file_dir, std::regex("^ +| +$"), "$1");

				// read the file
				std::ifstream file;
				file.open(file_dir);

				if (!file) {
					std::cout << "Could not load file from \"" + file_dir + "\"." << std::endl;
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
		else if (input_line == "#clear") {
			clear_screen();
		}

		// parse as program
		else {

			// add line to program
			program += input_line;

			// count number of open scopes
			unsigned int open_scopes = 0;
			open_scopes += std::count(input_line.begin(), input_line.end(), '{');
			open_scopes -= std::count(input_line.begin(), input_line.end(), '}');

			while (open_scopes) {
				std::cout << "... ";

				// read next line
				input_line.clear();
				getline(std::cin, input_line);

				// update scope count
				open_scopes += std::count(input_line.begin(), input_line.end(), '{');
				open_scopes -= std::count(input_line.begin(), input_line.end(), '}');

				// add line to program
				program += input_line + "\n";
			}
		}

		try {

			// tokenise and initialise parser
			lexer::Lexer lexer(program);
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
					lexer::Lexer expr_lexer(program);
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
			visitor::SemanticScope temp = semantic_global_scope;
			visitor::SemanticAnalyser temp_semantic_analyser(&temp, programs);
			temp_semantic_analyser.start();

			// if this succeeds, perform semantic analysis modifying global scope
			visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, programs);
			temp_semantic_analyser.start();

			// interpreter
			visitor::Interpreter interpreter(&interpreter_global_scope, programs);
			interpreter.visit(prog);

			// if loading file, show user that everything went well
			if (file_load)
				std::cout << "File loaded successfully." << std::endl;

			// if expression, show user output
			else if (expr) {
				auto current = interpreter.currentExpr();
				switch (current.first) {
				case parser::TYPE::INT:
					std::cout << current.second.i;
					break;
				case parser::TYPE::FLOAT:
					std::cout << current.second.f;
					break;
				case parser::TYPE::BOOL:
					std::cout << ((current.second.b) ? "true" : "false");
					break;
				case parser::TYPE::STRING:
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

Program loadSource(std::string workspace, std::string file_name) {
	std::string source;
	std::string path = workspace + file_name;

	// read the file
	std::ifstream file;
	file.open(path);

	if (!file) {
		std::cout << "Could not load file from \"" << path << "\"." << std::endl;
	}
	else {
		// convert whole program to std::string
		std::string line;
		while (std::getline(file, line))
		{
			source.append(line + "\n");
		}
	}

	// skips the Byte Order Mark (BOM) that defines UTF-8 in some text files.
	if ((unsigned char)source[0] == 0xEF &&
		(unsigned char)source[1] == 0xBB &&
		(unsigned char)source[2] == 0xBF) {
		source = source.substr(3, source.size());
	}

	return Program(file_name.substr(0, file_name.length() - 3), source);
}

std::vector<Program> loadPrograms(std::string workspace, std::string main_file, std::vector<std::string> files) {
	std::vector<Program> source_programs;

	Program program = loadSource(workspace, main_file);
	if (program.source.empty()) throw EMPTY_FILE_FAILURE;
	source_programs.push_back(program);

	for (auto file_name : files) {
		program = loadSource(workspace, file_name);
		if (program.source.empty()) throw EMPTY_FILE_FAILURE;
		source_programs.push_back(program);
	}
}

int interpreter(std::vector<Program> source_programs) {
	// create Global Scopes
	visitor::SemanticScope semantic_global_scope;
	visitor::InterpreterScope interpreter_global_scope;

	try {
		std::vector<parser::ASTProgramNode*> programs;

		for (auto source : source_programs) {
			// tokenise and initialise parser
			lexer::Lexer lexer(source.source);
			parser::Parser parser(&lexer, source.name);

			// try to parse as program
			programs.push_back(parser.parseProgram());
		}

		// if this succeeds, perform semantic analysis modifying global scope
		visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, programs);
		semantic_analyser.start();

		// interpreter
		visitor::Interpreter interpreter(&interpreter_global_scope, programs);
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

/**
 * The main function implements the interpreter.
 * @return 0
 */
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

	//result = interpreter("C:\\Users\\Carlos\\Documents\\newProject\\", "main.cp", { });
	//system("pause");
	//return result;
	//return interpreter("C:\\Users\\Carlos\\Documents\\newProject\\", "main.cp", { "math.cp", "problems.cp"});

	// check if it has arguments
	if (argc == 4) {
		std::string workspace = argv[1];
		std::string main_file = argv[2];
		std::vector<std::string> files;

		if(argc>= 3) {
			std::string strfiles = argv[3];
			strfiles = strfiles.substr(1, strfiles.length() - 2);
			std::vector<std::string> auxFiles = split(strfiles, ',');
			for (auto file : auxFiles) {
				files.push_back(trim(file));
			}
		}
		auto programs = loadPrograms(workspace, main_file, files);
		result = interpreter(programs);
	}

	//for (int i = 0; i < argc; ++i) {
	//	std::cout << argv[i] << std::endl;
	//}
	//return repl();
	//std::cout << std::endl << std::endl;
	system("pause");
	return result;
}
