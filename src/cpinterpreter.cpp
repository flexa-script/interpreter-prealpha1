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
#include "interpreter.hpp"
#include "virtual_machine.hpp"

CPInterpreter::CPInterpreter(const std::string& project_root, std::vector<std::string>&& files,
	bool debug, const std::string& engine)
	: project_root(axe::PathUtils::normalize_path_sep(project_root)),
	cp_root(axe::PathUtils::normalize_path_sep(axe::PathUtils::get_current_path() + "libs")),
	files(std::move(files)), debug(debug), engine(engine) {}

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

void CPInterpreter::parse_programs(const std::vector<CPSource>& source_programs, std::shared_ptr<ASTProgramNode>* main_program,
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
	const std::vector<CPSource>& source_programs = load_programs(files);

	std::shared_ptr<visitor::Scope> semantic_global_scope = std::make_shared<visitor::Scope>();
	std::shared_ptr<visitor::Scope> interpreter_global_scope = std::make_shared<visitor::Scope>();

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

		visitor::SemanticAnalyser semantic_analyser(semantic_global_scope, main_program, programs);
		semantic_analyser.start();

		long long result = 0;

		if (engine == "ast") {
			visitor::Interpreter interpreter(interpreter_global_scope, main_program, programs);
			interpreter.start();
			result = interpreter.current_expression_value->get_i();
		}
		else {
			// compile
			visitor::Compiler compiler(main_program, programs);
			compiler.start();

			BytecodeInstruction::write_bytecode_table(compiler.bytecode_program, project_root + "\\" + source_programs[0].name + ".cpt");

			// execute
			VirtualMachine vm(compiler.bytecode_program);
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
