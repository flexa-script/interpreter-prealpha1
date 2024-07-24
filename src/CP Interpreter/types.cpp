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
	"cp.core.exception",
	"cp.core.pair",
	"cp.core.console"
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

bool TypeDefinition::is_any_or_match_type(TypeDefinition* lvtype, TypeDefinition ltype, TypeDefinition* rvtype, TypeDefinition rtype, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector) {
	if (lvtype && is_any(lvtype->type)
		|| rvtype && is_any(rvtype->type)
		|| is_any(ltype.type)
		|| is_any(rtype.type)
		|| is_void(ltype.type)
		|| is_void(rtype.type)) return true;
	return match_type(ltype, rtype, evaluate_access_vector);
}

bool TypeDefinition::match_type(TypeDefinition ltype, TypeDefinition rtype, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector) {
	if (match_type_bool(ltype, rtype)) return true;
	if (match_type_int(ltype, rtype)) return true;
	if (match_type_float(ltype, rtype)) return true;
	if (match_type_char(ltype, rtype)) return true;
	if (match_type_string(ltype, rtype)) return true;
	if (match_type_array(ltype, rtype, evaluate_access_vector)) return true;
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

bool TypeDefinition::match_type_float(TypeDefinition ltype, TypeDefinition rtype) {
	return is_float(ltype.type)
		&& (is_float(rtype.type) || is_int(rtype.type));
}

bool TypeDefinition::match_type_char(TypeDefinition ltype, TypeDefinition rtype) {
	return is_char(ltype.type) && is_char(rtype.type);
}

bool TypeDefinition::match_type_string(TypeDefinition ltype, TypeDefinition rtype) {
	return is_string(ltype.type)
		&& (is_char(rtype.type) || is_string(rtype.type));
}

bool TypeDefinition::match_type_array(TypeDefinition ltype, TypeDefinition rtype, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector) {
	TypeDefinition latype = TypeDefinition(ltype.array_type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), ltype.type_name, ltype.type_name_space);
	TypeDefinition ratype = TypeDefinition(rtype.array_type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), rtype.type_name, rtype.type_name_space);

	return is_array(ltype.type) && is_array(rtype.type)
		&& is_any_or_match_type(&latype, latype, nullptr, ratype, evaluate_access_vector)
		&& match_array_dim(ltype, rtype, evaluate_access_vector);
}

bool TypeDefinition::match_type_struct(TypeDefinition ltype, TypeDefinition rtype) {
	return is_struct(ltype.type) && is_struct(rtype.type)
		&& ltype.type_name == rtype.type_name
		// todo: check why it is not possible use typename namespace
		//&& ltype.type_name_space == rtype.type_name_space
		;
}

bool TypeDefinition::match_type_function(TypeDefinition ltype, TypeDefinition rtype) {
	return is_function(ltype.type) && is_function(rtype.type);
}

