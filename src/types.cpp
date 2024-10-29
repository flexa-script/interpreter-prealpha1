#include "vendor/axeutils.hpp"

#include "module.hpp";
#include "datetime.hpp"
#include "graphics.hpp"
#include "files.hpp"
#include "console.hpp"

#include "types.hpp"
#include "visitor.hpp"
#include "token.hpp"

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

TypeDefinition::TypeDefinition(Type type, Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: type(type), array_type(array_type), dim(dim), type_name(type_name), type_name_space(type_name_space) {
	reset_ref();
}

TypeDefinition::TypeDefinition(Type type)
	: type(type), array_type(Type::T_UNDEFINED), dim(std::vector<void*>()), type_name(""), type_name_space("") {
	reset_ref();
}

TypeDefinition::TypeDefinition()
	: type(Type::T_UNDEFINED), array_type(Type::T_UNDEFINED), dim(std::vector<void*>()), type_name(""), type_name_space("") {
	reset_ref();
}

TypeDefinition TypeDefinition::get_basic(Type type) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<void*>(), "", "");
}

TypeDefinition TypeDefinition::get_array(Type array_type, const std::vector<void*>& dim) {
	return TypeDefinition(Type::T_ARRAY, array_type, dim, "", "");
}

TypeDefinition TypeDefinition::get_struct(const std::string& type_name, const std::string& type_name_space) {
	return TypeDefinition(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<void*>(), type_name, type_name_space);
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
		Type::T_UNDEFINED, std::vector<void*>(), ltype.type_name, ltype.type_name_space);
	TypeDefinition ratype = TypeDefinition(is_undefined(rtype.array_type) ? Type::T_ANY : rtype.array_type,
		Type::T_UNDEFINED, std::vector<void*>(), rtype.type_name, rtype.type_name_space);

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

ParameterDefinition::ParameterDefinition(Type type, Type array_type, const std::vector<void*>& dim, const std::string& type_name,
	const std::string& type_name_space, void* assign_value, bool is_rest)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space), assign_value(assign_value), is_rest(is_rest) {}

ParameterDefinition::ParameterDefinition(TypeDefinition type_definition, void* assign_value, bool is_rest)
	: TypeDefinition(type_definition), assign_value(assign_value), is_rest(is_rest) {}

VariableDefinition::VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<void*>& dim,
	void* default_value, bool is_rest, unsigned int row, unsigned int col)
	: ParameterDefinition(type, array_type, dim, type_name, type_name_space, default_value, is_rest), CodePosition(row, col),
	identifier(identifier) {}

VariableDefinition::VariableDefinition()
	: ParameterDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<void*>(), "", "", nullptr), CodePosition(),
	identifier("") {}

VariableDefinition VariableDefinition::get_basic(const std::string& identifier, Type type,
	void* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", Type::T_UNDEFINED, std::vector<void*>(), default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_array(const std::string& identifier, parser::Type array_type,
	const std::vector<void*>& dim, void* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, Type::T_ARRAY, "", "", array_type, dim, default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_struct(const std::string& identifier,
	const std::string& type_name, const std::string& type_name_space,
	void* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, Type::T_STRUCT, type_name, type_name_space, Type::T_UNDEFINED,
		std::vector<void*>(), default_value, is_rest, row, col);
}

UnpackedVariableDefinition::UnpackedVariableDefinition(Type type, Type array_type, const std::vector<void*>& dim, const std::string& type_name,
	const std::string& type_name_space, const std::vector<VariableDefinition>& variables, void* expr_value)
	: ParameterDefinition(type, array_type, dim, type_name, type_name_space, expr_value), variables(variables) {}

