#ifndef VISITOR_H
#define VISITOR_H

#include <string>
#include <vector>
#include <any>
#include <stdexcept>


struct Value;

namespace parser {
	enum class TYPE {
		T_ND, T_VOID, T_NULL, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_ANY, T_ARRAY, T_STRUCT
	};
	std::string typeStr(TYPE t);
}

typedef bool                  cp_bool;
typedef int64_t               cp_int;
typedef long double           cp_float;
typedef char                  cp_char;
typedef std::string           cp_string;
typedef std::vector<Value*>   cp_array;

typedef std::pair<std::string, Value*>           cp_struct_value;
typedef std::vector<cp_struct_value>             cp_struct_values;
typedef std::pair<std::string, cp_struct_values> cp_struct;

typedef struct Value {
	Value(parser::TYPE type) : b(0), i(0), f(0), c(0), s(""), str(cp_struct()), arr(cp_array()), hasValue(false), type(type), currType(type) {};

	bool hasValue;
	
	parser::TYPE type;
	parser::TYPE currType;
	parser::TYPE arrType;
	std::vector<int> dim;
	cp_bool b;
	cp_int i;
	cp_float f;
	cp_char c;
	cp_string s;
	cp_array arr;
	cp_struct str;

	void set(cp_bool b) {
		this->b = b;
		hasValue = true;
		setCurrType(parser::TYPE::T_BOOL);
	}
	void set(cp_int i) {
		this->i = i;
		hasValue = true;
		setCurrType(parser::TYPE::T_INT);
	}
	void set(cp_float f) {
		this->f = f;
		hasValue = true;
		setCurrType(parser::TYPE::T_FLOAT);
	}
	void set(cp_char c) {
		this->c = c;
		hasValue = true;
		setCurrType(parser::TYPE::T_CHAR);
	}
	void set(cp_string s) {
		this->s = s;
		hasValue = true;
		setCurrType(parser::TYPE::T_STRING);
	}
	void set(cp_array arr) {
		this->arr = arr;
		hasValue = true;
		setCurrType(parser::TYPE::T_ARRAY);
	}
	void set(cp_struct str) {
		this->str = str;
		hasValue = true;
		setCurrType(parser::TYPE::T_STRUCT);
	}
	void setType(parser::TYPE type) {
		this->type = type;
	}
	void setCurrType(parser::TYPE currType) {
		this->currType = currType;
	}
	void setNull() {
		hasValue = false;
	}
	void copyFrom(Value* value) {
		hasValue = value->hasValue;
		currType = value->currType;
		arrType = value->arrType;
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
} Value_t;

namespace parser {
	class ASTProgramNode;

	class ASTUsingNode;
	class ASTDeclarationNode;
	class ASTAssignmentNode;
	class ASTPrintNode;
	class ASTReadNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTBreakNode;
	class ASTSwitchNode;
	class ASTElseIfNode;
	class ASTIfNode;
	class ASTForNode;
	class ASTForEachNode;
	class ASTWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;
	template <typename T> class ASTLiteralNode;
	class ASTArrayConstructorNode;
	class ASTStructConstructorNode;
	class ASTBinaryExprNode;
	class ASTIdentifierNode;
	class ASTUnaryExprNode;
	class ASTFunctionCallNode;
	class ASTTypeParseNode;
	class ASTReadNode;
	class ASTNullNode;
	class ASTThisNode;
	class ASTTypeNode;
	class ASTLenNode;
	class ASTRoundNode;
}

namespace visitor {
	class Visitor {
	public:
		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTPrintNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTBreakNode*) = 0;
		virtual void visit(parser::ASTSwitchNode*) = 0;
		virtual void visit(parser::ASTElseIfNode*) = 0;
		virtual void visit(parser::ASTIfNode*) = 0;
		virtual void visit(parser::ASTForNode*) = 0;
		virtual void visit(parser::ASTForEachNode*) = 0;
		virtual void visit(parser::ASTWhileNode*) = 0;
		virtual void visit(parser::ASTFunctionDefinitionNode*) = 0;
		virtual void visit(parser::ASTStructDefinitionNode*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_string>*) = 0;
		virtual void visit(parser::ASTArrayConstructorNode*) = 0;
		virtual void visit(parser::ASTStructConstructorNode*) = 0;
		virtual void visit(parser::ASTBinaryExprNode*) = 0;
		virtual void visit(parser::ASTIdentifierNode*) = 0;
		virtual void visit(parser::ASTUnaryExprNode*) = 0;
		virtual void visit(parser::ASTFunctionCallNode*) = 0;
		virtual void visit(parser::ASTTypeParseNode*) = 0;
		virtual void visit(parser::ASTTypeNode*) = 0;
		virtual void visit(parser::ASTLenNode*) = 0;
		virtual void visit(parser::ASTRoundNode*) = 0;
		virtual void visit(parser::ASTReadNode*) = 0;
		virtual void visit(parser::ASTNullNode*) = 0;
		virtual void visit(parser::ASTThisNode*) = 0;
	};
}

#endif //VISITOR_H
