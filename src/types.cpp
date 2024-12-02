#include "types.hpp"

#include "vendor/axeutils.hpp"

#include "exception_handler.hpp"
#include "module.hpp";
#include "datetime.hpp"
#include "graphics.hpp"
#include "files.hpp"
#include "console.hpp"

#include "visitor.hpp"
#include "token.hpp"

#include <sstream> 

using namespace visitor;
using namespace lexer;

namespace parser {
	std::string type_str(Type t) {
		switch (t) {
		case Type::T_UNDEFINED:
			return "undefined";
		case Type::T_VOID:
			return "void";
		case Type::T_BOOL:
			return "bool";
		case Type::T_INT:
			return "int";
		case Type::T_FLOAT:
			return "float";
		case Type::T_CHAR:
			return "char";
		case Type::T_STRING:
			return "string";
		case Type::T_ANY:
			return "any";
		case Type::T_ARRAY:
			return "array";
		case Type::T_STRUCT:
			return "struct";
		case Type::T_FUNCTION:
			return "function";
		default:
			throw std::runtime_error("invalid type encountered");
		}
	}

	bool match_type(Type type1, Type type2) {
		return type1 == type2;
	}

	bool is_undefined(Type type) {
		return match_type(type, Type::T_UNDEFINED);
	}

	bool is_void(Type type) {
		return match_type(type, Type::T_VOID);
	}

	bool is_bool(Type type) {
		return match_type(type, Type::T_BOOL);
	}

	bool is_int(Type type) {
		return match_type(type, Type::T_INT);
	}

	bool is_float(Type type) {
		return match_type(type, Type::T_FLOAT);
	}

	bool is_char(Type type) {
		return match_type(type, Type::T_CHAR);
	}

	bool is_string(Type type) {
		return match_type(type, Type::T_STRING);
	}

	bool is_any(Type type) {
		return match_type(type, Type::T_ANY);
	}

	bool is_array(Type type) {
		return match_type(type, Type::T_ARRAY);
	}

	bool is_struct(Type type) {
		return match_type(type, Type::T_STRUCT);
	}

	bool is_function(Type type) {
		return match_type(type, Type::T_FUNCTION);
	}

	bool is_text(Type type) {
		return is_string(type) || is_char(type);
	}

	bool is_numeric(Type type) {
		return is_int(type) || is_float(type);
	}

	bool is_collection(Type type) {
		return is_string(type) || is_array(type);
	}

	bool is_iterable(Type type) {
		return is_collection(type) || is_struct(type);
	}
}

TypeDefinition::TypeDefinition(Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: type(type), array_type(array_type), dim(dim), type_name(type_name), type_name_space(type_name_space) {
	reset_ref();
}

TypeDefinition::TypeDefinition(Type type)
	: type(type), array_type(Type::T_UNDEFINED), dim(std::vector<std::shared_ptr<ASTExprNode>>()), type_name(""), type_name_space("") {
	reset_ref();
}

TypeDefinition::TypeDefinition()
	: type(Type::T_UNDEFINED), array_type(Type::T_UNDEFINED), dim(std::vector<std::shared_ptr<ASTExprNode>>()), type_name(""), type_name_space("") {
	reset_ref();
}

TypeDefinition TypeDefinition::get_basic(Type type) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "");
}

TypeDefinition TypeDefinition::get_array(Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim) {
	return TypeDefinition(Type::T_ARRAY, array_type, dim, "", "");
}

TypeDefinition TypeDefinition::get_struct(const std::string& type_name, const std::string& type_name_space) {
	return TypeDefinition(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), type_name, type_name_space);
}

bool TypeDefinition::is_any_or_match_type(TypeDefinition ltype, TypeDefinition rtype,
	dim_eval_func_t evaluate_access_vector, bool strict, bool strict_array) {
	if (is_any(ltype.type)
		|| is_any(rtype.type)
		|| is_void(ltype.type)
		|| is_void(rtype.type)) return true;
	return match_type(ltype, rtype, evaluate_access_vector, strict, strict_array);
}

bool TypeDefinition::match_type(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector, bool strict, bool strict_array) {
	if (match_type_bool(ltype, rtype)) return true;
	if (match_type_int(ltype, rtype)) return true;
	if (match_type_float(ltype, rtype, strict)) return true;
	if (match_type_char(ltype, rtype)) return true;
	if (match_type_string(ltype, rtype, strict)) return true;
	if (match_type_array(ltype, rtype, evaluate_access_vector, strict, strict_array)) return true;
	if (match_type_struct(ltype, rtype)) return true;
	if (match_type_function(ltype, rtype)) return true;
	return false;
}

