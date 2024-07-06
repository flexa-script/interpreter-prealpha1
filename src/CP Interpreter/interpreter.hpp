#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
#include <stack>
#include <functional>

#include "visitor.hpp"
#include "ast.hpp"
#include "interpreter_scope.hpp"


using namespace visitor;
using namespace parser;


namespace modules {
	class Graphics;
	class Files;
	class Console;
}

namespace visitor {
	class Interpreter : public Visitor {
	public:
		std::map<std::string, std::function<void()>> builtin_functions;
		std::vector<Value*> builtin_arguments;
		Value current_expression_value;

		std::string parse_value_to_string(const Value* value);

	private:
		std::map<std::string, std::vector<InterpreterScope*>> scopes;
		std::vector<std::string> parsed_libs;
		std::string identifier_call_name;
		std::string function_call_name;
		interpreter_parameter_list_t function_call_parameters;
		std::stack<std::string> current_function_nmspace;
		std::stack<visitor::TypeDefinition> current_function_return_type;
		std::stack<std::string> current_name;
		std::string return_from_function_name;
		//Value* current_param_ref;
		Variable* current_var_ref;
		std::vector<Value*> last_function_arguments;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		bool is_function_context = false;
		bool return_from_function = false;
		bool is_switch = false;
		bool is_loop = false;
		bool exit_from_program = false;
		bool continue_block = false;
		bool break_block = false;
		bool executed_elif = false;
		bool has_string_access = false;
		bool exception = false;

		modules::Graphics* cpgraphics;
		modules::Files* cpfiles;
		modules::Console* cpconsole;


		std::vector<unsigned int> evaluate_access_vector(const std::vector<parser::ASTExprNode*>& expr_access_vector);
		std::vector<unsigned int> calculate_array_dim_size(const cp_array& arr);

		std::vector<Value*> build_array(const std::vector<parser::ASTExprNode*>& dim, Value* init_value, long long i);

		void declare_structure(cp_struct* str, const std::string& nmspace);

		bool is_any_or_match_type(TypeDefinition vtype, TypeDefinition ltype, TypeDefinition rtype);
		bool is_any_or_match_type(TypeDefinition ltype, TypeDefinition rtype);
		bool match_type_array(TypeDefinition ltype, TypeDefinition rtype);

		Variable* do_operation(const std::string& op, Variable* lval, Variable* rval, cp_int str_pos = 0);
		Value* do_operation(const std::string& op, Value* lval, Value* rval, bool is_expr = false, cp_int str_pos = 0);
		cp_int do_relational_operation(const std::string& op, Value* lval, Value* rval);
		cp_bool do_equality_operation(const std::string& op, Value* lval, Value* rval);
		cp_int do_operation(cp_int lval, cp_int rval, const std::string& op);
		cp_float do_operation(cp_float lval, cp_float rval, const std::string& op);
		cp_string do_operation(cp_string lval, cp_string rval, const std::string& op);
		cp_array do_operation(cp_array lval, cp_array rval, const std::string& op);

		std::string parse_array_to_string(const cp_array& arr_value);
		std::string parse_struct_to_string(const cp_struct& str_value);

		InterpreterScope* get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier);
		InterpreterScope* get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier);
		InterpreterScope* get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<visitor::TypeDefinition>& signature);

		Value* set_value(InterpreterScope* scope, const std::vector<parser::Identifier>& identifier_vector, Value* new_value);
		Value* access_value(const InterpreterScope* scope, Value* value, const std::vector<parser::Identifier>& identifier_vector, size_t i = 0);

		void call_builtin_function(const std::string& identifier);
		void declare_function_block_parameters(const std::string& nmspace);
		void register_built_in_functions();
		void register_built_in_lib(const std::string& libname);

		bool equals_value(const Value* lval, const Value* rval);
		bool equals_array(const cp_array& larr, const cp_array& rarr);
		bool equals_struct(const cp_struct* lstr, const cp_struct* rstr);

		const std::string& get_current_namespace();
		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const override;
		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;

	public:
		Interpreter(InterpreterScope* global_scope, parser::ASTProgramNode* main_program, const std::map<std::string, parser::ASTProgramNode*>& programs);
		Interpreter() = default;
		~Interpreter() = default;

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
		void visit(parser::ASTEnumNode*) override;
		void visit(parser::ASTTryCatchNode*) override;
		void visit(parser::ASTThrowNode*) override;
		void visit(parser::ASTReticencesNode*) override;
		void visit(parser::ASTSwitchNode*) override;
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
		void visit(parser::ASTFunctionExpression*) override;
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

#endif // !INTERPRETER_HPP
