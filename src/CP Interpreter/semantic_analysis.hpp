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
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* current_program;
		std::vector<SemanticScope*> scopes;
		std::stack<parser::Type> functions;
		parser::Type current_expression_type;
		parser::Type current_expression_array_type;
		std::string current_expression_type_name;
		bool is_function_definition_context;
		bool current_expression_is_constant;
		int array_level = 0;
		std::vector<parser::VariableDefinition_t> current_function_parameters;

	private:
		bool is_any(parser::Type);

		bool returns(parser::ASTNode*);

		void assign_structure(std::string, parser::ASTStructConstructorNode*);
		void declare_structure(std::string, std::string);

		parser::VariableDefinition_t find_declared_variable_recursively(std::string);

		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		std::vector<unsigned int> calculate_array_dim_size(parser::ASTArrayConstructorNode*);
		void determine_array_type(parser::ASTArrayConstructorNode*);
		void check_array_type(parser::ASTExprNode*, unsigned int, unsigned int);

		std::string msg_header(unsigned int, unsigned int);

	public:
		SemanticAnalyser(SemanticScope*, std::vector<parser::ASTProgramNode*>);
		~SemanticAnalyser();

		void start();
		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTPrintNode*) override;
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTBlockNode*) override;
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
