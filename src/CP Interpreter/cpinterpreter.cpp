#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <filesystem>

#include "lexer.hpp"
#include "parser.hpp"
#include "semantic_analysis.hpp"
#include "interpreter.hpp"
#include "vendor/util.hpp"
#include "cputil.hpp"
#include "cpinterpreter.hpp"
#include "cplibloader.hpp"


int CPInterpreter::execute(int argc, const char* argv[]) {
	std::vector<std::string> files;
	std::string root = argv[1];
	for (size_t i = 2; i < argc; ++i) {
		files.push_back(argv[i]);
	}
	if (files.size() > 0) {
		auto programs = load_programs(root, files);
		return interpreter(programs);
	}

	return 0;
}

std::vector<CPSource> CPInterpreter::load_programs(std::string root, std::vector<std::string> files) {
	std::vector<CPSource> source_programs;
	auto norm_root = axe::Util::normalize_path_sep(root);

	for (size_t i = 0; i < files.size(); ++i) {
		auto program_path = norm_root + std::string{ std::filesystem::path::preferred_separator } + axe::Util::normalize_path_sep(files[i]);
		auto program = CPSource(CPUtil::get_lib_name(files[i]), CPUtil::load_source(program_path));
		source_programs.push_back(program);
	}

	return source_programs;
}

void CPInterpreter::parse_programs(std::vector<CPSource> source_programs, parser::ASTProgramNode** main_program,
	std::map<std::string, parser::ASTProgramNode*>* programs) {

	for (auto source : source_programs) {
		// tokenise and initialise parser
		lexer::Lexer lexer(source.source, source.name);
		parser::Parser parser(&lexer, source.name);

		parser::ASTProgramNode* program = parser.parse_program();

		// check if the parsed program is null
		if (!program) {
			std::cerr << "Failed to parse program: " << source.name << std::endl;
			continue;
		}

		if (!*main_program) {
			*main_program = program;
		}

		// try to parse as program
		(*programs)[program->name] = program;
	}
}

int CPInterpreter::interpreter(std::vector<CPSource> source_programs) {
	visitor::SemanticScope semantic_global_scope;
	visitor::InterpreterScope interpreter_global_scope;

	try {
		parser::ASTProgramNode* main_program = nullptr;
		std::map<std::string, parser::ASTProgramNode*> programs;
		parse_programs(source_programs, &main_program, &programs);
		size_t cplibs_size = 0;
		do {
			visitor::LibFinder libfinder(main_program, programs);
			libfinder.start();

			cplibs_size = libfinder.lib_names.size();

			if (cplibs_size > 0) {
				auto cplib_programs = load_programs(libfinder.cp_root, libfinder.lib_names);
				parse_programs(cplib_programs, &main_program, &programs);
			}
		} while (cplibs_size > 0);

		// if this succeeds, perform semantic analysis modifying global scope
		visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, main_program, programs);
		semantic_analyser.start();

		// interpreter
		visitor::Interpreter interpreter(&interpreter_global_scope, main_program, programs);
		interpreter.start();

		return interpreter.current_expression_value.i;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
