#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <filesystem>

#include "lexer.hpp"
#include "parser.hpp"
#include "semantic_analysis.hpp"
#include "interpreter.hpp"
#include "vendor/axeutils.hpp"
#include "cputil.hpp"
#include "cpinterpreter.hpp"
#include "libfinder.hpp"

CPInterpreter::CPInterpreter(const std::string& root, std::vector<std::string>&& files)
	: root(root), files(std::move(files)) {}

int CPInterpreter::execute() {
	if (files.size() > 0) {
		auto programs = load_programs(root, files);
		return interpreter(programs);
	}

	return 0;
}

std::vector<CPSource> CPInterpreter::load_programs(const std::string& root, const std::vector<std::string>& files) {
	std::vector<CPSource> source_programs;
	auto norm_root = axe::PathUtils::normalize_path_sep(root);

	for (size_t i = 0; i < files.size(); ++i) {
		auto program_path = norm_root + std::string{ std::filesystem::path::preferred_separator } + axe::PathUtils::normalize_path_sep(files[i]);
		auto program = CPSource(CPUtil::get_lib_name(files[i]), CPUtil::load_source(program_path));
		source_programs.push_back(program);
	}

	return source_programs;
}

void CPInterpreter::parse_programs(const std::vector<CPSource>& source_programs, parser::ASTProgramNode** main_program,
	std::map<std::string, parser::ASTProgramNode*>* programs) {

	for (const auto& source : source_programs) {
		lexer::Lexer lexer(source.name, source.source);
		parser::Parser parser(source.name , &lexer);

		parser::ASTProgramNode* program = parser.parse_program();

		if (!program) {
			std::cerr << "Failed to parse program: " << source.name << std::endl;
			continue;
		}

		if (!*main_program) {
			*main_program = program;
		}

		(*programs)[program->name] = program;
	}
}

int CPInterpreter::interpreter(const std::vector<CPSource>& source_programs) {
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

		visitor::SemanticAnalyser semantic_analyser(&semantic_global_scope, main_program, programs);
		semantic_analyser.start();

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
