#ifndef LIBFINDER_HPP
#define LIBFINDER_HPP

#include "ast.hpp"
#include "cpsource.hpp"

namespace visitor {

	class Linker : Visitor {
	public:
		std::vector<std::string> lib_names;

	private:
		std::vector<std::string> libs;

	public:
		Linker(ASTProgramNode* main_program, const std::map<std::string, ASTProgramNode*>& programs);

		void start();

	private:
		void visit(ASTProgramNode*) override;
		void visit(ASTUsingNode*) override;
		void visit(ASTNamespaceManagerNode*) override;
		void visit(ASTDeclarationNode*) override;
		void visit(ASTUnpackedDeclarationNode*) override;
		void visit(ASTAssignmentNode*) override;
		void visit(ASTReturnNode*) override;
		void visit(ASTExitNode*) override;
		void visit(ASTBlockNode*) override;
		void visit(ASTContinueNode*) override;
		void visit(ASTBreakNode*) override;
		void visit(ASTSwitchNode*) override;
		void visit(ASTEnumNode*) override;
		void visit(ASTTryCatchNode*) override;
		void visit(ASTThrowNode*) override;
		void visit(ASTReticencesNode*) override;
		void visit(ASTElseIfNode*) override;
		void visit(ASTIfNode*) override;
		void visit(ASTForNode*) override;
		void visit(ASTForEachNode*) override;
		void visit(ASTWhileNode*) override;
		void visit(ASTDoWhileNode*) override;
		void visit(ASTFunctionDefinitionNode*) override;
		void visit(ASTStructDefinitionNode*) override;
		void visit(ASTLiteralNode<cp_bool>*) override;
		void visit(ASTLiteralNode<cp_int>*) override;
		void visit(ASTLiteralNode<cp_float>*) override;
		void visit(ASTLiteralNode<cp_char>*) override;
		void visit(ASTLiteralNode<cp_string>*) override;
		void visit(ASTFunctionExpression*) override;
		void visit(ASTArrayConstructorNode*) override;
		void visit(ASTStructConstructorNode*) override;
		void visit(ASTBinaryExprNode*) override;
		void visit(ASTUnaryExprNode*) override;
		void visit(ASTIdentifierNode*) override;
		void visit(ASTTernaryNode*) override;
		void visit(ASTInNode*) override;
		void visit(ASTFunctionCallNode*) override;
		void visit(ASTTypeParseNode*) override;
		void visit(ASTNullNode*) override;
		void visit(ASTThisNode*) override;
		void visit(ASTTypingNode*) override;

		long long hash(ASTExprNode*) override;
		long long hash(ASTIdentifierNode*) override;
		long long hash(ASTLiteralNode<cp_bool>*) override;
		long long hash(ASTLiteralNode<cp_int>*) override;
		long long hash(ASTLiteralNode<cp_float>*) override;
		long long hash(ASTLiteralNode<cp_char>*) override;
		long long hash(ASTLiteralNode<cp_string>*) override;

		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;
	};

}

#endif // !LIBFINDER_HPP
