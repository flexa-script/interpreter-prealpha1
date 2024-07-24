#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

#include "types.hpp"

namespace parser {
	class ASTProgramNode;

	class ASTUsingNode;
	class ASTAsNamespaceNode;
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
extern std::vector<std::string> built_in_libs;

namespace visitor {

	class Visitor {
	public:
		std::map<std::string, parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* main_program;
		parser::ASTProgramNode* current_program;
		int curr_row;
		int curr_col;

		Visitor(const std::map<std::string, parser::ASTProgramNode*>& programs, parser::ASTProgramNode* main_program, const std::string& current_this_name);

		virtual const std::string& get_namespace(const std::string& nmspace = "") const = 0;
		virtual const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const = 0;

		virtual void set_curr_pos(unsigned int row, unsigned int col) = 0;
		virtual std::string msg_header() = 0;

		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTAsNamespaceNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTUnpackedDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTContinueNode*) = 0;
		virtual void visit(parser::ASTBreakNode*) = 0;
		virtual void visit(parser::ASTExitNode*) = 0;
		virtual void visit(parser::ASTSwitchNode*) = 0;
		virtual void visit(parser::ASTElseIfNode*) = 0;
		virtual void visit(parser::ASTEnumNode*) = 0;
		virtual void visit(parser::ASTTryCatchNode*) = 0;
		virtual void visit(parser::ASTThrowNode*) = 0;
		virtual void visit(parser::ASTReticencesNode*) = 0;
		virtual void visit(parser::ASTIfNode*) = 0;
		virtual void visit(parser::ASTForNode*) = 0;
		virtual void visit(parser::ASTForEachNode*) = 0;
		virtual void visit(parser::ASTWhileNode*) = 0;
		virtual void visit(parser::ASTDoWhileNode*) = 0;
		virtual void visit(parser::ASTFunctionDefinitionNode*) = 0;
		virtual void visit(parser::ASTStructDefinitionNode*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_string>*) = 0;
		virtual void visit(parser::ASTFunctionExpression*) = 0;
		virtual void visit(parser::ASTArrayConstructorNode*) = 0;
		virtual void visit(parser::ASTStructConstructorNode*) = 0;
		virtual void visit(parser::ASTBinaryExprNode*) = 0;
		virtual void visit(parser::ASTUnaryExprNode*) = 0;
		virtual void visit(parser::ASTIdentifierNode*) = 0;
		virtual void visit(parser::ASTTernaryNode*) = 0;
		virtual void visit(parser::ASTInNode*) = 0;
		virtual void visit(parser::ASTFunctionCallNode*) = 0;
		virtual void visit(parser::ASTTypeParseNode*) = 0;
		virtual void visit(parser::ASTNullNode*) = 0;
		virtual void visit(parser::ASTThisNode*) = 0;
		virtual void visit(parser::ASTTypingNode*) = 0;

		virtual long long hash(parser::ASTExprNode*) = 0;
		virtual long long hash(parser::ASTIdentifierNode*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_string>*) = 0;
	};
}

#endif // !VISITOR_HPP
