#ifndef SEMANTIC_ANALYSIS_HPP
#define SEMANTIC_ANALYSIS_HPP

#include <map>
#include <vector>
#include <stack>
#include <xutility>
#include <functional>

#include "ast.hpp"
#include "semantic_scope.hpp"


namespace visitor {

	class SemanticAnalyser : Visitor {
	private:
		std::map<std::string, std::vector<SemanticScope*>> scopes;
		std::vector<std::string> nmspaces;
		std::vector<std::string> libs;
		std::string current_namespace;
		parser::SemanticValue current_expression;
		std::stack<parser::FunctionDefinition> current_function;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		bool exception = false;
		bool is_switch = false;
		bool is_loop = false;

	private:
		bool returns(parser::ASTNode*);

		void validate_struct_assign(SemanticScope*, parser::SemanticValue*, parser::ASTStructConstructorNode*);
		void declare_structure();

		std::vector<unsigned int> evaluate_access_vector(std::vector<parser::ASTExprNode*>);
		std::vector<unsigned int> calculate_array_dim_size(parser::ASTArrayConstructorNode*);

		bool namespace_exists(std::string nmspace);
		SemanticScope* get_inner_most_variable_scope(std::string, std::string);
		SemanticScope* get_inner_most_function_scope(std::string, std::string, std::vector<parser::TypeDefinition>);
		SemanticScope* get_inner_most_struct_definition_scope(std::string, std::string);

		void determine_array_type(parser::ASTArrayConstructorNode*);
		void check_array_type(parser::ASTExprNode*, unsigned int, unsigned int);

		parser::VariableDefinition access_struct_variable(std::vector<parser::Identifier>, std::string, std::string, unsigned int = 0);

		void register_built_in_functions();
		void register_built_in_lib(std::string libname);

		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const override;

		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;

	public:
		SemanticAnalyser(SemanticScope*, parser::ASTProgramNode*, std::map<std::string, parser::ASTProgramNode*>);
		~SemanticAnalyser() = default;

		void start();

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
	};
}

#endif // !SEMANTIC_ANALYSIS_HPP
