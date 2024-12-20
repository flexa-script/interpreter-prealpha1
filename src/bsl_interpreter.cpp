#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <filesystem>

#include "bsl_interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "utils.hpp"
#include "linker.hpp"
#include "interpreter.hpp"
#include "vm.hpp"

CPInterpreter::CPInterpreter(const BSLCliArgs& args)
	: project_root(utils::PathUtils::normalize_path_sep(args.workspace)),
	cp_root(utils::PathUtils::normalize_path_sep(utils::PathUtils::get_current_path() + "libs")),
	args(args) {}

int CPInterpreter::execute() {
	if (!args.main.empty() || args.sources.size() > 0) {
		return interpreter();
	}

	return 0;
}

BSLSource CPInterpreter::load_program(const std::string& source) {
	BSLSource source_program;

	auto current_file_path = std::string{ std::filesystem::path::preferred_separator } + utils::PathUtils::normalize_path_sep(source);
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

	source_program = BSLSource{ get_lib_name(source), load_source(current_full_path) };

	return source_program;
}

std::vector<BSLSource> CPInterpreter::load_programs(const std::vector<std::string>& sources) {
	std::vector<BSLSource> source_programs;

	for (const auto& source : sources) {
		source_programs.push_back(load_program(source));
	}

	return source_programs;
}

void CPInterpreter::parse_programs(const std::vector<BSLSource>& source_programs, std::shared_ptr<ASTProgramNode>* main_program,
	std::map<std::string, std::shared_ptr<ASTProgramNode>>* programs) {

	for (const auto& source : source_programs) {
		lexer::Lexer lexer(source.name, source.source);
		parser::Parser parser(source.name , &lexer);

		std::shared_ptr<ASTProgramNode> program = parser.parse_program();

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
	BSLSource main_program;
	std::vector<BSLSource> source_programs;
	try {
		main_program = load_program(args.main);
		source_programs = load_programs(args.sources);
		source_programs.emplace(source_programs.begin(), main_program);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::shared_ptr<visitor::Scope> semantic_global_scope = std::make_shared<visitor::Scope>(nullptr);
	std::shared_ptr<visitor::Scope> interpreter_global_scope = std::make_shared<visitor::Scope>(nullptr);

	try {
		std::shared_ptr<ASTProgramNode> main_program = nullptr;
		std::map<std::string, std::shared_ptr<ASTProgramNode>> programs;
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

		semantic_global_scope->owner = main_program;
		interpreter_global_scope->owner = main_program;

		visitor::SemanticAnalyser semantic_analyser(semantic_global_scope, main_program, programs, args.cpargs);
		semantic_analyser.start();

		long long result = 0;

		if (args.engine == "ast") {
			visitor::Interpreter interpreter(interpreter_global_scope, main_program, programs, args.cpargs);
			interpreter.start();
			result = interpreter.current_expression_value->get_i();
		}
		else {
			// compile
			visitor::Compiler compiler(main_program, programs, args.cpargs);
			compiler.start();

			BytecodeInstruction::write_bytecode_table(compiler.bytecode_program, project_root + "\\" + source_programs[0].name + ".bslt");

			// execute
			VirtualMachine vm(interpreter_global_scope, compiler.bytecode_program);
			vm.run();

			result = vm.value_stack.back()->get_i();
		}

		return result;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
