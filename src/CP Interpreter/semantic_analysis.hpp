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
		std::map<std::string, std::vector<SemanticScope*>> scopes;
		std::vector<std::string> libs;
		std::string current_namespace;
		parser::SemanticValue_t current_expression;
		std::stack<parser::FunctionDefinition_t> current_function;

	private:
		bool returns(parser::ASTNode*);

		void validate_struct_assign(SemanticScope*, parser::SemanticValue_t*, parser::ASTStructConstructorNode*);
		void declare_structure();

		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		std::vector<unsigned int> calculate_array_dim_size(parser::ASTArrayConstructorNode*);

		SemanticScope* get_inner_most_variable_scope(std::string, std::string);
		SemanticScope* get_inner_most_function_scope(std::string nmspace, std::string identifier, std::vector<parser::Type> signature);
		SemanticScope* get_inner_most_struct_definition_scope(std::string, std::string);

		void determine_array_type(parser::ASTArrayConstructorNode*);
		void check_array_type(parser::ASTExprNode*, unsigned int, unsigned int);

		parser::VariableDefinition_t access_struct_variable(std::vector<parser::Identifier_t>, std::string, unsigned int = 0);

		std::string msg_header(unsigned int, unsigned int);

	public:
		SemanticAnalyser(SemanticScope*, parser::ASTProgramNode*, std::map<std::string, parser::ASTProgramNode*>);
		~SemanticAnalyser() = default;

		void start();

		std::string get_namespace(std::string = "") override;
		std::string get_namespace(parser::ASTProgramNode*, std::string = "") override;

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

		unsigned int hash(parser::ASTExprNode*) override;
		unsigned int hash(parser::ASTIdentifierNode*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_bool>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_int>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_float>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_char>*) override;
		unsigned int hash(parser::ASTLiteralNode<cp_string>*) override;
	};
}

#endif // SEMANTIC_ANALYSIS_HPP
