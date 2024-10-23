#include <iostream>
#include <algorithm> 
#include <cmath>
#include <compare>
#include <functional>

#include "virtual_machine.hpp"
#include "exception_handler.hpp"
#include "token.hpp"

#include "vendor/axeutils.hpp"
#include "vendor/axewatch.hpp"
#include "vendor/axeuuid.hpp"

using namespace lexer;

VirtualMachine::VirtualMachine(std::vector<BytecodeInstruction> instructions)
	: instructions(instructions), gc(GarbageCollector(value_stack)) {}

void VirtualMachine::run() {

	while (get_next()) {
		try {
			decode_operation();
		}
		catch (std::exception ex) {

			throw std::runtime_error(ex.what());
		}
	}

}

void VirtualMachine::push_empty(Type type) {
	auto val = gc.allocate(new RuntimeValue(type));
	value_stack.push(dynamic_cast<RuntimeValue*>(val));
}

RuntimeValue* VirtualMachine::alocate_value(RuntimeValue* value) {
	return dynamic_cast<RuntimeValue*>(gc.allocate(value));
}

void VirtualMachine::push_constant(RuntimeValue* value) {
	auto val = gc.allocate(value);
	value_stack.push(dynamic_cast<RuntimeValue*>(val));
}

void VirtualMachine::push_function_constant(const std::string& identifier) {
	auto fdef = axe::StringUtils::split(identifier, ":");
	push_constant(new RuntimeValue(cp_function(fdef[0], fdef[1])));
}

void VirtualMachine::binary_operation(const std::string& op) {
	RuntimeValue* rval = value_stack.top();
	value_stack.pop();
	RuntimeValue* lval = value_stack.top();
	value_stack.pop();
	push_constant(do_operation(op, lval, rval, true));
}

void VirtualMachine::unary_operation(const std::string& op) {
	RuntimeValue* value = value_stack.top();

	if (op == "ref" || op == "unref") {
		if (op == "unref") {
			value->use_ref = false;
		}
		else if (op == "ref") {
			value->use_ref = true;
		}
	}
	else {
		if (!value->use_ref) {
			value_stack.pop();
			push_constant(new RuntimeValue(value));
		}

		switch (value->type) {
		case Type::T_INT:
			if (op == "-") {
				value->set(cp_int(-value->get_i()));
			}
			else if (op == "~") {
				value->set(cp_int(~value->get_i()));
			}
			break;
		case Type::T_FLOAT:
			if (op == "-") {
				value->set(cp_float(-value->get_f()));
			}
			break;
		case Type::T_BOOL:
			value->set(cp_bool(!value->get_b()));
			break;
		default:
			throw std::runtime_error("incompatible unary operator '" + op +
				"' in front of " + type_str(value->type) + " expression");
		}
	}
}

void VirtualMachine::function_call_operation() {
	return_stack.push(pc + 1);

	std::string nmspace = get_namespace();
	std::string identifier = current_instruction.get_string_operand();
	bool strict = true;
	std::vector<TypeDefinition> signature;
	std::vector<RuntimeValue*> function_arguments;

	while (!value_stack.empty()) {
		RuntimeValue* value = value_stack.top();
		value_stack.pop();

		signature.insert(signature.begin(), *value);

		RuntimeValue* pvalue = nullptr;
		if (value->use_ref) {
			pvalue = value;
		}
		else {
			pvalue = new RuntimeValue(value);
		}

		function_arguments.insert(function_arguments.begin(), pvalue);
	}

	std::shared_ptr<Scope> func_scope;
	try {
		func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
	}
	catch (...) {
		try {
			strict = false;
			func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
		}
		catch (...) {
			try {
				auto var_scope = get_inner_most_variable_scope(nmspace, identifier);
				auto var = std::dynamic_pointer_cast<RuntimeVariable>(var_scope->find_declared_variable(identifier));
				nmspace = var->value->get_fun().first;
				identifier = var->value->get_fun().second;
				auto identifier_vector = std::vector<Identifier>{ Identifier(identifier) };
				func_scope = get_inner_most_function_scope(nmspace, identifier, &signature, strict);
			}
			catch (...) {
				std::string func_name = ExceptionHandler::buid_signature(identifier, signature, evaluate_access_vector_ptr);
				throw std::runtime_error("function '" + func_name + "' was never declared");
			}
		}
	}

	auto& declfun = func_scope->find_declared_function(identifier, &signature, evaluate_access_vector_ptr, strict);

	pc = declfun.pointer;
}

