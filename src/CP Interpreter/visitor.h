#ifndef VISITOR_H
#define VISITOR_H

#include <string>
#include <vector>
#include <any>

#if defined(_WIN32) || defined(WIN32)
typedef __int64 __int64_t;
#endif

struct Value;

namespace parser {
	enum class TYPE {
		T_ND, T_VOID, T_NULL, T_ANY, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_STRUCT, T_ARRAY
	};
}

typedef bool                  cp_bool;
typedef __int64_t             cp_int;
typedef long double           cp_float;
typedef char                  cp_char;
typedef std::string           cp_string;
typedef std::any              cp_any;
typedef std::vector<Value*>*  cp_array;

typedef std::pair<std::string, Value*>                 cp_struct_value;
typedef std::vector<cp_struct_value>*                  cp_struct_value_vector;
typedef std::pair<std::string, cp_struct_value_vector> cp_struct;

typedef struct Value {
	Value() : b(0), i(0), f(0), c(0), s(""), a(std::any()), str(cp_struct()), arr(cp_array()), currentType(parser::TYPE::T_ND) {};
	parser::TYPE currentType;
	cp_bool b;
	cp_int i;
	cp_float f;
	cp_char c;
	cp_string s;
	cp_any a;
	cp_array arr;
	cp_struct str;
	void set(cp_bool b) {
		this->b = b;
		currentType = parser::TYPE::T_BOOL;
	}
	void set(cp_int i) {
		this->i = i;
		currentType = parser::TYPE::T_INT;
	}
	void set(cp_float f) {
		this->f = f;
		currentType = parser::TYPE::T_FLOAT;
	}
	void set(cp_char c) {
		this->c = c;
		currentType = parser::TYPE::T_CHAR;
	}
	void set(cp_string s) {
		this->s = s;
		currentType = parser::TYPE::T_STRING;
	}
	void set(cp_any a) {
		this->a = a;
		currentType = parser::TYPE::T_ANY;
	}
	void set(cp_array arr) {
		this->arr = arr;
		currentType = parser::TYPE::T_ARRAY;
	}
	void set(cp_struct str) {
		this->str = str;
		currentType = parser::TYPE::T_STRUCT;
	}
	void setNull() {
		currentType = parser::TYPE::T_NULL;
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
	class ASTIfNode;
	class ASTWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;

	template <typename T> class ASTLiteralNode;
	class ASTBinaryExprNode;
	class ASTIdentifierNode;
	class ASTUnaryExprNode;
	class ASTExprFunctionCallNode;
	class ASTTypeParseNode;
	class ASTExprReadNode;
	class ASTThisNode;
}

namespace visitor {
	class Visitor {
	public:
		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTPrintNode*) = 0;
		virtual void visit(parser::ASTReadNode*) = 0;
		virtual void visit(parser::ASTFunctionCallNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTIfNode*) = 0;
		virtual void visit(parser::ASTWhileNode*) = 0;
		virtual void visit(parser::ASTFunctionDefinitionNode*) = 0;
		virtual void visit(parser::ASTStructDefinitionNode*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_string>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_any>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_array>*) = 0;
		virtual void visit(parser::ASTBinaryExprNode*) = 0;
		virtual void visit(parser::ASTIdentifierNode*) = 0;
		virtual void visit(parser::ASTUnaryExprNode*) = 0;
		virtual void visit(parser::ASTExprFunctionCallNode*) = 0;
		virtual void visit(parser::ASTTypeParseNode*) = 0;
		virtual void visit(parser::ASTExprReadNode*) = 0;
		virtual void visit(parser::ASTThisNode*) = 0;
	};
}

#endif //VISITOR_H