bool TypeDefinition::match_type_bool(TypeDefinition ltype, TypeDefinition rtype) {
	return is_bool(ltype.type) && is_bool(rtype.type);
}

bool TypeDefinition::match_type_int(TypeDefinition ltype, TypeDefinition rtype) {
	return is_int(ltype.type) && is_int(rtype.type);
}

bool TypeDefinition::match_type_float(TypeDefinition ltype, TypeDefinition rtype, bool strict) {
	return is_float(ltype.type)
		&& (strict && is_float(rtype.type) ||
			!strict && is_numeric(rtype.type));
}

bool TypeDefinition::match_type_char(TypeDefinition ltype, TypeDefinition rtype) {
	return is_char(ltype.type) && is_char(rtype.type);
}

bool TypeDefinition::match_type_string(TypeDefinition ltype, TypeDefinition rtype, bool strict) {
	return is_string(ltype.type)
		&& (strict && is_string(rtype.type) ||
			!strict && is_text(rtype.type));
}

bool TypeDefinition::match_type_array(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector, bool strict, bool strict_array) {
	TypeDefinition latype = TypeDefinition(is_undefined(ltype.array_type) ? Type::T_ANY : ltype.array_type,
		Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), ltype.type_name, ltype.type_name_space);
	TypeDefinition ratype = TypeDefinition(is_undefined(rtype.array_type) ? Type::T_ANY : rtype.array_type,
		Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), rtype.type_name, rtype.type_name_space);

	return is_array(ltype.type) && is_array(rtype.type)
		&& (!strict_array && is_any_or_match_type(latype, ratype, evaluate_access_vector, strict, strict_array) ||
			match_type(latype, ratype, evaluate_access_vector, strict, strict_array))
		&& match_array_dim(ltype, rtype, evaluate_access_vector);
}

bool TypeDefinition::match_type_struct(TypeDefinition ltype, TypeDefinition rtype) {
	return is_struct(ltype.type) && is_struct(rtype.type)
		&& ltype.type_name == rtype.type_name;
}

bool TypeDefinition::match_type_function(TypeDefinition ltype, TypeDefinition rtype) {
	return is_function(ltype.type) && is_function(rtype.type);
}

bool TypeDefinition::match_array_dim(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector) {
	std::vector<unsigned int> var_dim = evaluate_access_vector(ltype.dim);
	std::vector<unsigned int> expr_dim = evaluate_access_vector(rtype.dim);

	if (expr_dim.size() == 1
		|| var_dim.size() == 0
		|| expr_dim.size() == 0) {
		return true;
	}

	if (var_dim.size() != expr_dim.size()) {
		return false;
	}

	for (size_t dc = 0; dc < var_dim.size(); ++dc) {
		if (ltype.dim.at(dc) && var_dim.at(dc) != expr_dim.at(dc)) {
			return false;
		}
	}

	return true;
}

void TypeDefinition::reset_ref() {
	use_ref = is_struct(type);
}

VariableDefinition::VariableDefinition()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""), CodePosition(),
	identifier(""), default_value(nullptr), is_rest(false) {
}

VariableDefinition::VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	std::shared_ptr<ASTExprNode> default_value, bool is_rest, unsigned int row, unsigned int col)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space), CodePosition(row, col),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {
}

VariableDefinition::VariableDefinition(const std::string& identifier, Type type,
	std::shared_ptr<ASTExprNode> default_value, bool is_rest, unsigned int row, unsigned int col)
	: TypeDefinition(type, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""), CodePosition(row, col),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {
}

VariableDefinition::VariableDefinition(const std::string& identifier, parser::Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name, const std::string& type_name_space,
	std::shared_ptr<ASTExprNode> default_value, bool is_rest, unsigned int row, unsigned int col)
	: TypeDefinition(Type::T_ARRAY, array_type, dim, "", ""), CodePosition(row, col),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {
}

VariableDefinition::VariableDefinition(const std::string& identifier,
	const std::string& type_name, const std::string& type_name_space,
	std::shared_ptr<ASTExprNode> default_value, bool is_rest, unsigned int row, unsigned int col)
	: TypeDefinition(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), type_name, type_name_space), CodePosition(row, col),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {
}

