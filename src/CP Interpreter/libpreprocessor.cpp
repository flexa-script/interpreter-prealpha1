#include <filesystem>

#include "libpreprocessor.hpp"
#include "util.hpp"


using namespace visitor;
using namespace parser;


LibPreprocessor::LibPreprocessor(ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: cp_path(), libs(std::vector<std::string>()), lib_names(std::vector<std::string>()),
	Visitor(programs, main_program, main_program->name) {};

void LibPreprocessor::start() {
	visit(current_program);
}

void LibPreprocessor::visit(ASTProgramNode* astnode) {
	cp_path = axe::Util::get_current_path();

	for (auto& statement : astnode->statements) {
		if (!dynamic_cast<ASTUsingNode*>(statement)) {
			return;
		}
		statement->accept(this);
	}
}

void LibPreprocessor::visit(ASTUsingNode* astnode) {
	std::string libname = axe::Util::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		// find in cp std libs
		std::string path = cp_path + "\\" + axe::Util::replace(libname, ".", "\\") + ".cp";
		if (std::filesystem::exists(path)) {
			lib_names.push_back(path);
		}
	}

	auto program = programs[libname];

	// if was'nt parsed yet
	if (!axe::Util::contains(libs, libname)) {
		libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		start();
		current_program = prev_program;
	}
}

std::vector<std::string> LibPreprocessor::get_lib_names() {
	return lib_names;
}

void LibPreprocessor::visit(ASTDeclarationNode*) {}
void LibPreprocessor::visit(ASTAssignmentNode*) {}

void LibPreprocessor::visit(ASTFunctionCallNode*) {}
void LibPreprocessor::visit(ASTFunctionDefinitionNode*) {}

void LibPreprocessor::visit(ASTBlockNode*) {}

void LibPreprocessor::visit(ASTContinueNode*) {}
void LibPreprocessor::visit(ASTBreakNode*) {}
void LibPreprocessor::visit(ASTReturnNode*) {}

void LibPreprocessor::visit(ASTSwitchNode*) {}
void LibPreprocessor::visit(ASTElseIfNode*) {}
void LibPreprocessor::visit(ASTIfNode*) {}

void LibPreprocessor::visit(ASTForNode*) {}
void LibPreprocessor::visit(ASTForEachNode*) {}
void LibPreprocessor::visit(ASTWhileNode*) {}

void LibPreprocessor::visit(ASTBinaryExprNode*) {}
void LibPreprocessor::visit(ASTUnaryExprNode*) {}
void LibPreprocessor::visit(ASTLiteralNode<cp_bool>*) {}
void LibPreprocessor::visit(ASTLiteralNode<cp_int>*) {}
void LibPreprocessor::visit(ASTLiteralNode<cp_float>*) {}
void LibPreprocessor::visit(ASTLiteralNode<cp_char>*) {}
void LibPreprocessor::visit(ASTLiteralNode<cp_string>*) {}
void LibPreprocessor::visit(ASTIdentifierNode*) {}

void LibPreprocessor::visit(ASTStructDefinitionNode*) {}

void LibPreprocessor::visit(ASTArrayConstructorNode*) {}
void LibPreprocessor::visit(ASTStructConstructorNode*) {}

unsigned int LibPreprocessor::hash(ASTExprNode*) { return 0; }
unsigned int LibPreprocessor::hash(ASTLiteralNode<cp_bool>*) { return 0; }
unsigned int LibPreprocessor::hash(ASTLiteralNode<cp_int>*) { return 0; }
unsigned int LibPreprocessor::hash(ASTLiteralNode<cp_float>*) { return 0; }
unsigned int LibPreprocessor::hash(ASTLiteralNode<cp_char>*) { return 0; }
unsigned int LibPreprocessor::hash(ASTLiteralNode<cp_string>*) { return 0; }
unsigned int LibPreprocessor::hash(ASTIdentifierNode*) { return 0; }

void LibPreprocessor::visit(ASTTypeParseNode*) {}
void LibPreprocessor::visit(ASTTypeofNode*) {}
void LibPreprocessor::visit(ASTNullNode*) {}
void LibPreprocessor::visit(ASTThisNode*) {}

std::string LibPreprocessor::get_namespace(std::string) { return ""; }
std::string LibPreprocessor::get_namespace(ASTProgramNode*, std::string) { return ""; }
