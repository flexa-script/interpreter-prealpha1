#include "visitor.hpp"


namespace parser {
	std::string type_str(Type t) {
		switch (t) {
		case parser::Type::T_ND:
			return "none";
		case parser::Type::T_VOID:
			return "void";
		case parser::Type::T_NULL:
			return "null";
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
}

Value::Value(parser::Type type)
	: b(0), i(0), f(0), c(0), s(""), str(cp_struct()), arr(cp_array()), has_value(false), type(type), curr_type(type), arr_type(type) {};

void Value::set(cp_bool b) {
	this->b = b;
	has_value = true;
	set_curr_type(parser::Type::T_BOOL);
}

void Value::set(cp_int i) {
	this->i = i;
	has_value = true;
	set_curr_type(parser::Type::T_INT);
}

void Value::set(cp_float f) {
	this->f = f;
	has_value = true;
	set_curr_type(parser::Type::T_FLOAT);
}

void Value::set(cp_char c) {
	this->c = c;
	has_value = true;
	set_curr_type(parser::Type::T_CHAR);
}

void Value::set(cp_string s) {
	this->s = s;
	has_value = true;
	set_curr_type(parser::Type::T_STRING);
}

void Value::set(cp_array arr) {
	this->arr = arr;
	has_value = true;
	set_curr_type(parser::Type::T_ARRAY);
}

void Value::set(cp_struct str) {
	this->str = str;
	has_value = true;
	set_curr_type(parser::Type::T_STRUCT);
}

void Value::set_type(parser::Type type) {
	this->type = type;
}

void Value::set_curr_type(parser::Type curr_type) {
	this->curr_type = curr_type;
}

void Value::set_null() {
	has_value = false;
}

void Value::copy_from(Value* value) {
	has_value = value->has_value;
	curr_type = value->curr_type;
	arr_type = value->arr_type;
	dim = value->dim;
	type = value->type;
	b = value->b;
	i = value->i;
	f = value->f;
	c = value->c;
	s = value->s;
	arr = value->arr;
	str = value->str;
}