UnpackedVariableDefinition::UnpackedVariableDefinition(Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim, const std::string& type_name,
	const std::string& type_name_space, const std::vector<VariableDefinition>& variables)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space), variables(variables) {
}

UnpackedVariableDefinition::UnpackedVariableDefinition(TypeDefinition type_definition, const std::vector<VariableDefinition>& variables)
	: TypeDefinition(type_definition), variables(variables) {
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::vector<TypeDefinition*>& parameters, std::shared_ptr<ASTBlockNode> block, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), parameters(parameters), block(block) {
	check_signature();
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type,
	const std::vector<TypeDefinition*>& parameters, std::shared_ptr<ASTBlockNode> block)
	: CodePosition(), TypeDefinition(type),
	identifier(identifier), parameters(parameters), block(block) {
	check_signature();
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim)
	: CodePosition(), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier) {
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(Type::T_ANY, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""),
	identifier(identifier), parameters(std::vector<TypeDefinition*>()), block(nullptr), is_var(true) {
}

FunctionDefinition::FunctionDefinition()
	: TypeDefinition(), CodePosition(),
	identifier(""), parameters(std::vector<TypeDefinition*>()), block(nullptr) {
}

void FunctionDefinition::check_signature() const {
	bool has_default = false;
	for (size_t i = 0; i < parameters.size(); ++i) {
		if (auto parameter = dynamic_cast<VariableDefinition*>(parameters[i])) {
			if (parameter->is_rest && parameters.size() - 1 != i) {
				throw std::runtime_error("rest '" + identifier + "' parameter must be the last parameter");
			}
			if (parameter->default_value) {
				has_default = true;
			}
			if (!parameter->default_value && has_default) {
				throw std::runtime_error("default values as '" + identifier + "' must be at end");
			}
		}
	}
}

StructureDefinition::StructureDefinition(const std::string& identifier, const std::map<std::string, VariableDefinition>& variables,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), identifier(identifier), variables(variables) {
}

StructureDefinition::StructureDefinition(const std::string& identifier)
	: CodePosition(), identifier(identifier) {
}

StructureDefinition::StructureDefinition()
	: CodePosition(row, col), identifier(""), variables(std::map<std::string, VariableDefinition>()) {
}

Variable::Variable(const std::string& identifier, Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier) {
}

Variable::Variable(TypeDefinition value)
	: TypeDefinition(value) {
}

Variable::Variable()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
}

Value::Value(Type type, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
	const std::string& type_name, const std::string& type_name_space)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space) {
}

Value::Value(TypeDefinition type)
	: TypeDefinition(type) {
}

Value::Value()
	: TypeDefinition() {
}

SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name, const std::string& type_name_space, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type, array_type, dim, type_name, type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
}

SemanticValue::SemanticValue(parser::Type type, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type),
	hash(hash), is_const(is_const), is_sub(false) {
}

