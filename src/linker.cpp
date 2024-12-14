#include <filesystem>

#include "cputil.hpp"
#include "linker.hpp"
#include "utils.hpp"

using namespace visitor;
using namespace parser;

Linker::Linker(std::shared_ptr<ASTProgramNode> main_program, const std::map<std::string, std::shared_ptr<ASTProgramNode>>& programs)
	: Visitor(programs, main_program, main_program->name),
	libs(std::vector<std::string>()), lib_names(std::vector<std::string>()) {};

void Linker::start() {
	visit(current_program.top());
}

void Linker::visit(std::shared_ptr<ASTProgramNode> astnode) {
	for (auto& statement : astnode->statements) {
		if (std::dynamic_pointer_cast<ASTUsingNode>(statement)) {
			statement->accept(this);
		}
	}
}

void Linker::visit(std::shared_ptr<ASTUsingNode> astnode) {
	std::string libname = utils::StringUtils::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		std::string path = utils::StringUtils::replace(libname, ".", std::string{ std::filesystem::path::preferred_separator }) + ".cp";
		if (std::find(lib_names.begin(), lib_names.end(), path) == lib_names.end()) {
			lib_names.push_back(path);
		}
		return;
	}

	auto program = programs[libname];

	// if wasn't parsed yet
	if (!utils::CollectionUtils::contains(libs, libname)) {
		libs.push_back(libname);
		current_program.push(program);
		start();
		current_program.pop();
	}
}

void Linker::visit(std::shared_ptr<ASTNamespaceManagerNode>) {}

void Linker::visit(std::shared_ptr<ASTDeclarationNode>) {}
void Linker::visit(std::shared_ptr<ASTUnpackedDeclarationNode>) {}
void Linker::visit(std::shared_ptr<ASTAssignmentNode>) {}


void Linker::visit(std::shared_ptr<ASTBuiltinFunctionExecuterNode>) {}
void Linker::visit(std::shared_ptr<ASTFunctionCallNode>) {}
void Linker::visit(std::shared_ptr<ASTFunctionDefinitionNode>) {}

void Linker::visit(std::shared_ptr<ASTBlockNode>) {}

void Linker::visit(std::shared_ptr<ASTContinueNode>) {}
void Linker::visit(std::shared_ptr<ASTBreakNode>) {}
void Linker::visit(std::shared_ptr<ASTReturnNode>) {}
void Linker::visit(std::shared_ptr<ASTExitNode>) {}

void Linker::visit(std::shared_ptr<ASTEnumNode>) {}
void Linker::visit(std::shared_ptr<ASTTryCatchNode>) {}
void Linker::visit(std::shared_ptr<ASTThrowNode>) {}
void Linker::visit(std::shared_ptr<ASTReticencesNode>) {}
void Linker::visit(std::shared_ptr<ASTSwitchNode>) {}
void Linker::visit(std::shared_ptr<ASTElseIfNode>) {}
void Linker::visit(std::shared_ptr<ASTIfNode>) {}

void Linker::visit(std::shared_ptr<ASTForNode>) {}
void Linker::visit(std::shared_ptr<ASTForEachNode>) {}
void Linker::visit(std::shared_ptr<ASTWhileNode>) {}
void Linker::visit(std::shared_ptr<ASTDoWhileNode>) {}

void Linker::visit(std::shared_ptr<ASTBinaryExprNode>) {}
void Linker::visit(std::shared_ptr<ASTUnaryExprNode>) {}
void Linker::visit(std::shared_ptr<ASTTernaryNode>) {}
void Linker::visit(std::shared_ptr<ASTLiteralNode<cp_bool>>) {}
void Linker::visit(std::shared_ptr<ASTLiteralNode<cp_int>>) {}
void Linker::visit(std::shared_ptr<ASTLiteralNode<cp_float>>) {}
void Linker::visit(std::shared_ptr<ASTLiteralNode<cp_char>>) {}
void Linker::visit(std::shared_ptr<ASTLiteralNode<cp_string>>) {}
void Linker::visit(std::shared_ptr<ASTIdentifierNode>) {}
void Linker::visit(std::shared_ptr<ASTInNode>) {}

void Linker::visit(std::shared_ptr<ASTStructDefinitionNode>) {}
void Linker::visit(std::shared_ptr<ASTFunctionExpression>) {}
void Linker::visit(std::shared_ptr<ASTArrayConstructorNode>) {}
void Linker::visit(std::shared_ptr<ASTStructConstructorNode>) {}

void Linker::visit(std::shared_ptr<ASTTypeParseNode>) {}
void Linker::visit(std::shared_ptr<ASTTypingNode>) {}
void Linker::visit(std::shared_ptr<ASTNullNode>) {}
void Linker::visit(std::shared_ptr<ASTThisNode>) {}
void Linker::visit(std::shared_ptr<ASTValueNode>) {}

long long Linker::hash(std::shared_ptr<ASTExprNode>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTValueNode>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTLiteralNode<cp_bool>>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTLiteralNode<cp_int>>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTLiteralNode<cp_float>>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTLiteralNode<cp_char>>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTLiteralNode<cp_string>>) { return 0; }
long long Linker::hash(std::shared_ptr<ASTIdentifierNode>) { return 0; }

void Linker::set_curr_pos(unsigned int, unsigned int) {}
std::string Linker::msg_header() { return ""; }
