#include <filesystem>

#include "cputil.hpp"
#include "linker.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;
using namespace parser;

Linker::Linker(ASTProgramNode* main_program, const std::map<std::string, ASTProgramNode*>& programs)
	: Visitor(programs, main_program, main_program->name),
	libs(std::vector<std::string>()), lib_names(std::vector<std::string>()) {};

void Linker::start() {
	visit(current_program.top());
}

void Linker::visit(ASTProgramNode* astnode) {
	for (auto& statement : astnode->statements) {
		if (dynamic_cast<ASTUsingNode*>(statement)) {
			statement->accept(this);
		}
	}
}

void Linker::visit(ASTUsingNode* astnode) {
	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		std::string path = axe::StringUtils::replace(libname, ".", std::string{ std::filesystem::path::preferred_separator }) + ".cp";
		if (std::find(lib_names.begin(), lib_names.end(), path) == lib_names.end()) {
			lib_names.push_back(path);
		}
		return;
	}

	auto program = programs[libname];

	// if wasn't parsed yet
	if (!axe::CollectionUtils::contains(libs, libname)) {
		libs.push_back(libname);
		current_program.push(program);
		start();
		current_program.pop();
	}
}

void Linker::visit(ASTNamespaceManagerNode*) {}

void Linker::visit(ASTDeclarationNode*) {}
void Linker::visit(ASTUnpackedDeclarationNode*) {}
void Linker::visit(ASTAssignmentNode*) {}

void Linker::visit(ASTFunctionCallNode*) {}
void Linker::visit(ASTFunctionDefinitionNode*) {}

void Linker::visit(ASTBlockNode*) {}

void Linker::visit(ASTContinueNode*) {}
void Linker::visit(ASTBreakNode*) {}
void Linker::visit(ASTReturnNode*) {}
void Linker::visit(ASTExitNode*) {}

void Linker::visit(ASTEnumNode*) {}
void Linker::visit(ASTTryCatchNode*) {}
void Linker::visit(ASTThrowNode*) {}
void Linker::visit(ASTReticencesNode*) {}
void Linker::visit(ASTSwitchNode*) {}
void Linker::visit(ASTElseIfNode*) {}
void Linker::visit(ASTIfNode*) {}

void Linker::visit(ASTForNode*) {}
void Linker::visit(ASTForEachNode*) {}
void Linker::visit(ASTWhileNode*) {}
void Linker::visit(ASTDoWhileNode*) {}

void Linker::visit(ASTBinaryExprNode*) {}
void Linker::visit(ASTUnaryExprNode*) {}
void Linker::visit(ASTTernaryNode*) {}
void Linker::visit(ASTLiteralNode<cp_bool>*) {}
void Linker::visit(ASTLiteralNode<cp_int>*) {}
void Linker::visit(ASTLiteralNode<cp_float>*) {}
void Linker::visit(ASTLiteralNode<cp_char>*) {}
void Linker::visit(ASTLiteralNode<cp_string>*) {}
void Linker::visit(ASTIdentifierNode*) {}
void Linker::visit(ASTInNode*) {}

void Linker::visit(ASTStructDefinitionNode*) {}
void Linker::visit(ASTFunctionExpression*) {}
void Linker::visit(ASTArrayConstructorNode*) {}
void Linker::visit(ASTStructConstructorNode*) {}

void Linker::visit(ASTTypeParseNode*) {}
void Linker::visit(ASTTypingNode*) {}
void Linker::visit(ASTNullNode*) {}
void Linker::visit(ASTThisNode*) {}
void Linker::visit(ASTValueNode*) {}

long long Linker::hash(ASTExprNode*) { return 0; }
long long Linker::hash(ASTValueNode*) { return 0; }
long long Linker::hash(ASTLiteralNode<cp_bool>*) { return 0; }
long long Linker::hash(ASTLiteralNode<cp_int>*) { return 0; }
long long Linker::hash(ASTLiteralNode<cp_float>*) { return 0; }
long long Linker::hash(ASTLiteralNode<cp_char>*) { return 0; }
long long Linker::hash(ASTLiteralNode<cp_string>*) { return 0; }
long long Linker::hash(ASTIdentifierNode*) { return 0; }

void Linker::set_curr_pos(unsigned int, unsigned int) {}
std::string Linker::msg_header() { return ""; }
