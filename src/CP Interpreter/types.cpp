#include "vendor/axeutils.hpp"

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

std::string default_namespace = "__main";

std::vector<std::string> std_libs = {
	"cp.std.math",
	"cp.std.print",
	"cp.std.random",
	"cp.std.testing"
};

std::vector<std::string> built_in_libs = {
	"cp.core.graphics",
	"cp.core.files",
	"cp.core.console",
	"cp.core.exception",
	"cp.core.pair"
};

TypeDefinition::TypeDefinition(Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space)
	: type(type), array_type(array_type), dim(dim), type_name(type_name), type_name_space(type_name_space) {
	reset_ref();
}

TypeDefinition::TypeDefinition()
	: type(Type::T_UNDEFINED), array_type(Type::T_UNDEFINED), dim(std::vector<ASTExprNode*>()), type_name(""), type_name_space("") {
	reset_ref();
}

TypeDefinition TypeDefinition::get_basic(Type type) {
	return TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "");
}

TypeDefinition TypeDefinition::get_array(Type array_type, std::vector<ASTExprNode*>&& dim) {
	return TypeDefinition(Type::T_ARRAY, array_type, std::move(dim), "", "");
}

TypeDefinition TypeDefinition::get_struct(const std::string& type_name, const std::string& type_name_space) {
	return TypeDefinition(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), type_name, type_name_space);
}

bool TypeDefinition::is_any_or_match_type(TypeDefinition* lvtype, TypeDefinition ltype, TypeDefinition* rvtype, TypeDefinition rtype,
	dim_eval_func_t evaluate_access_vector, bool strict, bool strict_array) {
	if (lvtype && is_any(lvtype->type)
		|| rvtype && is_any(rvtype->type)
		|| is_any(ltype.type)
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
		Type::T_UNDEFINED, std::vector<ASTExprNode*>(), ltype.type_name, ltype.type_name_space);
	TypeDefinition ratype = TypeDefinition(is_undefined(rtype.array_type) ? Type::T_ANY : rtype.array_type,
		Type::T_UNDEFINED, std::vector<ASTExprNode*>(), rtype.type_name, rtype.type_name_space);

	return is_array(ltype.type) && is_array(rtype.type)
		&& (!strict_array && is_any_or_match_type(&latype, latype, nullptr, ratype, evaluate_access_vector, strict, strict_array) ||
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

VariableDefinition::VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space),
	identifier(identifier), default_value(default_value), is_rest(is_rest) {}

VariableDefinition::VariableDefinition()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(""), default_value(nullptr), is_rest(false) {}

VariableDefinition VariableDefinition::get_basic(const std::string& identifier, Type type,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, type, "", "", Type::T_UNDEFINED, std::vector<ASTExprNode*>(), default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_array(const std::string& identifier, parser::Type array_type,
	std::vector<ASTExprNode*>&& dim, ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, Type::T_ARRAY, "", "", array_type, std::move(dim), default_value, is_rest, row, col);
}

VariableDefinition VariableDefinition::get_struct(const std::string& identifier,
	const std::string& type_name, const std::string& type_name_space,
	ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col) {
	return VariableDefinition(identifier, Type::T_STRUCT, type_name, type_name_space, Type::T_UNDEFINED,
		std::vector<ASTExprNode*>(), default_value, is_rest, row, col);
}

FunctionDefinition::FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
	const std::string& type_name_space, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), parameters(parameters), signature(signature) {}

FunctionDefinition::FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(Type::T_ANY, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(identifier), signature(std::vector<TypeDefinition>()), parameters(std::vector<VariableDefinition>()), is_var(true) {}

FunctionDefinition::FunctionDefinition()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""), CodePosition(0, 0),
	identifier(""), signature(std::vector<TypeDefinition>()), parameters(std::vector<VariableDefinition>()) {}

void FunctionDefinition::check_signature() const {
	bool has_default = false;
	for (size_t i = 0; i < parameters.size(); ++i) {
		if (parameters[i].is_rest && parameters.size() - 1 != i) {
			throw std::runtime_error("'" + identifier + "': the rest parameter must be the last parameter");
		}
		if (parameters[i].default_value) {
			has_default = true;
		}
		if (!parameters[i].default_value && has_default) {
			throw std::runtime_error("'" + identifier + "': the rest parameter must be the last parameter");
		}
	}
}

StructureDefinition::StructureDefinition(const std::string& identifier, const std::map<std::string, VariableDefinition>& variables,
	unsigned int row, unsigned int col)
	: CodePosition(row, col), identifier(identifier), variables(variables) {}

StructureDefinition::StructureDefinition()
	: CodePosition(row, col), identifier(""), variables(std::map<std::string, VariableDefinition>()) {}

SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	ref(nullptr), hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(TypeDefinition type_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	TypeDefinition(type_definition.type, type_definition.array_type,
		type_definition.dim, type_definition.type_name, type_definition.type_name_space),
	ref(nullptr), hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(VariableDefinition variable_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	TypeDefinition(variable_definition.type, variable_definition.array_type,
		variable_definition.dim, variable_definition.type_name, variable_definition.type_name_space),
	ref(nullptr), hash(hash), is_const(is_const), is_sub(false) {}

SemanticValue::SemanticValue(Type type, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	ref(nullptr), hash(0), is_const(false), is_sub(false) {}

SemanticValue::SemanticValue()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	ref(nullptr), hash(0), is_const(false), is_sub(false) {}

void SemanticValue::copy_from(SemanticValue* value) {
	type = value->type;
	array_type = value->array_type;
	dim = value->dim;
	type_name = value->type_name;
	type_name_space = value->type_name_space;
	hash = value->hash;
	is_const = value->is_const;
	ref = value->ref;
	is_sub = value->is_sub;
	row = value->row;
	col = value->col;
}

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(def_type(type), def_array_type(array_type, dim), dim, type_name, type_name_space),
	identifier(identifier), value(value), is_const(is_const) {
	value->ref = this;
}

SemanticVariable::SemanticVariable()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(""), value(nullptr), is_const(false) {}

Type SemanticVariable::def_type(Type type) {
	return is_void(type) || is_undefined(type) ? Type::T_ANY : type;
}

Type SemanticVariable::def_array_type(Type array_type, const std::vector<ASTExprNode*>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

Value::Value(Type type, Type array_type, std::vector<ASTExprNode*> dim,
	const std::string& type_name, const std::string& type_name_space,
	unsigned int row, unsigned int col)
	: TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space) {}

Value::Value()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "") {}

Value::Value(cp_bool rawv) {
	set(rawv);
}

Value::Value(cp_int rawv) {
	set(rawv);
}

Value::Value(cp_float rawv) {
	set(rawv);
}

Value::Value(cp_char rawv) {
	set(rawv);
}

Value::Value(cp_string rawv) {
	set(rawv);
}

Value::Value(cp_array rawv, Type array_type, std::vector<ASTExprNode*> dim) {
	set(rawv, array_type, dim);
}

Value::Value(cp_struct rawv, std::string type_name, std::string type_name_space) {
	set(rawv, type_name, type_name_space);
}

Value::Value(cp_function rawv) {
	set(rawv);
}

Value::Value(Variable* rawv) {
	set(rawv);
}

Value::Value(Type type)
	: TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "") {}

Value::Value(Type array_type, std::vector<ASTExprNode*> dim, std::string type_name, std::string type_name_space)
	: TypeDefinition(Type::T_ARRAY, array_type, dim, type_name, type_name_space) {}

Value::Value(std::string type_name, std::string type_name_space)
	: TypeDefinition(Type::T_STRUCT, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), type_name, type_name_space) {}

Value::Value(Value* v) {
	copy_from(v);
}

Value::Value(TypeDefinition v)
	: TypeDefinition(v.type, v.array_type, v.dim, v.type_name, v.type_name_space) {}

Value::~Value() = default;

