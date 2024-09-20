#include "visitor.hpp"

#include "datetime.hpp"
#include "graphics.hpp"
#include "files.hpp"
#include "console.hpp"
#include "input.hpp"

using namespace visitor;

Visitor::Visitor(const std::map<std::string, ASTProgramNode*>& programs, ASTProgramNode* main_program, const std::string& current_this_name)
	: programs(programs), main_program(main_program), current_program(main_program), curr_row(0), curr_col(0) {};

std::string default_namespace = "__main";

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
	"cp.std.utils"
};

std::map<std::string, std::shared_ptr<modules::Module>> built_in_libs = {
	{"cp.core.graphics", std::shared_ptr<modules::Graphics>(new modules::Graphics())},
	{"cp.core.files", std::shared_ptr<modules::Files>(new modules::Files())},
	{"cp.core.console", std::shared_ptr<modules::Console>(new modules::Console())},
	{"cp.core.datetime", std::shared_ptr<modules::DateTime>(new modules::DateTime())},
	{"cp.core.input", std::shared_ptr<modules::Input>(new modules::Input())}
};
