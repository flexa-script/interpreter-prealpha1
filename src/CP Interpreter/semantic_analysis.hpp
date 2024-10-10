#ifndef SEMANTIC_ANALYSIS_HPP
#define SEMANTIC_ANALYSIS_HPP

#include <memory>
#include <map>
#include <vector>
#include <stack>
#include <xutility>
#include <functional>

#include "ast.hpp"
#include "semantic_scope.hpp"

using namespace visitor;
using namespace parser;

namespace visitor {

	class SemanticAnalyser : Visitor {
	public:
		std::map<std::string, void*> builtin_functions;
		std::map<std::string, std::vector<std::shared_ptr<SemanticScope>>> scopes;

	private:
		dim_eval_func_t evaluate_access_vector_ptr = std::bind(&SemanticAnalyser::evaluate_access_vector, this, std::placeholders::_1);
		std::vector<std::string> nmspaces;
		std::vector<std::string> parsed_libs;
		std::string current_namespace;
		SemanticValue current_expression;
		std::stack<FunctionDefinition> current_function;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		bool exception = false;
		bool is_switch = false;
		bool is_loop = false;

		std::vector<ASTExprNode*> current_expression_array_dim;
		int current_expression_array_dim_max;
		TypeDefinition current_expression_array_type;
		bool is_max;

	private:
		bool returns(ASTNode* astnode);

		void equals_value(const SemanticValue& lval, const SemanticValue& rval);

		std::vector<unsigned int> evaluate_access_vector(const std::vector<ASTExprNode*>& expr_access_vector);

		std::shared_ptr<SemanticScope> get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier);
		std::shared_ptr<SemanticScope> get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier);
		std::shared_ptr<SemanticScope> get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>* signature, bool strict = true);

		TypeDefinition do_operation(const std::string& op, TypeDefinition lvtype, TypeDefinition ltype, TypeDefinition* rvtype, TypeDefinition rtype, bool is_expr = true);
		std::shared_ptr<SemanticValue> access_value(std::shared_ptr<SemanticValue> value, const std::vector<Identifier>& identifier_vector, size_t i = 0);

		bool namespace_exists(const std::string& nmspace);
		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const ASTProgramNode* program, const std::string& nmspace = "") const override;

		void check_is_struct_exists(Type type, const std::string& nmspace, const std::string& identifier);

		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;

	public:
		SemanticAnalyser(std::shared_ptr<SemanticScope> global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs);
		~SemanticAnalyser() = default;

		void start();

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
	};
}

#endif // !SEMANTIC_ANALYSIS_HPP