SemanticValue::SemanticValue(TypeDefinition type_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	Value(type_definition.type, type_definition.array_type,
		type_definition.dim, type_definition.type_name, type_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
}

SemanticValue::SemanticValue(VariableDefinition variable_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	Value(variable_definition.type, variable_definition.array_type,
		variable_definition.dim, variable_definition.type_name, variable_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
}

SemanticValue::SemanticValue(Type type, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type),
	hash(0), is_const(false), is_sub(false) {
}

SemanticValue::SemanticValue()
	: CodePosition(), Value(),
	hash(0), is_const(false), is_sub(false) {
}

void SemanticValue::copy_from(const SemanticValue& value) {
	type = value.type;
	array_type = value.array_type;
	dim = value.dim;
	type_name = value.type_name;
	type_name_space = value.type_name_space;
	hash = value.hash;
	is_const = value.is_const;
	is_sub = value.is_sub;
	row = value.row;
	col = value.col;
}

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
	const std::string& type_name, const std::string& type_name_space, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Variable(identifier, def_type(type), def_array_type(array_type, dim), dim, type_name, type_name_space),
	value(nullptr), is_const(is_const) {
}

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Variable(identifier, def_type(type), Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""),
	value(nullptr), is_const(is_const) {
}

SemanticVariable::SemanticVariable()
	: CodePosition(0, 0), Variable("", Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""),
	value(nullptr), is_const(false) {
}

void SemanticVariable::set_value(std::shared_ptr<SemanticValue> value) {
	this->value = value;
	this->value->ref = shared_from_this();
}

std::shared_ptr<SemanticValue> SemanticVariable::get_value() {
	value->ref = shared_from_this();
	return value;
}

Type SemanticVariable::def_type(Type type) {
	return is_void(type) || is_undefined(type) ? Type::T_ANY : type;
}

Type SemanticVariable::def_array_type(Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

void SemanticVariable::reset_ref() {
	TypeDefinition::reset_ref();
	use_ref = value && (use_ref || is_struct(value->type));
}


RuntimeValue::RuntimeValue(Type type, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
	const std::string& type_name, const std::string& type_name_space,
	unsigned int row, unsigned int col)
	: Value(type, array_type, dim, type_name, type_name_space) {
}

RuntimeValue::RuntimeValue()
	: Value(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
}

RuntimeValue::RuntimeValue(cp_bool rawv)
	: Value(Type::T_BOOL, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_int rawv)
	: Value(Type::T_INT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_float rawv)
	: Value(Type::T_FLOAT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_char rawv)
	: Value(Type::T_CHAR, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_string rawv)
	: Value(Type::T_STRING, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_array rawv)
	: Value(Type::T_ARRAY, Type::T_ANY, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_array rawv, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name, std::string type_name_space)
	: Value(Type::T_ARRAY, array_type, dim, type_name, type_name_space) {
	set(rawv, array_type, dim, type_name, type_name_space);
}

RuntimeValue::RuntimeValue(cp_struct rawv, std::string type_name, std::string type_name_space)
	: Value(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), type_name, type_name_space) {
	set(rawv, type_name, type_name_space);
}

RuntimeValue::RuntimeValue(cp_function rawv)
	: Value(Type::T_FUNCTION, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
	set(rawv);
}

RuntimeValue::RuntimeValue(Type type)
	: Value(type, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", "") {
}

RuntimeValue::RuntimeValue(Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name, std::string type_name_space)
	: Value(Type::T_ARRAY, array_type, dim, type_name, type_name_space) {
}

RuntimeValue::RuntimeValue(std::string type_name, std::string type_name_space)
	: Value(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), type_name, type_name_space) {
}

RuntimeValue::RuntimeValue(RuntimeValue* v) {
	copy_from(v);
}

RuntimeValue::RuntimeValue(TypeDefinition v)
	: Value(v) {
}

RuntimeValue::~RuntimeValue() {
	if (str) {
		for (auto& var : *str) {
			if (var.first == modules::Module::INSTANCE_ID_NAME) {
				delete reinterpret_cast<void*>(*var.second->i);
			}
		}
	}
	unset();
};

void RuntimeValue::set(cp_bool b) {
	unset();
	this->b = new cp_bool(b);
	type = Type::T_BOOL;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_int i) {
	unset();
	this->i = new cp_int(i);
	type = Type::T_INT;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_float f) {
	unset();
	this->f = new cp_float(f);
	type = Type::T_FLOAT;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_char c) {
	unset();
	this->c = new cp_char(c);
	type = Type::T_CHAR;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_string s) {
	unset();
	this->s = new cp_string(s);
	type = Type::T_STRING;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_array arr) {
	unset();
	this->arr = new cp_array(arr);
	type = Type::T_ARRAY;
}

void RuntimeValue::set(cp_array arr, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name, std::string type_name_space) {
	unset();
	this->arr = new cp_array(arr);
	type = Type::T_ARRAY;
	this->array_type = array_type;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(cp_struct str, std::string type_name, std::string type_name_space) {
	unset();
	this->str = new cp_struct(str);
	type = Type::T_STRUCT;
	array_type = Type::T_UNDEFINED;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(cp_function fun) {
	unset();
	this->fun = new cp_function(fun);
	type = Type::T_FUNCTION;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set_sub(std::string identifier, RuntimeValue* sub_value) {
	if (!str) return;
	(*str)[identifier] = sub_value;
}

void RuntimeValue::set_sub(size_t index, RuntimeValue* sub_value) {
	if (!arr) return;
	(*arr)[index] = sub_value;
}

cp_bool RuntimeValue::get_b() const {
	if (!b) return cp_bool();
	return *b;
}

cp_int RuntimeValue::get_i() const {
	if (!i) return cp_int();
	return *i;
}

cp_float RuntimeValue::get_f() const {
	if (!f) return cp_float();
	return *f;
}

cp_char RuntimeValue::get_c() const {
	if (!c) return cp_char();
	return *c;
}

cp_string RuntimeValue::get_s() const {
	if (!s) return cp_string();
	return *s;
}

cp_array RuntimeValue::get_arr() const {
	if (!arr) return cp_array();
	return *arr;
}

cp_struct RuntimeValue::get_str() const {
	if (!str) return cp_struct();
	return *str;
}

cp_function RuntimeValue::get_fun() const {
	if (!fun) return cp_function();
	return *fun;
}

cp_bool* RuntimeValue::get_raw_b() {
	return b;
}

cp_int* RuntimeValue::get_raw_i() {
	return i;
}

cp_float* RuntimeValue::get_raw_f() {
	return f;
}

cp_char* RuntimeValue::get_raw_c() {
	return c;
}

cp_string* RuntimeValue::get_raw_s() {
	return s;
}

cp_array* RuntimeValue::get_raw_arr() {
	return arr;
}

cp_struct* RuntimeValue::get_raw_str() {
	return str;
}

cp_function* RuntimeValue::get_raw_fun() {
	return fun;
}

void RuntimeValue::set_type(Type type) {
	this->type = type;
}

void RuntimeValue::set_arr_type(Type arr_type) {
	this->array_type = arr_type;
}

void RuntimeValue::unset() {
	if (this->b && reinterpret_cast<uintptr_t>(this->b) != 0xdddddddddddddddd) {
		delete this->b;
		this->b = nullptr;
	}
	if (this->i && reinterpret_cast<uintptr_t>(this->i) != 0xdddddddddddddddd) {
		delete this->i;
		this->i = nullptr;
	}
	if (this->f && reinterpret_cast<uintptr_t>(this->f) != 0xdddddddddddddddd) {
		delete this->f;
		this->f = nullptr;
	}
	if (this->c && reinterpret_cast<uintptr_t>(this->c) != 0xdddddddddddddddd) {
		delete this->c;
		this->c = nullptr;
	}
	if (this->s && reinterpret_cast<uintptr_t>(this->s) != 0xdddddddddddddddd) {
		delete this->s;
		this->s = nullptr;
	}
	if (this->arr && reinterpret_cast<uintptr_t>(this->arr) != 0xdddddddddddddddd) {
		delete this->arr;
		this->arr = nullptr;
	}
	if (this->str && reinterpret_cast<uintptr_t>(this->str) != 0xdddddddddddddddd) {
		delete this->str;
		this->str = nullptr;
	}
	if (this->fun && reinterpret_cast<uintptr_t>(this->fun) != 0xdddddddddddddddd) {
		delete this->fun;
		this->fun = nullptr;
	}
}


void RuntimeValue::set_null() {
	auto v = RuntimeValue(Type::T_VOID);
	copy_from(&v);
}

void RuntimeValue::set_undefined() {
	auto v = RuntimeValue(Type::T_UNDEFINED);
	copy_from(&v);
}

bool RuntimeValue::has_value() {
	return type != Type::T_UNDEFINED
		&& type != Type::T_VOID;
}

long double RuntimeValue::value_hash() const {
	switch (type) {
	case Type::T_UNDEFINED:
		throw std::runtime_error("value is undefined");
	case Type::T_VOID:
		throw std::runtime_error("value is null");
	case Type::T_BOOL:
		return (long double)get_b();
	case Type::T_INT:
		return (long double)get_i();
	case Type::T_FLOAT:
		return (long double)get_f();
	case Type::T_CHAR:
		return (long double)get_c();
	case Type::T_STRING:
		return (long double)axe::StringUtils::hashcode(get_s());
	case Type::T_ANY:
		throw std::runtime_error("value is any");
	case Type::T_ARRAY: {
		long double h = 0;
		for (const auto& v : get_arr()) {
			h = h * 31 + v->value_hash();
		}
		return h;
	}
	case Type::T_STRUCT: {
		long double h = 0;
		for (const auto& p : get_str()) {
			h += p.second->value_hash();
		}
		return h;
	}
	default:
		throw std::runtime_error("cannot hash invalid type");
	}
}

void RuntimeValue::copy_from(RuntimeValue* value) {
	type = value->type;
	type_name = value->type_name;
	type_name_space = value->type_name_space;
	array_type = value->array_type;
	dim = value->dim;
	unset();
	switch (type)
	{
	case parser::Type::T_BOOL:
		b = new cp_bool(value->get_b());
		break;
	case parser::Type::T_INT:
		i = new cp_int(value->get_i());
		break;
	case parser::Type::T_FLOAT:
		f = new cp_float(value->get_f());
		break;
	case parser::Type::T_CHAR:
		c = new cp_char(value->get_c());
		break;
	case parser::Type::T_STRING:
		s = new cp_string(value->get_s());
		break;
	case parser::Type::T_ARRAY:
		arr = new cp_array(value->get_arr());
		break;
	case parser::Type::T_STRUCT:
		str = new cp_struct(value->get_str());
		break;
	case parser::Type::T_FUNCTION:
		fun = new cp_function(value->get_fun());
		break;
	default:
		break;
	}
	ref = value->ref;
	use_ref = value->use_ref;
}

std::vector<GCObject*> RuntimeValue::get_references() {
	std::vector<GCObject*> references;

	if (is_array(type)) {
		for (const auto& val : get_arr()) {
			references.push_back(val);
		}
	}
	if (is_struct(type)) {
		for (const auto& sub : get_str()) {
			references.push_back(sub.second);
		}
	}

	return references;
}

RuntimeVariable::RuntimeVariable(const std::string& identifier, parser::Type type, parser::Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
	const std::string& type_name, const std::string& type_name_space)
	: Variable(identifier, def_type(type), def_array_type(array_type, dim),
		std::move(dim), type_name, type_name_space),
	value(nullptr) {
}

RuntimeVariable::RuntimeVariable(const std::string& identifier, TypeDefinition v)
	: Variable(identifier, def_type(v.type), def_array_type(v.array_type, dim),
		v.dim, v.type_name, v.type_name_space),
	value(nullptr) {
}

RuntimeVariable::RuntimeVariable()
	: Variable("", Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<std::shared_ptr<ASTExprNode>>(), "", ""),
	value(nullptr) {
}

RuntimeVariable::~RuntimeVariable() = default;

void RuntimeVariable::set_value(RuntimeValue* val) {
	value = val;
	value->ref = shared_from_this();
}

RuntimeValue* RuntimeVariable::get_value() {
	value->ref = shared_from_this();
	return value;
}

Type RuntimeVariable::def_type(Type type) {
	return is_void(type) || is_undefined(type) ? Type::T_ANY : type;
}

Type RuntimeVariable::def_array_type(Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

void RuntimeVariable::reset_ref() {
	TypeDefinition::reset_ref();
	use_ref = value && (use_ref || is_struct(value->type));
}

std::vector<GCObject*> RuntimeVariable::get_references() {
	std::vector<GCObject*> references;

	references.push_back(value);

	return references;
}

cp_bool RuntimeOperations::equals_value(const RuntimeValue* lval, const RuntimeValue* rval) {
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
	case Type::T_STRUCT:
		return lval == rval;
	}
	return false;
}

std::string RuntimeOperations::parse_value_to_string(const RuntimeValue* value, std::vector<uintptr_t> printed) {
	std::string str = "";
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
			s << value->type_name << "{...}";
			str = s.str();
		}
		else {
			printed.push_back(reinterpret_cast<uintptr_t>(value));
			str = RuntimeOperations::parse_struct_to_string(value, printed);
		}
		break;
	}
	case Type::T_ARRAY: {
		if (std::find(printed.begin(), printed.end(), reinterpret_cast<uintptr_t>(value)) != printed.end()) {
			str = "[...]";
		}
		else {
			printed.push_back(reinterpret_cast<uintptr_t>(value));
			str = RuntimeOperations::parse_array_to_string(value->get_arr(), printed);
		}
		break;
	}
	case Type::T_FUNCTION: {
		str = value->get_fun().first + (value->get_fun().first.empty() ? "" : "::") + value->get_fun().second + "(...)";
		break;
	}
	case Type::T_UNDEFINED:
		throw std::runtime_error("undefined expression");
	default:
		throw std::runtime_error("can't determine value type on parsing");
	}
	return str;
}

std::string RuntimeOperations::parse_array_to_string(const cp_array& arr_value, std::vector<uintptr_t> printed) {
	std::stringstream s = std::stringstream();
	s << "[";
	for (auto i = 0; i < arr_value.size(); ++i) {
		bool isc = is_char(arr_value[i]->type);
		bool iss = is_string(arr_value[i]->type);

		if (isc) s << "'";
		else if (iss) s << "\"";

		s << parse_value_to_string(arr_value[i], printed);

		if (isc) s << "'";
		else if (iss) s << "\"";

		if (i < arr_value.size() - 1) {
			s << ",";
		}
	}
	s << "]";
	return s.str();
}

std::string RuntimeOperations::parse_struct_to_string(const RuntimeValue* value, std::vector<uintptr_t> printed) {
	auto str_value = value->get_str();
	std::stringstream s = std::stringstream();
	if (!value->type_name_space.empty() && value->type_name_space != default_namespace) {
		s << value->type_name_space << "::";
	}
	s << value->type_name << "<" << value << ">{";
	for (auto const& [key, val] : str_value) {
		s << key + ":";
		s << parse_value_to_string(val, printed);
		s << ",";
	}
	auto rs = s.str();
	if (rs[rs.size() - 1] != '{') {
		s.seekp(-1, std::ios_base::end);
	}
	s << "}";
	return s.str();
}

RuntimeValue* RuntimeOperations::do_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, dim_eval_func_t evaluate_access_vector_ptr, bool is_expr, cp_int str_pos) {
	Type l_var_type = lval->ref.lock() ? lval->ref.lock()->type : lval->type;
	Type l_var_array_type = lval->ref.lock() ? lval->ref.lock()->array_type : lval->array_type;
	Type l_type = is_undefined(lval->type) ? l_var_type : lval->type;
	Type r_var_type = rval->ref.lock() ? rval->ref.lock()->type : rval->type;
	Type r_var_array_type = rval->ref.lock() ? rval->ref.lock()->array_type : rval->array_type;
	Type r_type = rval->type;
	bool has_string_access = str_pos >= 0;
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
			res_value = new RuntimeValue(do_relational_operation(op, lval, rval, evaluate_access_vector_ptr));

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
			lval->set(do_relational_operation(op, lval, rval, evaluate_access_vector_ptr));

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
		if (is_any(l_var_type) && op == "=" && !has_string_access) {
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
			if (has_string_access) {
				if (op != "=") {
					ExceptionHandler::throw_operation_err(op, *lval, *rval, evaluate_access_vector_ptr);
				}
				has_string_access = false;
				lval->get_s()[str_pos] = rval->get_c();
				lval->set(lval->get_s());
			}
			else {
				lval->set(do_operation(lval->get_s(), std::string{ rval->get_c() }, op));
			}
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

cp_bool RuntimeOperations::do_relational_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval, dim_eval_func_t evaluate_access_vector_ptr) {
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

cp_int RuntimeOperations::do_spaceship_operation(const std::string& op, RuntimeValue* lval, RuntimeValue* rval) {
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

cp_int RuntimeOperations::do_operation(cp_int lval, cp_int rval, const std::string& op) {
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

cp_float RuntimeOperations::do_operation(cp_float lval, cp_float rval, const std::string& op) {
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

cp_string RuntimeOperations::do_operation(cp_string lval, cp_string rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		return lval + rval;
	}
	throw std::runtime_error("invalid '" + op + "' operator for types 'string' and 'string'");
}

cp_array RuntimeOperations::do_operation(cp_array lval, cp_array rval, const std::string& op) {
	if (op == "=") {
		return rval;
	}
	else if (op == "+=" || op == "+") {
		lval.insert(lval.end(), rval.begin(), rval.end());

		return lval;
	}

	throw std::runtime_error("invalid '" + op + "' operator for types 'array' and 'array'");
}

void RuntimeOperations::normalize_type(std::shared_ptr<RuntimeVariable> var, RuntimeValue* val) {
	if (is_string(var->type) && is_char(val->type)) {
		val->type = var->type;
		val->set(cp_string{ val->get_c() });
	}
	else if (is_float(var->type) && is_int(val->type)) {
		val->type = var->type;
		val->set(cp_float(val->get_i()));
	}
}


std::string RuntimeOperations::build_str_type(RuntimeValue* curr_value, dim_eval_func_t evaluate_access_vector_ptr) {
	auto dim = std::vector<unsigned int>();
	auto type = is_void(curr_value->type) ? curr_value->type : curr_value->type;
	std::string str_type = "";

	if (is_array(type)) {
		dim = evaluate_access_vector_ptr(curr_value->dim);
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
