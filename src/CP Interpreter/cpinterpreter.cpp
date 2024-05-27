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
	if (argc == 2) {

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

std::vector<CPSource> CPInterpreter::load_programs(std::vector<std::string> files) {
	std::vector<CPSource> source_programs;
	auto norm_path = axe::Util::normalize_path_sep(files[0]);
	auto start_lib_name_index = norm_path.find_last_of(std::filesystem::path::preferred_separator) + 1;

	auto program = CPSource(CPUtil::get_lib_name(start_lib_name_index, norm_path), CPUtil::load_source(norm_path));
	if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
	source_programs.push_back(program);

	for (size_t i = 1; i < files.size(); ++i) {
		norm_path = axe::Util::normalize_path_sep(files[i]);
		program = CPSource(CPUtil::get_lib_name(start_lib_name_index, norm_path), CPUtil::load_source(norm_path));
		if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
		source_programs.push_back(program);
	}

	return source_programs;
}

std::vector<CPSource> CPInterpreter::load_cp_libs(std::vector<std::string> files) {
	std::vector<CPSource> source_programs;
	auto norm_path = axe::Util::normalize_path_sep(files[0]);
	std::string searchstr("cp" + std::string{ std::filesystem::path::preferred_separator } + "std");
	auto start_lib_name_index = norm_path.rfind(searchstr);

	auto program = CPSource(CPUtil::get_lib_name(start_lib_name_index, norm_path), CPUtil::load_source(norm_path));
	if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
	source_programs.push_back(program);

	for (size_t i = 1; i < files.size(); ++i) {
		norm_path = axe::Util::normalize_path_sep(files[i]);
		program = CPSource(CPUtil::get_lib_name(start_lib_name_index, norm_path), CPUtil::load_source(norm_path));
		if (program.source.empty()) throw std::runtime_error("file '" + program.name + "' is empty");
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
	// create Global Scopes
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

			cplibs_size = libfinder.get_lib_names().size();

			if (cplibs_size > 0) {
				auto cplib_programs = load_cp_libs(libfinder.get_lib_names());
				parse_programs(cplib_programs, &main_program, &programs);
			}
		} while (cplibs_size > 0);

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
