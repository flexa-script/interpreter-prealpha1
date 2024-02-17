#include "visitor.h"


namespace parser {
	std::string typeStr(TYPE t) {
		switch (t) {
		case parser::TYPE::T_ND:
			return "none";
		case parser::TYPE::T_VOID:
			return "void";
		case parser::TYPE::T_NULL:
			return "null";
		case parser::TYPE::T_BOOL:
			return "bool";
		case parser::TYPE::T_INT:
			return "int";
		case parser::TYPE::T_FLOAT:
			return "float";
		case parser::TYPE::T_CHAR:
			return "char";
		case parser::TYPE::T_STRING:
			return "string";
		case parser::TYPE::T_ANY:
			return "any";
		case parser::TYPE::T_ARRAY:
			return "array";
		case parser::TYPE::T_STRUCT:
			return "struct";
		default:
			throw std::runtime_error("invalid type encountered");
		}
	}
}
