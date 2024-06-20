#ifndef LIBPREPROCESSOR_HPP
#define LIBPREPROCESSOR_HPP

#include "ast.hpp"
#include "cpsource.hpp"


namespace visitor {

	class LibFinder : Visitor {
	public:
		std::string cp_root;
		std::vector<std::string> lib_names;

	private:
		std::vector<std::string> libs;

	public:
		LibFinder(parser::ASTProgramNode*, std::map<std::string, parser::ASTProgramNode*>);

		void start();

	private:
		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTAsNamespaceNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTExitNode*) override;
		void visit(parser::ASTBlockNode*) override;
		void visit(parser::ASTContinueNode*) override;
		void visit(parser::ASTBreakNode*) override;
		void visit(parser::ASTSwitchNode*) override;
		void visit(parser::ASTEnumNode*) override;
		void visit(parser::ASTTryCatchNode*) override;
		void visit(parser::ASTThrowNode*) override;
		void visit(parser::ASTReticencesNode*) override;
		void visit(parser::ASTElseIfNode*) override;
		void visit(parser::ASTIfNode*) override;
		void visit(parser::ASTForNode*) override;
		void visit(parser::ASTForEachNode*) override;
		void visit(parser::ASTWhileNode*) override;
		void visit(parser::ASTDoWhileNode*) override;
		void visit(parser::ASTFunctionDefinitionNode*) override;
		void visit(parser::ASTStructDefinitionNode*) override;
		void visit(parser::ASTLiteralNode<cp_bool>*) override;
		void visit(parser::ASTLiteralNode<cp_int>*) override;
		void visit(parser::ASTLiteralNode<cp_float>*) override;
		void visit(parser::ASTLiteralNode<cp_char>*) override;
		void visit(parser::ASTLiteralNode<cp_string>*) override;
		void visit(parser::ASTArrayConstructorNode*) override;
		void visit(parser::ASTStructConstructorNode*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTTernaryNode*) override;
		void visit(parser::ASTInNode*) override;
		void visit(parser::ASTFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;
		void visit(parser::ASTTypingNode*) override;

		long long hash(parser::ASTExprNode*) override;
		long long hash(parser::ASTIdentifierNode*) override;
		long long hash(parser::ASTLiteralNode<cp_bool>*) override;
		long long hash(parser::ASTLiteralNode<cp_int>*) override;
		long long hash(parser::ASTLiteralNode<cp_float>*) override;
		long long hash(parser::ASTLiteralNode<cp_char>*) override;
		long long hash(parser::ASTLiteralNode<cp_string>*) override;

		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const override;
		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;
	};

}

#endif // !LIBPREPROCESSOR_HPP
