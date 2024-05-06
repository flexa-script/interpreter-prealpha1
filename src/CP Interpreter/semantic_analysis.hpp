#ifndef SEMANTIC_ANALYSIS_HPP
#define SEMANTIC_ANALYSIS_HPP

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "ast.hpp"
#include "semantic_scope.hpp"


namespace visitor {

	class SemanticAnalyser : Visitor {
	private:
		std::vector<SemanticScope*> scopes;
		//parser::SemanticVariable_t current_variable_expression; // maybe to get the real type?
		parser::SemanticValue_t current_expression;
		std::stack<parser::FunctionDefinition_t> current_function;
		std::string retfun_identifier;
		parser::SemanticValue_t retfun_expression;
		bool retfun_has_return;
		bool retfun_returned;

	private:
		bool returns(parser::ASTNode*);

		void assign_structure(SemanticScope*, parser::SemanticValue_t*, parser::ASTStructConstructorNode*);
		void declare_structure();

		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		std::vector<unsigned int> calculate_array_dim_size(parser::ASTArrayConstructorNode*);
		void determine_array_type(parser::ASTArrayConstructorNode*);
		void check_array_type(parser::ASTExprNode*, unsigned int, unsigned int);
		parser::SemanticValue_t* access_value(SemanticScope*, parser::SemanticValue_t*, std::vector<parser::Identifier_t>, size_t = 0, bool = false);

		std::string msg_header(unsigned int, unsigned int);

	public:
		SemanticAnalyser(SemanticScope*, std::vector<parser::ASTProgramNode*>);
		~SemanticAnalyser();

		void start();

		std::string get_namespace() override;

		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTPrintNode*) override;
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTBlockNode*) override;
		void visit(parser::ASTContinueNode*) override;
		void visit(parser::ASTBreakNode*) override;
		void visit(parser::ASTSwitchNode*) override;
		void visit(parser::ASTElseIfNode*) override;
		void visit(parser::ASTIfNode*) override;
		void visit(parser::ASTForNode*) override;
		void visit(parser::ASTForEachNode*) override;
		void visit(parser::ASTWhileNode*) override;
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
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTTypeNode*) override;
		void visit(parser::ASTLenNode*) override;
		void visit(parser::ASTRoundNode*) override;
		void visit(parser::ASTReadNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;

		unsigned int hash(parser::ASTIdentifierNode*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_bool>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_int>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_float>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_char>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_string>*) override;
	};
}

#endif // SEMANTIC_ANALYSIS_HPP
