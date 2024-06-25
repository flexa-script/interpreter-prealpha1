#include "visitor.hpp"

using namespace parser;

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

Value::Value()
	: type(Type::T_UNDEFINED),
	curr_type(Type::T_UNDEFINED),
	arr_type(Type::T_UNDEFINED) {};

Value::Value(cp_bool rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_int rawv){
	tset(rawv);
	def_ref();
};

Value::Value(cp_float rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_char rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_string rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_array rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_struct* rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(cp_function rawv) {
	tset(rawv);
	def_ref();
};

Value::Value(Type type)
	: type(type), curr_type(type),
	arr_type(Type::T_UNDEFINED) {
	def_ref();
};

Value::Value(Type type, Type arr_type)
	: type(type), curr_type(type), arr_type(arr_type) {
	def_ref();
};

Value::Value(Value* v)
	: b(v->b), i(v->i), f(v->f), c(v->c), s(v->s), str(v->str), fun(v->fun),
	type(v->type), curr_type(v->curr_type), arr_type(v->arr_type), ref(v->ref) {
	copy_array(v->arr);
};

Value::~Value() = default;

void Value::set(cp_bool b) {
	this->b = b;
	set_curr_type(Type::T_BOOL);
}

void Value::set(cp_int i) {
	this->i = i;
	set_curr_type(Type::T_INT);
}

void Value::set(cp_float f) {
	this->f = f;
	set_curr_type(Type::T_FLOAT);
}

void Value::set(cp_char c) {
	this->c = c;
	set_curr_type(Type::T_CHAR);
}

void Value::set(cp_string s) {
	this->s = s;
	set_curr_type(Type::T_STRING);
}

void Value::set(cp_array arr) {
	copy_array(arr);
	set_curr_type(Type::T_ARRAY);
}

void Value::set(cp_struct* str) {
	this->str = str;
	set_curr_type(Type::T_STRUCT);
}

void Value::set(cp_function fun) {
	this->fun = fun;
	set_curr_type(Type::T_FUNCTION);
}

void Value::tset(cp_bool b) {
	set_type(Type::T_BOOL);
	set(b);
}

void Value::tset(cp_int i) {
	set_type(Type::T_INT);
	set(i);
}

void Value::tset(cp_float f) {
	set_type(Type::T_FLOAT);
	set(f);
}

void Value::tset(cp_char c) {
	set_type(Type::T_CHAR);
	set(c);
}

void Value::tset(cp_string s) {
	set_type(Type::T_STRING);
	set(s);
}

void Value::tset(cp_array arr) {
	set_type(Type::T_ARRAY);
	set(arr);
}

void Value::tset(cp_struct* str) {
	set_type(Type::T_STRUCT);
	set(str);
}

void Value::tset(cp_function fun) {
	set_type(Type::T_FUNCTION);
	set(fun);
}

void Value::def_ref() {
	ref = is_struct(type) || is_struct(arr_type);
}

void Value::set_type(Type type) {
	this->type = type;
}

void Value::set_curr_type(Type curr_type) {
	this->curr_type = curr_type;
}

void Value::set_arr_type(Type arr_type) {
	this->arr_type = arr_type;
}

void Value::set_null() {
	set_empty(Type::T_VOID);
}

void Value::set_undefined() {
	set_empty(Type::T_UNDEFINED);
}

void Value::set_empty(Type empty_type) {
	auto aux_type = type;
	copy_from(new Value(empty_type));
	type = aux_type;
}

bool Value::has_value() {
	return curr_type != Type::T_UNDEFINED && curr_type != Type::T_VOID;
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
	curr_type = value->curr_type;
	arr_type = value->arr_type;
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
		curr_type == value->curr_type &&
		arr_type == value->arr_type &&
		b == value->b &&
		i == value->i &&
		f == value->f &&
		c == value->c &&
		s == value->s &&
		equals_array(value->arr) &&
		str == value->str;
}