void Value::set(cp_bool b) {
	unset();
	this->b = std::shared_ptr<cp_bool>(new cp_bool(b));

	type = Type::T_BOOL;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_int i) {
	unset();
	this->i = std::shared_ptr<cp_int>(new cp_int(i));
	type = Type::T_INT;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_float f) {
	unset();
	this->f = std::shared_ptr<cp_float>(new cp_float(f));
	type = Type::T_FLOAT;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_char c) {
	unset();
	this->c = std::shared_ptr<cp_char>(new cp_char(c));
	type = Type::T_CHAR;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_string s) {
	unset();
	this->s = std::shared_ptr<cp_string>(new cp_string(s));
	type = Type::T_STRING;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_array arr, Type array_type, std::vector<ASTExprNode*> dim, std::string type_name, std::string type_name_space) {
	unset();
	this->arr = std::shared_ptr<cp_array>(new cp_array(arr));
	type = Type::T_ARRAY;
	this->array_type = array_type;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void Value::set(cp_struct str, std::string type_name, std::string type_name_space) {
	unset();
	this->str = std::shared_ptr<cp_struct>(new cp_struct(str));
	type = Type::T_STRUCT;
	array_type = Type::T_UNDEFINED;
	this->type_name = type_name;
	this->type_name_space = type_name_space;
}

void Value::set(cp_function fun) {
	unset();
	this->fun = std::shared_ptr<cp_function>(new cp_function(fun));
	type = Type::T_FUNCTION;
	array_type = Type::T_UNDEFINED;
}

cp_bool Value::get_b() const {
	if (b) {
		return *b;
	}
	return false;
}

cp_int Value::get_i() const {
	if (i) {
		return *i;
	}
	return 0;
}

cp_float Value::get_f() const {
	if (f) {
		return *f;
	}
	return 0;
}

cp_char Value::get_c() const {
	if (c) {
		return *c;
	}
	return '\0';
}

cp_string Value::get_s() const {
	if (s) {
		return *s;
	}
	return "";
}

cp_array Value::get_arr() const {
	if (arr) {
		return *arr;
	}
	return cp_array();
}

cp_struct Value::get_str() const {
	if (str) {
		return *str;
	}
	return cp_struct();
}

cp_function Value::get_fun() const {
	if (fun) {
		return *fun;
	}
	return cp_function();
}

void Value::set_type(Type type) {
	this->type = type;
}

void Value::set_arr_type(Type arr_type) {
	this->array_type = arr_type;
}

void Value::unset() {
	this->b = nullptr;
	this->i = nullptr;
	this->f = nullptr;
	this->c = nullptr;
	this->s = nullptr;
	this->arr = nullptr;
	this->str = nullptr;
	this->fun = nullptr;
}

void Value::set_null() {
	copy_from(new Value(Type::T_VOID));
}

void Value::set_undefined() {
	copy_from(new Value(Type::T_UNDEFINED));
}

bool Value::has_value() {
	return type != Type::T_UNDEFINED
		&& type != Type::T_VOID;
}

long double Value::value_hash() const {
	switch (type) {
	case Type::T_UNDEFINED:
		throw std::runtime_error("value is undefined");
	case Type::T_VOID:
		throw std::runtime_error("value is null");
	case Type::T_BOOL:
		return (long double)*b;
	case Type::T_INT:
		return (long double)*i;
	case Type::T_FLOAT:
		return (long double)*f;
	case Type::T_CHAR:
		return (long double)*c;
	case Type::T_STRING:
		return (long double)axe::StringUtils::hashcode(*s);
	case Type::T_ANY:
		throw std::runtime_error("value is any");
	case Type::T_ARRAY: {
		long double h = 0;
		for (size_t i = 0; i < arr->second; ++i) {
			h += arr->first[i]->value_hash();
		}
		return h;
	}
	case Type::T_STRUCT: {
		long double h = 0;
		for (const auto& v : *str) {
			h += v.second->value_hash();
		}
		return h;
	}
	default:
		throw std::runtime_error("invalid type encountered");
	}
}

void Value::copy_array(std::shared_ptr<cp_array> arr) {
	if (!arr) {
		this->arr = nullptr;
		return;
	}

	auto rarr = new Value * [arr->second];

	for (size_t i = 0; i < arr->second; ++i) {
		Value* currval = arr->first[i];
		Value* val = currval ? new Value(currval) : nullptr;
		rarr[i] = val;
	}

	this->arr = std::shared_ptr<cp_array>(new cp_array(rarr, arr->second));
}

void Value::copy_from(Value* value) {
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
	copy_array(value->arr);
	str = value->str;
	fun = value->fun;
	ref = value->ref;
	use_ref = value->use_ref;
}

bool Value::equals_array(cp_array arr) {
	if (this->arr->second != arr.second) {
		return false;
	}

	for (size_t i = 0; i < arr.second; ++i) {
		if (!this->arr->first[i]->equals(arr.first[i])) {
			return false;
		}
	}

	return true;
}

bool Value::equals(Value* value) {
	return type == value->type &&
		type_name == value->type_name &&
		type_name_space == value->type_name_space &&
		array_type == value->array_type &&
		b == value->b &&
		i == value->i &&
		f == value->f &&
		c == value->c &&
		s == value->s &&
		equals_array(*value->arr) &&
		str == value->str;
}

Variable::Variable(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	const std::string& type_name, const std::string& type_name_space, Value* value)
	: TypeDefinition(def_type(type), def_array_type(array_type, dim),
		std::move(dim), type_name, type_name_space),
	value(value) {
	value->ref = this;
}

Variable::Variable()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	value(nullptr) {}

Variable::Variable(Value* v)
	: TypeDefinition(def_type(v->type), def_array_type(v->array_type, dim),
		v->dim, v->type_name, v->type_name_space),
	value(v) {
	value->ref = this;
}

Variable::~Variable() = default;

void Variable::set_value(Value* val) {
	value = val;
	value->ref = this;
}

Value* Variable::get_value() {
	value->ref = this;
	return value;
}

Type Variable::def_type(Type type) {
	return is_void(type) || is_undefined(type) ? Type::T_ANY : type;
}

Type Variable::def_array_type(Type array_type, const std::vector<ASTExprNode*>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}