bool TypeDefinition::match_array_dim(TypeDefinition ltype, TypeDefinition rtype, std::function<std::vector<unsigned int>(const std::vector<parser::ASTExprNode*>&)> evaluate_access_vector) {
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


SemanticValue::SemanticValue(parser::Type type, parser::Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
	resetref();
}

SemanticValue::SemanticValue(TypeDefinition type_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	TypeDefinition(type_definition.type, type_definition.array_type,
		type_definition.dim, type_definition.type_name, type_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
	resetref();
}

SemanticValue::SemanticValue(VariableDefinition variable_definition, long long hash,
	bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col),
	TypeDefinition(variable_definition.type, variable_definition.array_type,
		variable_definition.dim, variable_definition.type_name, variable_definition.type_name_space),
	hash(hash), is_const(is_const), is_sub(false) {
	resetref();
}

SemanticValue::SemanticValue(Type type, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	hash(0), is_const(false), is_sub(false) {
	resetref();
}

SemanticValue::SemanticValue()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	hash(0), is_const(false), ref(nullptr), is_sub(false) {}

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

void SemanticValue::resetref() {
	//ref = is_struct(type) || is_struct(array_type);
}

SemanticVariable::SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
	const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col)
	: CodePosition(row, col), TypeDefinition(type, array_type, dim, type_name, type_name_space),
	identifier(identifier), value(value), is_const(is_const) {}

SemanticVariable::SemanticVariable()
	: CodePosition(0, 0), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(""), value(nullptr), is_const(false) {}

void SemanticVariable::copy_from(SemanticVariable* var) {
	identifier = var->identifier;
	type = var->type;
	array_type = var->array_type;
	dim = var->dim;
	type_name = var->type_name;
	is_const = value->is_const;
	row = var->row;
	col = var->col;

	delete value;
	value = new SemanticValue();
	value->copy_from(var->value);
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
	: CodePosition(row, col), TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", ""),
	identifier(identifier), signature(std::vector<TypeDefinition>()), parameters(std::vector<VariableDefinition>()) {}

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

Value::Value(Type type, Type array_type, std::vector<ASTExprNode*> dim,
	const std::string& type_name, const std::string& type_name_space,
	unsigned int row, unsigned int col)
	: TypeDefinition(type, array_type, std::move(dim), type_name, type_name_space) {}

Value::Value()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "") {};

Value::Value(cp_bool rawv) {
	set(rawv);
};

Value::Value(cp_int rawv) {
	set(rawv);
};

Value::Value(cp_float rawv) {
	set(rawv);
};

Value::Value(cp_char rawv) {
	set(rawv);
};

Value::Value(cp_string rawv) {
	set(rawv);
};

Value::Value(cp_array rawv, Type array_type) {
	set(rawv, array_type);
};

Value::Value(cp_struct* rawv) {
	set(rawv);
};

Value::Value(cp_function rawv) {
	set(rawv);
};

Value::Value(Variable* rawv) {
	set(rawv);
};

Value::Value(Type type)
	: TypeDefinition(type, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "") {
};

Value::Value(Type type, Type arr_type, std::vector<ASTExprNode*> dim)
	: TypeDefinition(type, arr_type, dim, "", "") {
};

Value::Value(Value* v)
	: TypeDefinition(v->type, v->array_type, v->dim, v ->type_name, v->type_name_space),
	b(v->b), i(v->i), f(v->f), c(v->c), s(v->s), str(v->str), fun(v->fun) {
	copy_array(v->arr);
};

Value::Value(TypeDefinition v)
	: TypeDefinition(v.type, v.array_type, v.dim, v.type_name, v.type_name_space) {};

Value::~Value() = default;

void Value::set(cp_bool b) {
	this->b = b;
	type = Type::T_BOOL;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_int i) {
	this->i = i;
	type = Type::T_INT;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_float f) {
	this->f = f;
	type = Type::T_FLOAT;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_char c) {
	this->c = c;
	type = Type::T_CHAR;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_string s) {
	this->s = s;
	type = Type::T_STRING;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_array arr, Type array_type) {
	copy_array(arr);
	type = Type::T_ARRAY;
	array_type = array_type;
}

void Value::set(cp_struct* str) {
	this->str = str;
	type = Type::T_STRUCT;
	array_type = Type::T_UNDEFINED;
}

void Value::set(cp_function fun) {
	this->fun = fun;
	type = Type::T_FUNCTION;
	array_type = Type::T_UNDEFINED;
}

void Value::set_type(Type type) {
	this->type = type;
}

void Value::set_curr_type(TypeDefinition curr_type) {
	//this->curr_type = curr_type;
}

void Value::set_arr_type(Type arr_type) {
	this->array_type = arr_type;
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

void Value::copy_array(cp_array arr) {
	this->arr = cp_array();
	for (auto ca : arr) {
		auto val = new Value(ca);
		this->arr.emplace_back(val);
	}
}

void Value::copy_from(Value* value) {
	type = value->type;
	//curr_type = value->curr_type;
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
}

bool Value::equals_array(cp_array arr) {
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

bool Value::equals(Value* value) {
	return type == value->type &&
		//curr_type == value->curr_type &&
		type_name == value->type_name &&
		type_name_space == value->type_name_space &&
		array_type == value->array_type &&
		dim == value->dim && // TODO: check if it will handle equality
		b == value->b &&
		i == value->i &&
		f == value->f &&
		c == value->c &&
		s == value->s &&
		equals_array(value->arr) &&
		str == value->str;
}

Variable::Variable(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
	const std::string& type_name, const std::string& type_name_space, Value* value)
	: TypeDefinition(def_type(type), def_array_type(array_type, dim),
		std::move(dim), type_name, type_name_space),
	value(value) {}

Variable::Variable()
	: TypeDefinition(Type::T_UNDEFINED, Type::T_UNDEFINED, std::vector<ASTExprNode*>(), "", "") {};

Variable::Variable(Value* v)
	: TypeDefinition(def_type(v->type), def_array_type(v->array_type, dim),
		v->dim, v ->type_name, v->type_name_space),
	value(v) {};

Variable::~Variable() = default;

void Variable::set(Value* val) {
	value = new Value(val);
	//if (is_any(type) || is_void(type) || is_undefined(type)) {
	//	type = val->type;
	//	type_name = val->type_name;
	//	type_name_space = val->type_name_space;
	//	array_type = val->array_type;
	//	dim = val->dim;
	//}
	value->ref = this;
}

Type Variable::def_type(Type type) {
	return is_void(type) || is_undefined(type) ? Type::T_ANY : type;
}

Type Variable::def_array_type(Type array_type, const std::vector<ASTExprNode*>& dim) {
	return is_void(array_type) || is_undefined(array_type) && dim.size() > 0 ? Type::T_ANY : array_type;
}

