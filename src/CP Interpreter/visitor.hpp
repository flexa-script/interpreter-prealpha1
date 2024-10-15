#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

#include "types.hpp"
#include "module.hpp"

namespace parser {
	class ASTProgramNode;

	class ASTUsingNode;
	class ASTNamespaceManagerNode;
	class ASTDeclarationNode;
	class ASTUnpackedDeclarationNode;
	class ASTAssignmentNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTContinueNode;
	class ASTBreakNode;
	class ASTSwitchNode;
	class ASTExitNode;
	class ASTEnumNode;
	class ASTElseIfNode;
	class ASTIfNode;
	class ASTTryCatchNode;
	class ASTThrowNode;
	class ASTReticencesNode;
	class ASTForNode;
	class ASTForEachNode;
	class ASTWhileNode;
	class ASTDoWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;
	template <typename T> class ASTLiteralNode;
	class ASTFunctionExpression;
	class ASTExprNode;
	class ASTArrayConstructorNode;
	class ASTStructConstructorNode;
	class ASTBinaryExprNode;
	class ASTUnaryExprNode;
	class ASTIdentifierNode;
	class ASTTernaryNode;
	class ASTInNode;
	class ASTFunctionCallNode;
	class ASTTypeParseNode;
	class ASTNullNode;
	class ASTThisNode;
	class ASTTypingNode;
}

using namespace parser;

extern std::string default_namespace;
extern std::vector<std::string> std_libs;
extern std::map<std::string, std::shared_ptr<modules::Module>> built_in_libs;

namespace visitor {

	class Visitor {
	public:
		std::map<std::string, ASTProgramNode*> programs;
		ASTProgramNode* main_program;
		std::stack<ASTProgramNode*> current_program;
		std::vector<std::string> parsed_libs;
		int curr_row;
		int curr_col;

		Visitor(const std::map<std::string, ASTProgramNode*>& programs, ASTProgramNode* main_program, const std::string& current_this_name);

		virtual void set_curr_pos(unsigned int row, unsigned int col) = 0;
		virtual std::string msg_header() = 0;

		virtual void visit(ASTProgramNode*) = 0;
		virtual void visit(ASTUsingNode*) = 0;
		virtual void visit(ASTNamespaceManagerNode*) = 0;
		virtual void visit(ASTDeclarationNode*) = 0;
		virtual void visit(ASTUnpackedDeclarationNode*) = 0;
		virtual void visit(ASTAssignmentNode*) = 0;
		virtual void visit(ASTReturnNode*) = 0;
		virtual void visit(ASTBlockNode*) = 0;
		virtual void visit(ASTContinueNode*) = 0;
		virtual void visit(ASTBreakNode*) = 0;
		virtual void visit(ASTExitNode*) = 0;
		virtual void visit(ASTSwitchNode*) = 0;
		virtual void visit(ASTElseIfNode*) = 0;
		virtual void visit(ASTEnumNode*) = 0;
		virtual void visit(ASTTryCatchNode*) = 0;
		virtual void visit(ASTThrowNode*) = 0;
		virtual void visit(ASTReticencesNode*) = 0;
		virtual void visit(ASTIfNode*) = 0;
		virtual void visit(ASTForNode*) = 0;
		virtual void visit(ASTForEachNode*) = 0;
		virtual void visit(ASTWhileNode*) = 0;
		virtual void visit(ASTDoWhileNode*) = 0;
		virtual void visit(ASTFunctionDefinitionNode*) = 0;
		virtual void visit(ASTStructDefinitionNode*) = 0;
		virtual void visit(ASTLiteralNode<cp_bool>*) = 0;
		virtual void visit(ASTLiteralNode<cp_int>*) = 0;
		virtual void visit(ASTLiteralNode<cp_float>*) = 0;
		virtual void visit(ASTLiteralNode<cp_char>*) = 0;
		virtual void visit(ASTLiteralNode<cp_string>*) = 0;
		virtual void visit(ASTFunctionExpression*) = 0;
		virtual void visit(ASTArrayConstructorNode*) = 0;
		virtual void visit(ASTStructConstructorNode*) = 0;
		virtual void visit(ASTBinaryExprNode*) = 0;
		virtual void visit(ASTUnaryExprNode*) = 0;
		virtual void visit(ASTIdentifierNode*) = 0;
		virtual void visit(ASTTernaryNode*) = 0;
		virtual void visit(ASTInNode*) = 0;
		virtual void visit(ASTFunctionCallNode*) = 0;
		virtual void visit(ASTTypeParseNode*) = 0;
		virtual void visit(ASTNullNode*) = 0;
		virtual void visit(ASTThisNode*) = 0;
		virtual void visit(ASTTypingNode*) = 0;
	};
}

#endif // !VISITOR_HPP
