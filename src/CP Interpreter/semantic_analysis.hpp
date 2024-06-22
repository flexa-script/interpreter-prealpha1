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
		std::vector<std::string> parsed_libs;
		std::string current_namespace;
		parser::SemanticValue current_expression;
		std::stack<parser::FunctionDefinition> current_function;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		bool exception = false;
		bool is_switch = false;
		bool is_loop = false;

	private:
		bool returns(parser::ASTNode* astnode);

		void equals_value(const parser::SemanticValue& lval, const parser::SemanticValue& rval);
		void validate_struct_assign(SemanticScope* curr_scope, parser::SemanticValue* expression, parser::ASTStructConstructorNode* expr);

		std::vector<unsigned int> evaluate_access_vector(const std::vector<parser::ASTExprNode*>& expr_access_vector);
		std::vector<unsigned int> calculate_array_dim_size(parser::ASTArrayConstructorNode*);

		SemanticScope* get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier);
		SemanticScope* get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier);
		SemanticScope* get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<parser::TypeDefinition>& signature);

		void determine_array_type(parser::ASTArrayConstructorNode* astnode);
		void check_array_type(parser::ASTExprNode* astnode, unsigned int row, unsigned int col);

		parser::VariableDefinition access_struct_variable(std::vector<parser::Identifier> identifier_vector, std::string type_name, std::string nmspace, unsigned int i = 0);

		void register_built_in_functions();

		bool namespace_exists(const std::string& nmspace);
		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const override;

		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;

	public:
		SemanticAnalyser(SemanticScope* global_scope, parser::ASTProgramNode* main_program, std::map<std::string, parser::ASTProgramNode*> programs);
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
