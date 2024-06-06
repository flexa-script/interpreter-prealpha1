#include "visitor.hpp"


namespace parser {
	std::string type_str(Type t) {
		switch (t) {
		case parser::Type::T_UNDEF:
			return "undefined";
		case parser::Type::T_VOID:
			return "void";
		case parser::Type::T_BOOL:
			return "bool";
		case parser::Type::T_INT:
			return "int";
		case parser::Type::T_FLOAT:
			return "float";
		case parser::Type::T_CHAR:
			return "char";
		case parser::Type::T_STRING:
			return "string";
		case parser::Type::T_ANY:
			return "any";
		case parser::Type::T_ARRAY:
			return "array";
		case parser::Type::T_STRUCT:
			return "struct";
		default:
			throw std::runtime_error("invalid type encountered");
		}
	}

	bool match_type(parser::Type type1, parser::Type type2) {
		return type1 == type2;
	}

	bool is_undefined(parser::Type type) {
		return match_type(type, parser::Type::T_UNDEF);
	}

	bool is_void(parser::Type type) {
		return match_type(type, parser::Type::T_VOID);
	}

	bool is_bool(parser::Type type) {
		return match_type(type, parser::Type::T_BOOL);
	}

	bool is_int(parser::Type type) {
		return match_type(type, parser::Type::T_INT);
	}

	bool is_float(parser::Type type) {
		return match_type(type, parser::Type::T_FLOAT);
	}

	bool is_char(parser::Type type) {
		return match_type(type, parser::Type::T_CHAR);
	}

	bool is_string(parser::Type type) {
		return match_type(type, parser::Type::T_STRING);
	}

	bool is_any(parser::Type type) {
		return match_type(type, parser::Type::T_ANY);
	}

	bool is_array(parser::Type type) {
		return match_type(type, parser::Type::T_ARRAY);
	}

	bool is_struct(parser::Type type) {
		return match_type(type, parser::Type::T_STRUCT);
	}
}

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
	"cp.core.console"
};

Value::Value(parser::Type type)
	: b(0), i(0), f(0), c(0), s(""), str(new cp_struct()), arr(cp_array()), type(type), curr_type(type), arr_type(type) {};

Value::Value(Value* value)
	: b(value->b), i(value->i), f(value->f), c(value->c), s(value->s), str(value->str),
	type(value->type), curr_type(value->curr_type), arr_type(value->arr_type) {
	copy_array(value->arr);
};

void Value::set(cp_bool b) {
	this->b = b;
	set_curr_type(parser::Type::T_BOOL);
}

void Value::set(cp_int i) {
	this->i = i;
	set_curr_type(parser::Type::T_INT);
}

void Value::set(cp_float f) {
	this->f = f;
	set_curr_type(parser::Type::T_FLOAT);
}

void Value::set(cp_char c) {
	this->c = c;
	set_curr_type(parser::Type::T_CHAR);
}

void Value::set(cp_string s) {
	this->s = s;
	set_curr_type(parser::Type::T_STRING);
}

void Value::set(cp_array arr) {
	copy_array(arr);
	set_curr_type(parser::Type::T_ARRAY);
}

void Value::set(cp_struct* str) {
	this->str = str;
	set_curr_type(parser::Type::T_STRUCT);
}

void Value::set_type(parser::Type type) {
	this->type = type;
}

void Value::set_curr_type(parser::Type curr_type) {
	this->curr_type = curr_type;
}

void Value::set_arr_type(parser::Type arr_type) {
	this->arr_type = arr_type;
}

void Value::set_null() {
	set_curr_type(parser::Type::T_VOID);
}

void Value::set_undefined() {
	set_curr_type(parser::Type::T_UNDEF);
}

bool Value::has_value() {
	return curr_type != parser::Type::T_UNDEF && curr_type != parser::Type::T_VOID;
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
