#include "cprepl.hpp"


int CPRepl::execute() {
	// REPL greeting
	std::cout << "CP Lang 1.0.0 [2024]\n";
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
			std::cout << "\n" << "Welcome to CPLang 1.0.0! \n";
			std::cout << "To use this interactive REPL, just type in regular CP commands and hit\n";
			std::cout << "enter. You can also make use of the following commands: \n\n";

			std::cout << " #load \"file path\"  Loads variable and function declarations from a specified\n";
			std::cout << std::setw(20);
			std::cout << "" << "file into memory, e.g.\n";
			std::cout << std::setw(20);
			std::cout << "" << ">>> #load .\\main.cp\n\n";

			std::cout << " #quit              Exits the CPLang REPL.\n";
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
			lexer::Lexer lexer(program, "main");
			parser::Parser parser(&lexer, "main");
			parser::ASTProgramNode* prog;

			// try to parse as program
			try {
				prog = parser.parse_program();
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
					prog = new parser::ASTProgramNode(std::vector<parser::ASTNode*>({ parser.parse_expression() }), "main");

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
				auto current = interpreter.current_expr();
				switch (current.first) {
				case parser::Type::T_BOOL:
					std::cout << ((current.second->b) ? "true" : "false");
					break;
				case parser::Type::T_INT:
					std::cout << current.second->i;
					break;
				case parser::Type::T_FLOAT:
					std::cout << current.second->f;
					break;
				case parser::Type::T_CHAR:
					std::cout << current.second->c;
					break;
				case parser::Type::T_STRING:
					std::cout << current.second->s;
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
