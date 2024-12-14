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
	"cp.std.collections.collection",
	"cp.std.collections.dictionary",
	"cp.std.collections.hashtable",
	"cp.std.collections.list",
	"cp.std.collections.queue",
	"cp.std.collections.stack",
	"cp.std.arrays",
	"cp.std.math",
	"cp.std.print",
	"cp.std.random",
	"cp.std.strings",
	"cp.std.structs",
	"cp.std.testing",
	"cp.std.utils",
	"cp.std.DSL.BPS",
	"cp.std.DSL.JSON",
	"cp.std.DSL.YAML",
	"cp.std.DSL.XML"
};

std::map<std::string, std::shared_ptr<modules::Module>> built_in_libs = {
	{"builtin", std::shared_ptr<modules::ModuleBuiltin>(new modules::ModuleBuiltin())},
	{"cp.core.graphics", std::shared_ptr<modules::ModuleGraphics>(new modules::ModuleGraphics())},
	{"cp.core.files", std::shared_ptr<modules::ModuleFiles>(new modules::ModuleFiles())},
	{"cp.core.console", std::shared_ptr<modules::ModuleConsole>(new modules::ModuleConsole())},
	{"cp.core.datetime", std::shared_ptr<modules::ModuleDateTime>(new modules::ModuleDateTime())},
	{"cp.core.input", std::shared_ptr<modules::ModuleInput>(new modules::ModuleInput())},
	{"cp.core.sound", std::shared_ptr<modules::ModuleSound>(new modules::ModuleSound())},
	{"cp.core.HTTP", std::shared_ptr<modules::ModuleHTTP>(new modules::ModuleHTTP())}
};
