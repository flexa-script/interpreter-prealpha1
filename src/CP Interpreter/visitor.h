#ifndef VISITOR_H
#define VISITOR_H

#include <string>
#include <vector>
#include <any>

#if defined(_WIN32) || defined(WIN32)
typedef __int64 __int64_t;
#endif

namespace parser {
	class ASTProgramNode;

	class ASTUsingNode;
	class ASTDeclarationNode;
	class ASTAssignmentNode;
	class ASTPrintNode;
	class ASTReadNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTIfNode;
	class ASTWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;

	template <typename T> class ASTLiteralNode;
	class ASTBinaryExprNode;
	class ASTIdentifierNode;
	class ASTUnaryExprNode;
	class ASTExprFunctionCallNode;
	class ASTFloatParseNode;
	class ASTIntParseNode;
	class ASTStringParseNode;
	class ASTExprReadNode;
	class ASTThisNode;
}

namespace visitor {
	class Visitor {
	public:
		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTPrintNode*) = 0;
		virtual void visit(parser::ASTReadNode*) = 0;
		virtual void visit(parser::ASTFunctionCallNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTIfNode*) = 0;
		virtual void visit(parser::ASTWhileNode*) = 0;
		virtual void visit(parser::ASTFunctionDefinitionNode*) = 0;
		virtual void visit(parser::ASTStructDefinitionNode*) = 0;
		virtual void visit(parser::ASTLiteralNode<bool>*) = 0;
		virtual void visit(parser::ASTLiteralNode<__int64_t>*) = 0;
		virtual void visit(parser::ASTLiteralNode<long double>*) = 0;
		virtual void visit(parser::ASTLiteralNode<char>*) = 0;
		virtual void visit(parser::ASTLiteralNode<std::string>*) = 0;
		virtual void visit(parser::ASTLiteralNode<std::any>*) = 0;
		virtual void visit(parser::ASTBinaryExprNode*) = 0;
		virtual void visit(parser::ASTIdentifierNode*) = 0;
		virtual void visit(parser::ASTUnaryExprNode*) = 0;
		virtual void visit(parser::ASTExprFunctionCallNode*) = 0;
		virtual void visit(parser::ASTFloatParseNode*) = 0;
		virtual void visit(parser::ASTIntParseNode*) = 0;
		virtual void visit(parser::ASTStringParseNode*) = 0;
		virtual void visit(parser::ASTExprReadNode*) = 0;
		virtual void visit(parser::ASTThisNode*) = 0;
	};
}

#endif //VISITOR_H
