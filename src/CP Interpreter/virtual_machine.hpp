#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <memory>
#include <map>
#include <unordered_map>
#include <stack>
#include <functional>

#include "scope.hpp"
#include "bytecode.hpp"
#include "types.hpp"
#include "visitor.hpp"
#include "ast.hpp"

using namespace parser;

class VirtualMachine {
public:
	std::map<std::string, std::function<void()>> builtin_functions;
	std::vector<RuntimeValue*> builtin_arguments;
	std::unordered_map<std::string, std::vector<std::shared_ptr<visitor::Scope>>> scopes;

	std::string parse_value_to_string(const RuntimeValue* value);

private:
	size_t pc = 0;
	std::vector<BytecodeInstruction> instructions;
	BytecodeInstruction current_instruction;
	GarbageCollector gc;
	std::stack<RuntimeValue*> value_stack;
	std::stack<StructureDefinition> struct_def_build_stack;
	std::stack<FunctionDefinition> func_def_build_stack;
	std::stack<std::string> namespace_stack;
	std::stack<size_t> return_stack;

	Type set_type;
	std::string set_type_name;
	std::string set_type_name_space;
	Type set_array_type;
	std::vector<RuntimeValue*> set_array_dim;
	RuntimeValue* set_default_value;
	bool set_is_rest;

	//dim_eval_func_t evaluate_access_vector_ptr = std::bind(&VirtualMachine::evaluate_access_vector, this, std::placeholders::_1);
	//std::vector<std::string> parsed_libs;
	//std::string function_call_name;
	//std::string return_from_function_name;
	//std::stack<std::vector<TypeDefinition>> current_function_signature;
	//std::stack<std::vector<Identifier>> current_function_call_identifier_vector;
	//std::stack<std::string> current_function_nmspace;
	////std::stack<interpreter_parameter_list_t> current_function_defined_parameters;
	//std::stack<std::vector<Value*>> current_function_calling_arguments;
	//std::stack<TypeDefinition> current_function_return_type;
	//std::stack<std::string> current_this_name;
	//std::map<std::string, std::vector<std::string>> program_nmspaces;
	//size_t is_switch = 0;
	//size_t is_loop = 0;
	//bool continue_block = false;
	//bool break_block = false;
	//bool return_from_function = false;
	//bool exit_from_program = false;
	//bool executed_elif = false;
	//bool has_string_access = false;
	//bool exception = false;

	//std::vector<ASTExprNode*> current_expression_array_dim;
	//int current_expression_array_dim_max = 0;
	//TypeDefinition current_expression_array_type;
	//bool is_max = false;

	//size_t print_level = 0;
	//std::vector<uintptr_t> printed;

private:
	void decode_operation();
	bool get_next();

	void cleanup_type_set();

	void push_empty(Type type);
	void push_empty(Type type);
	void push_constant(RuntimeValue* value);
	void push_function_constant(const std::string& identifier);
	//void push_array();
	//void set_element();
	//void push_struct();
	void binary_operation(const std::string& op);
	void unary_operation(const std::string& op);

	std::vector<unsigned int> evaluate_access_vector(const std::vector<ASTExprNode*>& expr_access_vector);
	std::vector<unsigned int> calculate_array_dim_size(const cp_array& arr);

	void check_build_array(RuntimeValue* new_value, std::vector<ASTExprNode*> dim);
	cp_array build_array(const std::vector<ASTExprNode*>& dim, RuntimeValue* init_value, long long i);
	cp_array build_undefined_array(const std::vector<ASTExprNode*>& dim, long long i);

	void normalize_type(std::shared_ptr<RuntimeVariable> var, RuntimeValue* val);
	RuntimeValue* do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, bool is_expr = false, cp_int str_pos = 0);
	cp_int do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
	cp_bool do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
	cp_int do_operation(cp_int lval, cp_int rval, const std::string& op);
	cp_float do_operation(cp_float lval, cp_float rval, const std::string& op);
	cp_string do_operation(cp_string lval, cp_string rval, const std::string& op);
	cp_array do_operation(cp_array lval, cp_array rval, const std::string& op);

	std::string parse_array_to_string(const cp_array& arr_value);
	std::string parse_struct_to_string(const  RuntimeValue* value);

	std::shared_ptr<InterpreterScope> get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier);
	std::shared_ptr<InterpreterScope> get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier);
	std::shared_ptr<InterpreterScope> get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>* signature, bool strict = true);
	std::shared_ptr<InterpreterScope> get_inner_most_functions_scope(const std::string& nmspace, const std::string& identifier);

	RuntimeValue* set_value(std::shared_ptr<InterpreterScope> scope, const std::vector<Identifier>& identifier_vector, RuntimeValue* new_value);
	RuntimeValue* access_value(const std::shared_ptr<InterpreterScope> scope, RuntimeValue* value, const std::vector<Identifier>& identifier_vector, size_t i = 0);

	void call_builtin_function(const std::string& identifier);
	void declare_function_block_parameters(const std::string& nmspace);

	cp_bool equals_value(const RuntimeValue* lval, const RuntimeValue* rval);
	cp_bool equals_array(const cp_array& larr, const cp_array& rarr);
	cp_bool equals_struct(const cp_struct& lstr, const cp_struct& rstr);

	const std::string& get_current_namespace();
	const std::string& get_namespace(const std::string& nmspace = "") const override;
	const std::string& get_namespace(const ASTProgramNode* program, const std::string& nmspace = "") const override;
	void set_curr_pos(unsigned int row, unsigned int col) override;
	std::string msg_header() override;

public:
	VirtualMachine(std::vector<BytecodeInstruction> instructions);
	VirtualMachine() = default;
	~VirtualMachine() = default;

	void run();

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
	void visit(ASTEnumNode*) override;
	void visit(ASTTryCatchNode*) override;
	void visit(ASTThrowNode*) override;
	void visit(ASTReticencesNode*) override;
	void visit(ASTSwitchNode*) override;
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


#endif // !INTERPRETER_HPP
