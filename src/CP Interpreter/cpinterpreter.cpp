#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <filesystem>

#include "cpinterpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "vendor/axeutils.hpp"
#include "cputil.hpp"
#include "linker.hpp"

CPInterpreter::CPInterpreter(const std::string& project_root, std::vector<std::string>&& files)
	: project_root(axe::PathUtils::normalize_path_sep(project_root)),
	cp_root(axe::PathUtils::normalize_path_sep(axe::PathUtils::get_current_path() + "libs")),
	files(std::move(files)) {}

int CPInterpreter::execute() {
	if (files.size() > 0) {
		return interpreter();
	}

	return 0;
}

std::vector<CPSource> CPInterpreter::load_programs(const std::vector<std::string>& files) {
	std::vector<CPSource> source_programs;

	for (size_t i = 0; i < files.size(); ++i) {
		auto current_file_path = std::string{ std::filesystem::path::preferred_separator } + axe::PathUtils::normalize_path_sep(files[i]);
		std::string current_full_path = "";

		if (std::filesystem::exists(project_root + current_file_path)) {
			current_full_path = project_root + current_file_path;
		}
		else if (std::filesystem::exists(cp_root + current_file_path)) {
			current_full_path = cp_root + current_file_path;
		}
		else {
			throw std::runtime_error("file not found: '" + current_file_path + "'");
		}

		auto program = CPSource(CPUtil::get_lib_name(files[i]), CPUtil::load_source(current_full_path));
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

int CPInterpreter::interpreter() {
	const std::vector<CPSource>& source_programs = load_programs(files);

	std::shared_ptr<visitor::Scope> semantic_global_scope = std::make_shared<visitor::Scope>();

	try {
		parser::ASTProgramNode* main_program = nullptr;
		std::map<std::string, parser::ASTProgramNode*> programs;
		parse_programs(source_programs, &main_program, &programs);
		size_t cplibs_size = 0;
		do {
			visitor::Linker libfinder(main_program, programs);
			libfinder.start();

			cplibs_size = libfinder.lib_names.size();

			if (cplibs_size > 0) {
				auto cplib_programs = load_programs(libfinder.lib_names);
				parse_programs(cplib_programs, &main_program, &programs);
			}
		} while (cplibs_size > 0);

		visitor::SemanticAnalyser semantic_analyser(semantic_global_scope, main_program, programs);
		semantic_analyser.start();

		// compile
		visitor::Compiler compiler(main_program, programs);
		compiler.start();

		BytecodeInstruction::write_bytecode_table(compiler.bytecode_program, project_root + "\\" + source_programs[0].name + ".cpt");

		// execute
		//visitor::Interpreter interpreter(interpreter_global_scope, main_program, programs);
		//interpreter.start();

		return 0;//interpreter.current_expression_value->get_i();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
