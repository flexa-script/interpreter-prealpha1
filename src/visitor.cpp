#include "visitor.hpp"

#include "md_builtin.hpp"
#include "md_datetime.hpp"
#include "md_graphics.hpp"
#include "md_files.hpp"
#include "md_console.hpp"
#include "md_input.hpp"
#include "md_sound.hpp"
#include "md_HTTP.hpp"

using namespace visitor;

Visitor::Visitor(const std::map<std::string, std::shared_ptr<ASTProgramNode>>& programs, std::shared_ptr<ASTProgramNode> main_program, const std::string& current_this_name)
	: programs(programs), main_program(main_program), curr_row(0), curr_col(0) {
	current_program.push(main_program);
};

std::string default_namespace = "__default__";

std::vector<std::string> std_libs = {
	"bsl.std.collections.collection",
	"bsl.std.collections.dictionary",
	"bsl.std.collections.hashtable",
	"bsl.std.collections.list",
	"bsl.std.collections.queue",
	"bsl.std.collections.stack",
	"bsl.std.arrays",
	"bsl.std.math",
	"bsl.std.print",
	"bsl.std.random",
	"bsl.std.strings",
	"bsl.std.structs",
	"bsl.std.testing",
	"bsl.std.utils",
	"bsl.std.DSL.BPS",
	"bsl.std.DSL.JSON",
	"bsl.std.DSL.YAML",
	"bsl.std.DSL.XML"
};

std::map<std::string, std::shared_ptr<modules::Module>> built_in_libs = {
	{"builtin", std::shared_ptr<modules::ModuleBuiltin>(new modules::ModuleBuiltin())},
	{"bsl.core.graphics", std::shared_ptr<modules::ModuleGraphics>(new modules::ModuleGraphics())},
	{"bsl.core.files", std::shared_ptr<modules::ModuleFiles>(new modules::ModuleFiles())},
	{"bsl.core.console", std::shared_ptr<modules::ModuleConsole>(new modules::ModuleConsole())},
	{"bsl.core.datetime", std::shared_ptr<modules::ModuleDateTime>(new modules::ModuleDateTime())},
	{"bsl.core.input", std::shared_ptr<modules::ModuleInput>(new modules::ModuleInput())},
	{"bsl.core.sound", std::shared_ptr<modules::ModuleSound>(new modules::ModuleSound())},
	{"bsl.core.HTTP", std::shared_ptr<modules::ModuleHTTP>(new modules::ModuleHTTP())}
};
