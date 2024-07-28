#include "visitor.hpp"

using namespace visitor;

Visitor::Visitor(const std::map<std::string, ASTProgramNode*>& programs, ASTProgramNode* main_program, const std::string& current_this_name)
	: programs(programs), main_program(main_program), current_program(main_program), curr_row(0), curr_col(0) {};