void VirtualMachine::throw_operation() {
	auto value = get_stack_top();

	if (is_struct(value->type)
		&& value->type_name == "Exception") {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}

		throw std::exception(value->get_str()["error"]->get_s().c_str());
	}
	else if (is_string(value->type)) {
		throw std::runtime_error(value->get_s());
	}
	else {
		throw std::runtime_error("expected cp::Exception struct or string in throw");
	}

}

void VirtualMachine::type_parse_operation() {
	Type type = Type(current_instruction.get_uint8_operand());
	auto value = get_stack_top();
	RuntimeValue* new_value = new RuntimeValue();

	switch (type) {
	case Type::T_BOOL:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->copy_from(value);
			break;
		case Type::T_INT:
			new_value->set(cp_bool(value->get_i() != 0));
			break;
		case Type::T_FLOAT:
			new_value->set(cp_bool(value->get_f() != .0));
			break;
		case Type::T_CHAR:
			new_value->set(cp_bool(value->get_c() != '\0'));
			break;
		case Type::T_STRING:
			new_value->set(cp_bool(!value->get_s().empty()));
			break;
		}
		break;

	case Type::T_INT:
		switch (type) {
		case Type::T_BOOL:
			new_value->set(cp_int(value->get_b()));
			break;
		case Type::T_INT:
			new_value->copy_from(value);
			break;
		case Type::T_FLOAT:
			new_value->set(cp_int(value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->set(cp_int(value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(cp_int(std::stoll(value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse int");
			}
			break;
		}
		break;

	case Type::T_FLOAT:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->set(cp_float(value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(cp_float(value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->copy_from(value);
			break;
		case Type::T_CHAR:
			new_value->set(cp_float(value->get_c()));
			break;
		case Type::T_STRING:
			try {
				new_value->set(cp_float(std::stold(value->get_s())));
			}
			catch (...) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case Type::T_CHAR:
		switch (value->type) {
		case Type::T_BOOL:
			new_value->set(cp_char(value->get_b()));
			break;
		case Type::T_INT:
			new_value->set(cp_char(value->get_i()));
			break;
		case Type::T_FLOAT:
			new_value->set(cp_char(value->get_f()));
			break;
		case Type::T_CHAR:
			new_value->copy_from(value);
			break;
		case Type::T_STRING:
			if (new_value->get_s().size() > 1) {
				throw std::runtime_error("'" + value->get_s() + "' is not a valid value to parse char");
			}
			else {
				new_value->set(cp_char(value->get_s()[0]));
			}
			break;
		}
		break;

	case Type::T_STRING:
		new_value->set(cp_string(parse_value_to_string(value)));

	}

	new_value->set_type(type);

	push_constant(new_value);
}

void VirtualMachine::decode_operation() {
	switch (current_instruction.opcode) {
	case OP_RES:
		throw std::runtime_error("Reserved operation");
		break;

		// namespace operations
	case OP_POP_NAMESPACE:
		current_namespace.push(current_instruction.get_string_operand());
		break;
	case OP_PUSH_NAMESPACE:
		current_namespace.pop();
		break;
	case OP_PUSH_NAMESPACE_STACK:
		push_constant(new RuntimeValue(current_namespace.top()));
		break;
	case OP_INCLUDE_NAMESPACE:
		program_nmspaces[get_namespace()].push_back(current_instruction.get_string_operand());
		break;
	case OP_EXCLUDE_NAMESPACE: {
		const auto& op_nmspace = current_instruction.get_string_operand();
		const auto& nmspace = get_namespace();
		size_t pos = std::distance(program_nmspaces[nmspace].begin(),
			std::find(program_nmspaces[nmspace].begin(),
				program_nmspaces[nmspace].end(), op_nmspace));
		program_nmspaces[nmspace].erase(program_nmspaces[nmspace].begin() + pos);
		break;
	}

		// constant operations
	case OP_POP_CONSTANT:
		value_stack.pop();
		break;
	case OP_PUSH_UNDEFINED:
		push_empty(Type::T_UNDEFINED);
		break;
	case OP_PUSH_VOID:
		push_empty(Type::T_VOID);
		break;
	case OP_PUSH_BOOL:
		push_constant(new RuntimeValue(current_instruction.get_bool_operand()));
		break;
	case OP_PUSH_INT:
		push_constant(new RuntimeValue(current_instruction.get_int_operand()));
		break;
	case OP_PUSH_FLOAT:
		push_constant(new RuntimeValue(current_instruction.get_float_operand()));
		break;
	case OP_PUSH_CHAR:
		push_constant(new RuntimeValue(current_instruction.get_char_operand()));
		break;
	case OP_PUSH_STRING:
		push_constant(new RuntimeValue(current_instruction.get_string_operand()));
		break;
	case OP_PUSH_FUNCTION:
		push_function_constant(current_instruction.get_string_operand());
		break;
	case OP_CREATE_ARRAY:{
		auto size = current_instruction.get_size_operand();
		auto val = gc.allocate(new RuntimeValue(cp_array(size)));
		value_stack.push(dynamic_cast<RuntimeValue*>(val));
		break;
	}
	case OP_SET_ELEMENT: {
		RuntimeValue* value = get_stack_top();
		value_stack.top()->arr[current_instruction.get_size_operand()] = value;
		break;
	}
	case OP_CREATE_STRUCT: {
		auto sdef = axe::StringUtils::split(current_instruction.get_string_operand(), ":");
		auto val = gc.allocate(new RuntimeValue(cp_struct(), sdef[1], sdef[0]));
		value_stack.push(dynamic_cast<RuntimeValue*>(val));
		break;
	}
	case OP_SET_FIELD:{
		RuntimeValue* value = get_stack_top();
		value_stack.top()->str[current_instruction.get_string_operand()] = value;
		break;
	}

		// struct definition operations
	case OP_STRUCT_START:
		struct_def_build_stack.push(StructureDefinition(current_instruction.get_string_operand()));
		break;
	case OP_STRUCT_SET_VAR: {
		auto var_id = current_instruction.get_string_operand();

		std::vector<void*> dim;
		for (auto s : set_array_dim) {
			dim.push_back(s);
		}

		auto var = VariableDefinition(var_id,
			set_type, set_type_name, set_type_name_space, set_array_type,
			dim, set_default_value, set_is_rest, 0, 0);

		struct_def_build_stack.top().variables[var_id] = var;

		cleanup_type_set();

		break;
	}
	case OP_STRUCT_END:{
		auto& str = struct_def_build_stack.top();
		struct_def_build_stack.pop();
		scopes[get_namespace()].back()->declare_structure_definition(str);
		break;
	}

		// typing operations
	case OP_SET_TYPE:
		set_type = (Type)current_instruction.get_uint8_operand();
		break;
	case OP_SET_TYPE_NAME:
		set_type_name = current_instruction.get_string_operand();
		break;
	case OP_SET_TYPE_NAME_SPACE:
		set_type_name_space = current_instruction.get_string_operand();
		break;
	case OP_SET_ARRAY_SIZE:{
		RuntimeValue* value = get_stack_top();
		set_array_dim.push_back(value);
		break;
	}
	case OP_SET_USE_REF:
		// todo
		break;
	case OP_SET_DEFAULT_VALUE:
		set_default_value = value_stack.top();
		value_stack.pop();
		break;
	case OP_SET_IS_REST:
		set_is_rest = current_instruction.get_bool_operand();
		break;

		// variable operations
	case OP_LOAD_VAR:
		// todo OP_LOAD_VAR
		break;
	case OP_STORE_VAR:
		// todo OP_FUN_START
		break;
	case OP_ASSIGN_VAR:
		// todo OP_ASSIGN_VAR
		break;
	case OP_LOAD_SUB_ID:
		// todo OP_LOAD_SUB_ID
		break;
	case OP_LOAD_SUB_IX:
		// todo OP_LOAD_SUB_IX
		break;
	case OP_ASSIGN_SUB:
		// todo OP_ASSIGN_SUB
		break;

		// function operations
	case OP_FUN_START: {
		std::vector<void*> dim;
		for (auto s : set_array_dim) {
			dim.push_back(s);
		}

		func_def_build_stack.push(FunctionDefinition(current_instruction.get_string_operand(), set_type, set_type_name,
			set_type_name_space, set_array_type, dim));

		cleanup_type_set();

		break;
	}
	case OP_FUN_SET_PARAM: {
		auto var_id = current_instruction.get_string_operand();

		std::vector<void*> dim;
		for (auto s : set_array_dim) {
			dim.push_back(s);
		}

		auto var = VariableDefinition(var_id,
			set_type, set_type_name, set_type_name_space, set_array_type,
			dim, set_default_value, set_is_rest, 0, 0);

		func_def_build_stack.top().signature.push_back(var);
		func_def_build_stack.top().parameters.push_back(var);

		cleanup_type_set();

		break;
	}
	case OP_FUN_END:{
		auto& fun = func_def_build_stack.top();
		func_def_build_stack.pop();
		fun.pointer = pc + 2;
		scopes[get_namespace()].back()->declare_function(fun.identifier, fun);
		break;
	}
	case OP_CALL:
		function_call_operation();
		break;
	case OP_RETURN:
		pc = return_stack.top();
		return_stack.pop();
		break;

		// conditional operations
	case OP_CONTINUE:
		// todo OP_CONTINUE
		break;
	case OP_BREAK:
		// todo OP_BREAK
		break;
	case OP_TRY_START:
		// todo OP_TRY_START
		break;
	case OP_TRY_END:
		// todo OP_TRY_END
		break;
	case OP_THROW:
		throw_operation();
		break;
	case OP_GET_ITERATOR:
		// todo OP_GET_ITERATOR
		break;
	case OP_NEXT_ELEMENT:
		// todo OP_NEXT_ELEMENT
		break;
	case OP_JUMP:
		// todo OP_JUMP
		break;
	case OP_JUMP_IF_FALSE:
		// todo OP_JUMP_IF_FALSE
		break;
	case OP_JUMP_IF_FALSE_OR_NEXT:
		// todo OP_JUMP_IF_FALSE_OR_NEXT
		break;
	case OP_JUMP_IF_TRUE:
		// todo OP_JUMP_IF_TRUE
		break;
	case OP_JUMP_IF_TRUE_OR_NEXT:
		// todo OP_JUMP_IF_TRUE_OR_NEXT
		break;

		// expression operations
	case OP_IS_TYPE: {
		Type type = static_cast<Type>(current_instruction.get_uint8_operand());
		auto value = get_stack_top();
		RuntimeValue* res_value = new RuntimeValue(Type::T_BOOL);
		if (is_any(type)) {
			res_value->set(cp_bool(
				(value->ref && (is_any(value->ref->type))
					|| is_any(value->ref->array_type))));
			return;
		}
		else if (is_array(type)) {
			res_value->set(cp_bool(is_array(value->type) || value->dim.size() > 0));
			return;
		}
		else if (is_struct(type)) {
			res_value->set(cp_bool(is_struct(value->type) || is_struct(value->array_type)));
			return;
		}
		break;
	}
	case OP_REFID:
		push_constant(new RuntimeValue(cp_int(get_stack_top())));
		break;
	case OP_TYPEID: {
		auto typeof = build_type();
		push_constant(new RuntimeValue(cp_string(typeof)));
		break;
	}
	case OP_TYPEOF: {
		auto typeof = build_type();
		push_constant(new RuntimeValue(cp_int(axe::StringUtils::hashcode(typeof))));
		break;
	}
	case OP_TYPE_PARSE:
		type_parse_operation();
		break;
	case OP_TERNARY:
		// todo OP_TERNARY
		break;
	case OP_IN:
		// todo OP_IN
		break;
	case OP_OR:
		binary_operation("or");
		break;
	case OP_AND:
		binary_operation("and");
		break;
	case OP_BIT_OR:
		binary_operation("|");
		break;
	case OP_BIT_XOR:
		binary_operation("^");
		break;
	case OP_BIT_AND:
		binary_operation("&");
		break;
	case OP_EQL:
		binary_operation("==");
		break;
	case OP_DIF:
		binary_operation("!=");
		break;
	case OP_LT:
		binary_operation("<");
		break;
	case OP_LTE:
		binary_operation("<=");
		break;
	case OP_GT:
		binary_operation(">");
		break;
	case OP_GTE:
		binary_operation("=>");
		break;
	case OP_SPACE_SHIP:
		binary_operation("<=>");
		break;
	case OP_LEFT_SHIFT:
		binary_operation("<<");
		break;
	case OP_RIGHT_SHIFT:
		binary_operation(">>");
		break;
	case OP_ADD:
		binary_operation("+");
		break;
	case OP_SUB:
		binary_operation("-");
		break;
	case OP_MUL:
		binary_operation("*");
		break;
	case OP_DIV:
		binary_operation("/");
		break;
	case OP_REMAINDER:
		binary_operation("%");
		break;
	case OP_FLOOR_DIV:
		binary_operation("/%");
		break;
	case OP_NOT:
		unary_operation("not");
		break;
	case OP_BIT_NOT:
		unary_operation("~");
		break;
	case OP_EXP:
		binary_operation("**");
		break;
	case OP_REF:
		unary_operation("ref");
		break;
	case OP_UNREF:
		unary_operation("unref");
		break;
	case OP_TRAP:
		break;
	case OP_HALT:
		pc = instructions.size();
		break;
	case OP_ERROR:
		std::cerr << "Operation error" << std::endl;
		break;

	default:
		std::cerr << "Unknow operation" << std::endl;
		break;
	}
}

RuntimeValue* VirtualMachine::get_stack_top() {
	auto value = value_stack.top();
	value_stack.pop();
	return value;
}

bool VirtualMachine::get_next() {
	if (pc >= instructions.size()) {
		return false;
	}
	current_instruction = instructions[pc++];
	return true;
}

void VirtualMachine::cleanup_type_set() {
	set_type = Type::T_UNDEFINED;
	set_type_name = "";
	set_type_name_space = "";
	set_array_type = Type::T_UNDEFINED;
	set_array_dim = std::vector<RuntimeValue*>();
	set_default_value = nullptr;
	set_is_rest = false;
}

std::string VirtualMachine::build_type() {
	auto curr_value = get_stack_top();
	auto dim = std::vector<unsigned int>();
	auto type = is_void(curr_value->type) ? curr_value->type : curr_value->type;
	std::string str_type = "";

	if (is_array(type)) {
		dim = evaluate_access_vector(curr_value->dim);
		type = curr_value->array_type;
	}

	str_type = type_str(type);

	if (is_struct(type)) {
		if (dim.size() > 0) {
			auto arr = curr_value->get_arr()[0];
			for (size_t i = 0; i < dim.size() - 1; ++i) {
				arr = arr->get_arr()[0];
			}
			str_type = arr->type_name;
		}
		else {
			str_type = curr_value->type_name;
		}
	}

	if (dim.size() > 0) {
		for (size_t i = 0; i < dim.size(); ++i) {
			str_type += "[" + std::to_string(dim[i]) + "]";
		}
	}

	if (is_struct(type) && !curr_value->type_name_space.empty()) {
		str_type = curr_value->type_name_space + "::" + str_type;
	}

	return str_type;
}

cp_bool VirtualMachine::equals_value(const RuntimeValue* lval, const RuntimeValue* rval) {
	if (lval->use_ref) {
		return lval == rval;
	}

	switch (lval->type) {
	case Type::T_VOID:
		return is_void(rval->type);
	case Type::T_BOOL:
		return lval->get_b() == rval->get_b();
	case Type::T_INT:
		return lval->get_i() == rval->get_i();
	case Type::T_FLOAT:
		return lval->get_f() == rval->get_f();
	case Type::T_CHAR:
		return lval->get_c() == rval->get_c();
	case Type::T_STRING:
		return lval->get_s() == rval->get_s();
	case Type::T_ARRAY:
		return equals_array(lval->get_arr(), rval->get_arr());
	case Type::T_STRUCT:
		return equals_struct(lval->get_str(), rval->get_str());
	}
	return false;
}

cp_bool VirtualMachine::equals_struct(const cp_struct& lstr, const cp_struct& rstr) {
	if (lstr.size() != rstr.size()) {
		return false;
	}

	for (auto& lval : lstr) {
		if (rstr.find(lval.first) == rstr.end()) {
			return false;
		}
		if (!equals_value(lval.second, rstr.at(lval.first))) {
			return false;
		}
	}

	return true;
}

cp_bool VirtualMachine::equals_array(const cp_array& larr, const cp_array& rarr) {
	if (larr.size() != rarr.size()) {
		return false;
	}

	for (size_t i = 0; i < larr.size(); ++i) {
		if (!equals_value(larr[i], rarr[i])) {
			return false;
		}
	}

	return true;
}

std::shared_ptr<Scope> VirtualMachine::get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_variable(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' variable");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> VirtualMachine::get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			bool found = false;
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_structure_definition(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' struct");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> VirtualMachine::get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>* signature, bool strict) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' fuction");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> VirtualMachine::get_inner_most_functions_scope(const std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function_name(identifier); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function_name(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' fuction");
		}
	}
	return scopes[nmspace][i];
}

std::string VirtualMachine::parse_value_to_string(const RuntimeValue* value) {
	std::string str = "";
	if (print_level == 0) {
		printed.clear();
	}
	++print_level;
	switch (value->type) {
	case Type::T_VOID:
		str = "null";
		break;
	case Type::T_BOOL:
		str = ((value->get_b()) ? "true" : "false");
		break;
	case Type::T_INT:
		str = std::to_string(value->get_i());
		break;
	case Type::T_FLOAT:
		str = std::to_string(value->get_f());
		break;
	case Type::T_CHAR:
		str = cp_string(std::string{ value->get_c() });
		break;
	case Type::T_STRING:
		str = value->get_s();
		break;
	case Type::T_STRUCT: {
		if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
			std::stringstream s = std::stringstream();
			if (!value->type_name_space.empty()) {
				s << value->type_name_space << "::";
			}
			s << value->type_name;
			s << "<" << value << ">{...}";
			str = s.str();
		}
		else {
			printed.push_back(reinterpret_cast<uintptr_t>(value));
			str = parse_struct_to_string(value);
		}
		break;
	}
	case Type::T_ARRAY:
		str = parse_array_to_string(value->get_arr());
		break;
	case Type::T_FUNCTION: {
		//auto funcs = get_inner_most_functions_scope(value->get_fun().first, value->get_fun().second)->find_declared_functions(value->get_fun().second);
		//for (auto& it = funcs.first; it != funcs.second; ++it) {
		//	auto& func_name = it->first;
		//	auto& func_sig = it->second.signature;
		//	str = ExceptionHandler::buid_signature(func_name, func_sig, evaluate_access_vector_ptr);
		//}
		str = value->get_fun().first + (value->get_fun().first.empty() ? "" : "::") + value->get_fun().second + "(...)";
		break;
	}
	case Type::T_UNDEFINED:
		throw std::runtime_error("undefined expression");
	default:
		throw std::runtime_error("can't determine value type on parsing");
	}
	--print_level;
	return str;
}

std::string VirtualMachine::parse_array_to_string(const cp_array& arr_value) {
	std::stringstream s = std::stringstream();
	s << "[";
	for (auto i = 0; i < arr_value.size(); ++i) {
		bool isc = is_char(arr_value[i]->type);
		bool iss = is_string(arr_value[i]->type);

		if (isc) s << "'";
		else if (iss) s << "\"";

		s << parse_value_to_string(arr_value[i]);

		if (isc) s << "'";
		else if (iss) s << "\"";

		if (i < arr_value.size() - 1) {
			s << ",";
		}
	}
	s << "]";
	return s.str();
}

std::string VirtualMachine::parse_struct_to_string(const RuntimeValue* value) {
	auto str_value = value->get_str();
	std::stringstream s = std::stringstream();
	if (!value->type_name_space.empty()) {
		s << value->type_name_space << "::";
	}
	s << value->type_name << "<" << value << ">{";
	for (auto const& [key, val] : str_value) {
		if (key != modules::Module::INSTANCE_ID_NAME) {
			s << key + ":";
			s << parse_value_to_string(val);
			s << ",";
		}
	}
	if (s.str() != "{") {
		s.seekp(-1, std::ios_base::end);
	}
	s << "}";
	return s.str();
}

RuntimeValue* VirtualMachine::do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, bool is_expr, cp_int str_pos) {
	Type l_var_type = lval->ref ? lval->ref->type : lval->type;
	Type l_var_array_type = lval->ref ? lval->ref->array_type : lval->array_type;
	Type l_type = is_undefined(lval->type) ? l_var_type : lval->type;
	Type r_var_type = rval->ref ? rval->ref->type : rval->type;
	Type r_var_array_type = rval->ref ? rval->ref->array_type : rval->array_type;
	Type r_type = rval->type;
	RuntimeValue* res_value = nullptr;

	if (is_void(r_type) && op == "=") {
		lval->set_null();
		return lval;
	}

	if (is_void(l_type) && op == "=") {
		lval->copy_from(rval);
		return lval;
	}

	if ((is_void(l_type) || is_void(r_type))
		&& Token::is_equality_op(op)) {
		return new RuntimeValue((cp_bool)((op == "==") ?
			match_type(l_type, r_type)
			: !match_type(l_type, r_type)));
	}

	if (lval->use_ref
		&& Token::is_equality_op(op)) {
		return new RuntimeValue((cp_bool)((op == "==") ?
			lval == rval
			: lval != rval));
	}

	switch (r_type) {
	case Type::T_BOOL: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_b());
			break;
		}

		if (!is_bool(l_type)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		if (op == "=") {
			lval->set(rval->get_b());
		}
		else if (op == "and") {
			res_value = new RuntimeValue((cp_bool)(lval->get_b() && rval->get_b()));
		}
		else if (op == "or") {
			res_value = new RuntimeValue((cp_bool)(lval->get_b() || rval->get_b()));
		}
		else if (op == "==") {
			res_value = new RuntimeValue((cp_bool)(lval->get_b() == rval->get_b()));
		}
		else if (op == "!=") {
			res_value = new RuntimeValue((cp_bool)(lval->get_b() != rval->get_b()));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_INT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_i());
			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& op == "<=>") {
			res_value = new RuntimeValue((cp_int)(do_spaceship_operation(op, lval, rval)));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_relational_op(op)) {
			res_value = new RuntimeValue(do_relational_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_equality_op(op)) {
			cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
			cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				l == r : l != r));

			break;
		}

		if (is_float(l_type) && (is_any(l_var_type) || is_expr)) {
			lval->set(do_operation(lval->get_f(), cp_float(rval->get_i()), op));
		}
		else if (is_int(l_type) && is_any(l_var_type)
			&& (op == "/=" || op == "/%=" || op == "/" || op == "/%")) {
			lval->set(do_operation(cp_float(lval->get_i()), cp_float(rval->get_i()), op));
		}
		else if (is_int(l_type)) {
			lval->set(do_operation(lval->get_i(), rval->get_i(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_FLOAT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_f());
			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& op == "<=>") {
			res_value = new RuntimeValue(do_spaceship_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_relational_op(op)) {
			lval->set(do_relational_operation(op, lval, rval));

			break;
		}

		if (is_expr
			&& is_numeric(l_type)
			&& Token::is_equality_op(op)) {
			cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
			cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				l == r : l != r));

			break;
		}

		if (is_float(l_type)) {
			lval->set(do_operation(lval->get_f(), rval->get_f(), op));
		}
		else if (is_int(l_type)) {
			lval->set(do_operation(cp_float(lval->get_i()), rval->get_f(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_CHAR: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_c());
			break;
		}

		if (is_expr
			&& is_char(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				lval->get_c() == rval->get_c()
				: lval->get_c() != lval->get_c()));

			break;
		}

		if (is_string(l_type)) {
			lval->set(do_operation(lval->get_s(), std::string{ rval->get_c() }, op));
		}
		else if (is_char(l_type)) {
			if (op != "=") {
				ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
			}

			lval->set(rval->get_c());
		}
		else if (is_any(l_var_type)) {
			if (op != "=") {
				ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
			}

			lval->set(rval->get_c());
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_STRING: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_s());
			break;
		}

		if (is_expr
			&& is_string(l_type)
			&& Token::is_equality_op(op)) {

			if (lval->get_s().size() > 30) {
				int x = 0;
			}

			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				lval->get_s() == rval->get_s()
				: lval->get_s() != rval->get_s()));

			break;
		}

		if (is_string(l_type)) {
			lval->set(do_operation(lval->get_s(), rval->get_s(), op));
		}
		else if (is_expr && is_char(l_type)) {
			lval->set(do_operation(cp_string{ lval->get_c() }, rval->get_s(), op));
		}
		else {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		break;
	}
	case Type::T_ARRAY: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_arr(), lval->array_type, lval->dim, lval->type_name, lval->type_name_space);
			break;
		}

		if (is_expr
			&& is_array(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval)));

			break;
		}

		if (!TypeDefinition::match_type_array(*lval, *rval, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		bool match_arr_t = lval->array_type == rval->array_type;
		if (!match_arr_t && !is_any(l_var_array_type)) {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(do_operation(lval->get_arr(), rval->get_arr(), op),
			match_arr_t ? lval->array_type : Type::T_ANY, lval->dim,
			lval->type_name, lval->type_name_space);

		break;
	}
	case Type::T_STRUCT: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_str(), rval->type_name, lval->type_name_space);
			break;
		}

		if (is_expr
			&& is_struct(l_type)
			&& Token::is_equality_op(op)) {
			res_value = new RuntimeValue((cp_bool)(op == "==" ?
				equals_value(lval, rval)
				: !equals_value(lval, rval)));

			break;
		}

		if (!is_struct(l_type) || op != "=") {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(rval->get_str(), rval->type_name, rval->type_name_space);

		break;
	}
	case Type::T_FUNCTION: {
		if (is_any(l_var_type) && op == "=") {
			lval->set(rval->get_str(), rval->type_name, rval->type_name_space);
			break;
		}

		if (!is_function(l_type) || op != "=") {
			ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
		}

		lval->set(rval->get_fun());

		break;
	}
	default:
		throw std::runtime_error("cannot determine type of operation");

	}

	if (!res_value) {
		res_value = lval;
	}

	return res_value;
}