UnpackedVariableDefinition::UnpackedVariableDefinition(TypeDefinition type_definition, const std::vector<VariableDefinition>& variables, void* expr_value)
	: ParameterDefinition(type_definition, expr_value), variables(variables) {}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<void*>& dim,
	const std::vector<TypeDefinition>& signature, const std::vector<ParameterDefinition>& parameters,
	ASTBlockNode* block, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), parameters(parameters), signature(signature), block(block) {
	check_signature();
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type,
	const std::vector<TypeDefinition>& signature, const std::vector<ParameterDefinition>& parameters)
	: CodePosition(), TypeDefinition(type),
	identifier(identifier), parameters(parameters), signature(signature), block(nullptr) {
	check_signature();
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<void*>& dim)
	: CodePosition(), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier) {}

FunctionDefinition::FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(Type::T_ANY, Type::T_UNDEFINED, std::vector<void*>(), "", ""),
	identifier(identifier), parameters(std::vector<ParameterDefinition>()), block(nullptr), is_var(true) {}

FunctionDefinition::FunctionDefinition()
	: TypeDefinition(), CodePosition(),
	identifier(""), parameters(std::vector<ParameterDefinition>()), block(nullptr) {}

void FunctionDefinition::check_signature() const {
	bool has_default = false;
	for (size_t i = 0; i < parameters.size(); ++i) {
		if (parameters[i].is_rest && parameters.size() - 1 != i) {
			throw std::runtime_error("rest '" + identifier + "' parameter must be the last parameter");
		}
		if (parameters[i].assign_value) {
			has_default = true;
		}
		if (!parameters[i].assign_value && has_default) {
			throw std::runtime_error("default values as '" + identifier + "' must be at end");
		}
	}
}

StructureDefinition::StructureDefinition(const std::string& identifier, const std::map<std::string, VariableDefinition>& variables,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), identifier(identifier), variables(variables) {}

StructureDefinition::StructureDefinition(const std::string& identifier)
	: CodePosition(), identifier(identifier) {}

StructureDefinition::StructureDefinition()
	: CodePosition(row, col), identifier(""), variables(std::map<std::string, VariableDefinition>()) {}

Variable::Variable(const std::string& identifier, Type type, Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier) {}

Variable::Variable(TypeDefinition value)
	: TypeDefinition(value) {}

Variable::Variable()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<void*>(), "", "") {}

Value::Value(Type type, Type array_type, std::vector<void*> dim,
	const std::string& type_name, const std::string& type_name_space)
	: TypeDefinition(type, array_type, dim, type_name, type_name_space) {}

Value::Value(TypeDefinition type)
	: TypeDefinition(type) {}

Value::Value()
	: TypeDefinition() {}

SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type, array_type, dim, type_name, type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(parser::Type type, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type),
	hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(TypeDefinition type_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	Value(type_definition.type, type_definition.array_type,
		type_definition.dim, type_definition.type_name, type_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(VariableDefinition variable_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	Value(variable_definition.type, variable_definition.array_type,
		variable_definition.dim, variable_definition.type_name, variable_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(Type type, unsigned int row, unsigned int col)
	: CodePosition(row, col), Value(type),
	hash(0), is_const(false), is_sub(false) {}

SemanticValue::SemanticValue()
	: CodePosition(), Value(),
	hash(0), is_const(false), is_sub(false) {}

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

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<void*>& dim,
	const std::string& type_name, const std::string& type_name_space, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Variable(identifier, def_type(type), def_array_type(array_type, dim), dim, type_name, type_name_space),
	value(nullptr), is_const(is_const) {}

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), Variable(identifier, def_type(type), Type::T_UNDEFINED, std::vector<void*>(), "", ""),
	value(nullptr), is_const(is_const) {}

SemanticVariable::SemanticVariable()
	: CodePosition(0, 0), Variable("", Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<void*>(), "", ""),
	value(nullptr), is_const(false) {}

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

Type SemanticVariable::def_array_type(Type array_type, const std::vector<void*>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

void SemanticVariable::reset_ref() {
	TypeDefinition::reset_ref();
	use_ref = value && (use_ref || is_struct(value->type));
}


RuntimeValue::RuntimeValue(Type type, Type array_type, std::vector<void*> dim,
	const std::string& type_name, const std::string& type_name_space,
	unsigned int row, unsigned int col)
	: Value(type, array_type, dim, type_name, type_name_space) {}

RuntimeValue::RuntimeValue()
	: Value(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<void*>(), "", "") {}

RuntimeValue::RuntimeValue(cp_bool rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_int rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_float rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_char rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_string rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_array rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(cp_array rawv, Type array_type, std::vector<void*> dim, std::string type_name, std::string type_name_space) {
	set(rawv, array_type, dim, type_name, type_name_space);
}

RuntimeValue::RuntimeValue(cp_struct rawv, std::string type_name, std::string type_name_space) {
	set(rawv, type_name, type_name_space);
}

RuntimeValue::RuntimeValue(cp_function rawv) {
	set(rawv);
}

RuntimeValue::RuntimeValue(Type type)
	: Value(type, Type::T_UNDEFINED, std::vector<void*>(), "", "") {}

RuntimeValue::RuntimeValue(Type array_type, std::vector<void*> dim, std::string type_name, std::string type_name_space)
	: Value(Type::T_ARRAY, array_type, dim, type_name, type_name_space) {}

RuntimeValue::RuntimeValue(std::string type_name, std::string type_name_space)
	: Value(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<void*>(), type_name, type_name_space) {}

RuntimeValue::RuntimeValue(RuntimeValue* v) {
	copy_from(v);
}

RuntimeValue::RuntimeValue(TypeDefinition v)
	: Value(v) {}

RuntimeValue::~RuntimeValue() = default;

void RuntimeValue::set(cp_bool b) {
	unset();
	this->b = b;
	type = Type::T_BOOL;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_int i) {
	unset();
	this->i = i;
	type = Type::T_INT;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_float f) {
	unset();
	this->f = f;
	type = Type::T_FLOAT;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_char c) {
	unset();
	this->c = c;
	type = Type::T_CHAR;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_string s) {
	unset();
	this->s = s;
	type = Type::T_STRING;
	array_type = Type::T_UNDEFINED;
}

void RuntimeValue::set(cp_array arr) {
	unset();
	this->arr = arr;
	type = Type::T_ARRAY;
}

void RuntimeValue::set(cp_array arr, Type array_type, std::vector<void*> dim, std::string type_name, std::string type_name_space) {
	unset();
	this->arr = arr;
	type = Type::T_ARRAY;
	this->array_type = array_type;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(cp_struct str, std::string type_name, std::string type_name_space) {
	unset();
	this->str = str;
	type = Type::T_STRUCT;
	array_type = Type::T_UNDEFINED;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void RuntimeValue::set(cp_function fun) {
	unset();
	this->fun = fun;
	type = Type::T_FUNCTION;
	array_type = Type::T_UNDEFINED;
}

cp_bool RuntimeValue::get_b() const {
	return b;
}

cp_int RuntimeValue::get_i() const {
	return i;
}

cp_float RuntimeValue::get_f() const {
	return f;
}

cp_char RuntimeValue::get_c() const {
	return c;
}

cp_string RuntimeValue::get_s() const {
	return s;
}

cp_array RuntimeValue::get_arr() const {
	return arr;
}

cp_struct RuntimeValue::get_str() const {
	return str;
}

cp_function RuntimeValue::get_fun() const {
	return fun;
}

void RuntimeValue::set_type(Type type) {
	this->type = type;
}

void RuntimeValue::set_arr_type(Type arr_type) {
	this->array_type = arr_type;
}

void RuntimeValue::unset() {
	this->b = false;
	this->i = 0;
	this->f = 0l;
	this->c = '\0';
	this->s = "";
	this->arr = cp_array();
	this->str = cp_struct();
	this->fun = cp_function();
}

void RuntimeValue::set_null() {
	copy_from(new RuntimeValue(Type::T_VOID));
}

void RuntimeValue::set_undefined() {
	copy_from(new RuntimeValue(Type::T_UNDEFINED));
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
		return (long double)b;
	case Type::T_INT:
		return (long double)i;
	case Type::T_FLOAT:
		return (long double)f;
	case Type::T_CHAR:
		return (long double)c;
	case Type::T_STRING:
		return (long double)axe::StringUtils::hashcode(s);
	case Type::T_ANY:
		throw std::runtime_error("value is any");
	case Type::T_ARRAY: {
		long double h = 0;
		for (size_t i = 0; i < arr.size(); ++i) {
			h = h * 31 + arr[i]->value_hash();
		}
		return h;
	}
	case Type::T_STRUCT: {
		long double h = 0;
		for (const auto& v : str) {
			h += v.second->value_hash();
		}
		return h;
	}
	default:
		throw std::runtime_error("invalid type encountered");
	}
}

void RuntimeValue::copy_array(cp_array arr) {
	if (arr.size() == 0) {
		this->arr = cp_array();
		return;
	}

	auto rarr = cp_array(arr.size());

	for (size_t i = 0; i < arr.size(); ++i) {
		RuntimeValue* currval = arr[i];
		RuntimeValue* val = currval ? new RuntimeValue(currval) : nullptr;
		rarr[i] = val;
	}

	this->arr = cp_array(rarr);
}

void RuntimeValue::copy_from(RuntimeValue* value) {
	type = value->type;
	type_name = value->type_name;
	type_name_space = value->type_name_space;
	array_type = value->array_type;
	dim = value->dim;
	b = value->b;
	i = value->i;
	f = value->f;
	c = value->c;
	s = value->s;
	//copy_array(value->arr);
	arr = value->arr;
	str = value->str;
	fun = value->fun;
	ref = value->ref;
	use_ref = value->use_ref;
}

bool RuntimeValue::equals_array(cp_array arr) {
	if (this->arr.size() != arr.size()) {
		return false;
	}

	for (size_t i = 0; i < arr.size(); ++i) {
		if (!this->arr[i]->equals(arr[i])) {
			return false;
		}
	}

	return true;
}

bool RuntimeValue::equals(RuntimeValue* value) {
	return type == value->type &&
		type_name == value->type_name &&
		type_name_space == value->type_name_space &&
		array_type == value->array_type &&
		b == value->b &&
		i == value->i &&
		f == value->f &&
		c == value->c &&
		s == value->s &&
		equals_array(value->arr) &&
		str == value->str;
}

std::vector<GCObject*> RuntimeValue::get_references() {
	std::vector<GCObject*> references;

	if (is_array(type)) {
		for (size_t i = 0; i < arr.size(); ++i) {
			references.push_back(arr[i]);
		}
	}
	if (is_struct(type)) {
		for (const auto& sub : str) {
			references.push_back(sub.second);
		}
	}

	return references;
}

RuntimeVariable::RuntimeVariable(const std::string& identifier, parser::Type type, parser::Type array_type, std::vector<void*> dim,
	const std::string& type_name, const std::string& type_name_space)
	: Variable(identifier, def_type(type), def_array_type(array_type, dim),
		std::move(dim), type_name, type_name_space),
	value(nullptr) {}

RuntimeVariable::RuntimeVariable(const std::string& identifier, TypeDefinition v)
	: Variable(identifier, def_type(v.type), def_array_type(v.array_type, dim),
		v.dim, v.type_name, v.type_name_space),
	value(nullptr) {}

RuntimeVariable::RuntimeVariable()
	: Variable("", Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<void*>(), "", ""),
	value(nullptr) {}

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

Type RuntimeVariable::def_array_type(Type array_type, const std::vector<void*>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

void RuntimeVariable::reset_ref() {
	TypeDefinition::reset_ref();
	use_ref = value && (use_ref || is_struct(value->type));
}
