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
#include "gc.hpp"

using namespace visitor;
using namespace parser;

class VirtualMachine : public MetaVisitor {
public:
	std::vector<RuntimeValue*> value_stack;
	size_t param_count = 0;
	std::map<std::string, std::function<void()>> builtin_functions;

	void push_constant(RuntimeValue* value);
	RuntimeValue* get_stack_top();

private:
	size_t pc = 0;
	std::vector<BytecodeInstruction> instructions;
	BytecodeInstruction current_instruction;
	GarbageCollector gc;
	std::stack<StructureDefinition> struct_def_build_stack;
	std::stack<FunctionDefinition> func_def_build_stack;
	std::stack<RuntimeValue*> value_build_stack;
	std::stack<size_t> return_stack;

	Type set_type;
	std::string set_type_name;
	std::string set_type_name_space;
	Type set_array_type;
	std::vector<std::shared_ptr<ASTExprNode>> set_array_dim;
	std::shared_ptr<ASTExprNode> set_default_value;
	bool set_is_rest;

	dim_eval_func_t evaluate_access_vector_ptr = std::bind(&VirtualMachine::evaluate_access_vector, this, std::placeholders::_1);

	size_t print_level = 0;
	std::vector<uintptr_t> printed;

	long long try_deep = 0;

private:
	void decode_operation();
	bool get_next();

	void cleanup_type_set();

	RuntimeValue* alocate_value(RuntimeValue* value);

	void push_empty(Type type);
	void push_function_constant(const std::string& identifier);
	void binary_operation(const std::string& op);
	void unary_operation(const std::string& op);
	void function_call_operation();
	void throw_operation();
	void type_parse_operation();
	void store_var();
	void load_var();

	std::vector<unsigned int> evaluate_access_vector(const std::vector<std::shared_ptr<ASTExprNode>>& expr_access_vector);

public:
	VirtualMachine(std::shared_ptr<Scope> global_scope, std::vector<BytecodeInstruction> instructions);
	VirtualMachine() = default;
	~VirtualMachine() = default;

	void run();
};


#endif // !VIRTUAL_MACHINE_HPP