cp_bool VirtualMachine::do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
	cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
	cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

	if (op == "<") {
		return l < r;
	}
	else if (op == ">") {
		return l > r;
	}
	else if (op == "<=") {
		return l <= r;
	}
	else if (op == ">=") {
		return l >= r;
	}
	ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
}

cp_int VirtualMachine::do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
	cp_float l = is_float(lval->type) ? lval->get_f() : lval->get_i();
	cp_float r = is_float(rval->type) ? rval->get_f() : rval->get_i();

	auto res = l <=> r;
	if (res == std::strong_ordering::less) {
		return cp_int(-1);
	}
	else if (res == std::strong_ordering::equal) {
		return cp_int(0);
	}
	else if (res == std::strong_ordering::greater) {
		return cp_int(1);
	}
}

cp_int VirtualMachine::do_operation(cp_int lval, cp_int rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (rval == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (rval == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return lval % rval;
	}
	else if (op == "/%=" || op == "/%") {
		if (rval == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return cp_int(std::floor(lval / rval));
	}
	else if (op == "**=" || op == "**") {
		return cp_int(std::pow(lval, rval));
	}
	else if (op == ">>=" || op == ">>") {
		return lval >> rval;
	}
	else if (op == "<<=" || op == "<<") {
		return lval << rval;
	}
	else if (op == "|=" || op == "|") {
		return lval | rval;
	}
	else if (op == "&=" || op == "&") {
		return lval & rval;
	}
	else if (op == "^=" || op == "^") {
		return lval ^ rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'int' and 'int'");
}

cp_float VirtualMachine::do_operation(cp_float lval, cp_float rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	else if (op == "-=" || op == "-") {
		return lval - rval;
	}
	else if (op == "*=" || op == "*") {
		return lval * rval;
	}
	else if (op == "/=" || op == "/") {
		if (int(rval) == 0) {
			throw std::runtime_error("division by zero encountered");
		}
		return lval / rval;
	}
	else if (op == "%=" || op == "%") {
		if (int(rval) == 0) {
			throw std::runtime_error("remainder by zero is undefined");
		}
		return std::fmod(lval, rval);
	}
	else if (op == "/%=" || op == "/%") {
		if (int(rval) == 0) {
			throw std::runtime_error("floor division by zero encountered");
		}
		return std::floor(lval / rval);
	}
	else if (op == "**=" || op == "**") {
		return cp_int(std::pow(lval, rval));
	}
	throw std::runtime_error("invalid '" + op + "' operator");
}

cp_string VirtualMachine::do_operation(cp_string lval, cp_string rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'string' and 'string'");
}

cp_array VirtualMachine::do_operation(cp_array lval, cp_array rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		//auto size = lval.size() + rval.size();
		//cp_array result = cp_array(size);

		//std::sort(lval.begin(), lval.end(), [](const RuntimeValue* a, const RuntimeValue* b) {
		//	return a->value_hash() < b->value_hash();
		//	});

		//std::sort(rval.begin(), rval.end(), [](const RuntimeValue* a, const RuntimeValue* b) {
		//	return a->value_hash() < b->value_hash();
		//	});

		//std::merge(lval.begin(), lval.end(), rval.begin(), rval.end(), result, [](const RuntimeValue* a, const RuntimeValue* b) {
		//	return a->value_hash() < b->value_hash();
		//	});

		lval.insert(lval.end(), rval.begin(), rval.end());

		return lval;
		//return result;
	}

	throw std::runtime_error("invalid '" + op + "' operator for types 'array' and 'array'");
}

std::vector<unsigned int> VirtualMachine::evaluate_access_vector(const std::vector<void*>& expr_access_vector) {
	auto access_vector = std::vector<unsigned int>();
	for (const auto& expr : expr_access_vector) {
		access_vector.push_back(static_cast<RuntimeValue*>(expr)->get_i());
	}
	return access_vector;
}
