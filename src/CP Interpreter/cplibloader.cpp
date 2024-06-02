#include <filesystem>

#include "cplibloader.hpp"
#include "vendor/util.hpp"
#include "cputil.hpp"


using namespace visitor;
using namespace parser;


LibFinder::LibFinder(ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: cp_path(axe::Util::get_current_path()), libs(std::vector<std::string>()), lib_names(std::vector<std::string>()),
	Visitor(programs, main_program, main_program->name) {};

void LibFinder::start() {
	visit(current_program);
}

void LibFinder::visit(ASTProgramNode* astnode) {
	for (auto& statement : astnode->statements) {
		if (!dynamic_cast<ASTUsingNode*>(statement)) {
			return;
		}
		statement->accept(this);
	}
}

void LibFinder::visit(ASTUsingNode* astnode) {
	std::string libname = axe::Util::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		// find in cp std libs
		std::string path = cp_path + axe::Util::replace(libname, ".", "/") + ".cp";
		if (std::filesystem::exists(path)) {
			if (std::find(lib_names.begin(), lib_names.end(), path) == lib_names.end()) {
				lib_names.push_back(path);
			}
			return;
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

std::vector<std::string> LibFinder::get_lib_names() {
	return lib_names;
}

void LibFinder::visit(ASTAsNamespaceNode*) {}

void LibFinder::visit(ASTDeclarationNode*) {}
void LibFinder::visit(ASTAssignmentNode*) {}

void LibFinder::visit(ASTFunctionCallNode*) {}
void LibFinder::visit(ASTFunctionDefinitionNode*) {}

void LibFinder::visit(ASTBlockNode*) {}

void LibFinder::visit(ASTContinueNode*) {}
void LibFinder::visit(ASTBreakNode*) {}
void LibFinder::visit(ASTReturnNode*) {}
void LibFinder::visit(ASTExitNode*) {}

void LibFinder::visit(ASTEnumNode*) {}
void LibFinder::visit(ASTTryCatchNode*) {}
void LibFinder::visit(ASTThrowNode*) {}
void LibFinder::visit(ASTSwitchNode*) {}
void LibFinder::visit(ASTElseIfNode*) {}
void LibFinder::visit(ASTIfNode*) {}

void LibFinder::visit(ASTForNode*) {}
void LibFinder::visit(ASTForEachNode*) {}
void LibFinder::visit(ASTWhileNode*) {}
void LibFinder::visit(ASTDoWhileNode*) {}

void LibFinder::visit(ASTBinaryExprNode*) {}
void LibFinder::visit(ASTUnaryExprNode*) {}
void LibFinder::visit(ASTLiteralNode<cp_bool>*) {}
void LibFinder::visit(ASTLiteralNode<cp_int>*) {}
void LibFinder::visit(ASTLiteralNode<cp_float>*) {}
void LibFinder::visit(ASTLiteralNode<cp_char>*) {}
void LibFinder::visit(ASTLiteralNode<cp_string>*) {}
void LibFinder::visit(ASTIdentifierNode*) {}

void LibFinder::visit(ASTStructDefinitionNode*) {}

void LibFinder::visit(ASTArrayConstructorNode*) {}
void LibFinder::visit(ASTStructConstructorNode*) {}

void LibFinder::visit(ASTTypeParseNode*) {}
void LibFinder::visit(ASTTypingNode*) {}
void LibFinder::visit(ASTNullNode*) {}
void LibFinder::visit(ASTThisNode*) {}

unsigned int LibFinder::hash(ASTExprNode*) { return 0; }
unsigned int LibFinder::hash(ASTLiteralNode<cp_bool>*) { return 0; }
unsigned int LibFinder::hash(ASTLiteralNode<cp_int>*) { return 0; }
unsigned int LibFinder::hash(ASTLiteralNode<cp_float>*) { return 0; }
unsigned int LibFinder::hash(ASTLiteralNode<cp_char>*) { return 0; }
unsigned int LibFinder::hash(ASTLiteralNode<cp_string>*) { return 0; }
unsigned int LibFinder::hash(ASTIdentifierNode*) { return 0; }

std::string LibFinder::get_namespace(std::string) { return ""; }
std::string LibFinder::get_namespace(ASTProgramNode*, std::string) { return ""; }
