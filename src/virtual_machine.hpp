#ifndef VIRTUAL_MACHINE_HPP
#define VIRTUAL_MACHINE_HPP

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
#include "meta_visitor.hpp"
#include "garbage_collector.hpp"

using namespace visitor;
using namespace parser;

class VirtualMachine : public MetaVisitor {
public:
	std::stack<RuntimeValue*> value_stack;
	std::map<std::string, std::function<void()>> builtin_functions;
	std::vector<RuntimeValue*> builtin_arguments;
	std::unordered_map<std::string, std::vector<std::shared_ptr<visitor::Scope>>> scopes;

	std::string parse_value_to_string(const RuntimeValue* value);

private:
	size_t pc = 0;
	std::vector<BytecodeInstruction> instructions;
	BytecodeInstruction current_instruction;
	GarbageCollector gc;
	std::stack<StructureDefinition> struct_def_build_stack;
	std::stack<FunctionDefinition> func_def_build_stack;
	std::stack<size_t> return_stack;

	Type set_type;
	std::string set_type_name;
	std::string set_type_name_space;
	Type set_array_type;
	std::vector<RuntimeValue*> set_array_dim;
	RuntimeValue* set_default_value;
	bool set_is_rest;

	dim_eval_func_t evaluate_access_vector_ptr = std::bind(&VirtualMachine::evaluate_access_vector, this, std::placeholders::_1);

	size_t print_level = 0;
	std::vector<uintptr_t> printed;

	long long try_deep = 0;

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

	RuntimeValue* alocate_value(RuntimeValue* value);

	void push_empty(Type type);
	void push_constant(RuntimeValue* value);
	void push_function_constant(const std::string& identifier);
	//void push_array();
	//void set_element();
	//void push_struct();
	void binary_operation(const std::string& op);
	void unary_operation(const std::string& op);
	void function_call_operation();
	void throw_operation();
	void type_parse_operation();

	RuntimeValue* get_stack_top();

	std::string build_type();

	RuntimeValue* do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, bool is_expr = false, cp_int str_pos = 0);
	cp_int do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
	cp_bool do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval);
	cp_int do_operation(cp_int lval, cp_int rval, const std::string& op);
	cp_float do_operation(cp_float lval, cp_float rval, const std::string& op);
	cp_string do_operation(cp_string lval, cp_string rval, const std::string& op);
	cp_array do_operation(cp_array lval, cp_array rval, const std::string& op);

	std::string parse_array_to_string(const cp_array& arr_value);
	std::string parse_struct_to_string(const  RuntimeValue* value);

	std::shared_ptr<Scope> get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier);
	std::shared_ptr<Scope> get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier);
	std::shared_ptr<Scope> get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition*>* signature, bool strict = true);
	std::shared_ptr<Scope> get_inner_most_functions_scope(const std::string& nmspace, const std::string& identifier);

	cp_bool equals_value(const RuntimeValue* lval, const RuntimeValue* rval);
	cp_bool equals_array(const cp_array& larr, const cp_array& rarr);
	cp_bool equals_struct(const cp_struct& lstr, const cp_struct& rstr);

	std::vector<unsigned int> evaluate_access_vector(const std::vector<void*>& expr_access_vector);

public:
	VirtualMachine(std::vector<BytecodeInstruction> instructions);
	VirtualMachine() = default;
	~VirtualMachine() = default;

	void run();
};


#endif // !VIRTUAL_MACHINE_HPP
