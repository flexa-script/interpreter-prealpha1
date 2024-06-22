#include "cprepl.hpp"
#include "vendor/util.hpp"

const std::string CPRepl::NAME = "CPLang";
const std::string CPRepl::VER = "v0.0.1";
const std::string CPRepl::YEAR = "2024";

void CPRepl::remove_header(std::string& err) {
	size_t pos = err.rfind(':');
	if (pos != std::string::npos) {
		err = err.substr(pos + 1);
	}
}

std::string CPRepl::read(const std::string& msg) {
	std::string input_line;
	std::cout << msg;
	std::getline(std::cin, input_line);
	return input_line;
}

void CPRepl::count_scopes(const std::string& input_line, unsigned int& open_scopes) {
	open_scopes += std::count(input_line.begin(), input_line.end(), '{');
	open_scopes -= std::count(input_line.begin(), input_line.end(), '}');
}

int CPRepl::execute() {
	std::cout << NAME << " " << VER << " [" << YEAR << "]\n";
	std::cout << "Type \"#help\" for more information.\n";

	visitor::SemanticScope semantic_global_scope;
	visitor::InterpreterScope interpreter_global_scope;

	while (true) {
		std::string input_line;
		std::string source;
		bool file_load = false;
		bool expr = false;

		input_line = read(">>> ");
		input_line = std::regex_replace(input_line, std::regex("^ +| +$"), "$1");

		if (input_line == "#quit") {
			break;
		}
		else if (input_line == "#help") {
			std::cout << "\n" << "Welcome to " << NAME << " " << VER << "! \n";
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
		else if (input_line.substr(0, 5) == "#load") {
			std::cout << input_line << std::endl;

			if (input_line.size() <= 6) {
				std::cout << "File path expected after '#load'." << std::endl;
				continue;
			}

			std::string file_dir = input_line.substr(6);
			file_dir = std::regex_replace(file_dir, std::regex("^ +| +$"), "$1");
			std::ifstream file;
			file.open(file_dir);

			if (!file) {
				std::cout << "Could not load file from \"" + file_dir + "\"." << std::endl;
				continue;
			}

			std::string line;
			while (std::getline(file, line)) {
				source.append(line + "\n");
			}
			file_load = true;

			file.close();
		}
		else if (input_line == "#clear") {
			clear_screen();
		}
		else {
			source += input_line;

			unsigned int open_scopes = 0;
			count_scopes(input_line, open_scopes);

			while (open_scopes) {
				input_line.clear();
				input_line = read("... ");
				count_scopes(input_line, open_scopes);
				source += input_line + "\n";
			}
		}

		try {
			lexer::Lexer lexer("__main", source);
			parser::Parser parser("__main", &lexer);
			parser::ASTProgramNode* program;
			std::map<std::string, parser::ASTProgramNode*> programs;

			try {
				program = parser.parse_program();
				programs = std::map<std::string, parser::ASTProgramNode*>({ std::pair("__main", program) });
			}
			catch (const std::exception& e) {
				std::string err = e.what();
				remove_header(err);
				std::cerr << axe::Util::trim(err) << std::endl;
				continue;
			}

			// check if it's all ok using a temp global scope
			visitor::SemanticScope temp = semantic_global_scope;
			visitor::SemanticAnalyser temp_semantic_analyser(&temp, program, programs);
			temp_semantic_analyser.start();

			visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, program, programs);
			semantic_analyser.start();
			visitor::Interpreter interpreter(&interpreter_global_scope, program, programs);
			interpreter.visit(program);

			if (file_load) {
				std::cout << "File loaded successfully." << std::endl;
			}
			else {
				// not is undefined and it's an expression
				if (!parser::is_undefined(interpreter.current_expression_value.curr_type)
					&& source.find(';') == std::string::npos) {
					std::cout << interpreter.parse_value_to_string(&interpreter.current_expression_value) << std::endl;
				}
			}
		}
		catch (const std::exception& e) {
			std::string err = e.what();
			remove_header(err);
			std::cerr << axe::Util::trim(err) << std::endl;
		}
	}

	return EXIT_SUCCESS;
}
