#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <vector>
#include <map>
#include <any>
#include <stdexcept>


class Value;

namespace parser {
	enum class Type {
		T_UNDEF, T_VOID, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_ARRAY, T_STRUCT, T_ANY
	};
	std::string type_str(Type);
	bool match_type(parser::Type, parser::Type);
	bool is_undefined(parser::Type);
	bool is_void(parser::Type);
	bool is_bool(parser::Type);
	bool is_int(parser::Type);
	bool is_float(parser::Type);
	bool is_char(parser::Type);
	bool is_string(parser::Type);
	bool is_any(parser::Type);
	bool is_array(parser::Type);
	bool is_struct(parser::Type);

	class ASTProgramNode;

	class ASTUsingNode;
	class ASTDeclarationNode;
	class ASTAssignmentNode;
	class ASTPrintNode;
	class ASTReadNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTContinueNode;
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
	class ASTExprNode;
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

typedef bool cp_bool;
typedef int64_t cp_int;
typedef long double cp_float;
typedef char cp_char;
typedef std::string cp_string; 
typedef std::vector<Value*> cp_array;
typedef std::map<std::string, Value*> cp_struct_values;
typedef std::pair<std::string, cp_struct_values> cp_struct;

class Value {
public:
	Value(parser::Type);
	
	parser::Type type;
	parser::Type curr_type;
	parser::Type arr_type;

	cp_bool b;
	cp_int i;
	cp_float f;
	cp_char c;
	cp_string s;
	cp_array arr;
	cp_struct* str;

	void set(cp_bool);
	void set(cp_int);
	void set(cp_float);
	void set(cp_char);
	void set(cp_string);
	void set(cp_array);
	void set(cp_struct*);
	void set_null();
	void set_undefined();

	void set_type(parser::Type);
	void set_curr_type(parser::Type);
	void set_arr_type(parser::Type);

	bool has_value();

	void copy_from(Value*);
};

namespace visitor {
	class Visitor {
	public:
		Visitor(std::map<std::string, parser::ASTProgramNode*> programs, parser::ASTProgramNode* main_program, std::string current_name)
			: programs(programs), main_program(main_program), current_program(main_program), current_name(current_name) { };

		std::map<std::string, parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* main_program;
		parser::ASTProgramNode* current_program;
		std::string current_name;

		virtual std::string get_namespace(std::string = "") = 0;
		virtual std::string get_namespace(parser::ASTProgramNode*, std::string = "") = 0;

		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTPrintNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTContinueNode*) = 0;
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

		virtual unsigned int hash(parser::ASTExprNode*) = 0;
		virtual unsigned int hash(parser::ASTIdentifierNode*) = 0;
		virtual unsigned int hash(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual unsigned int hash(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual unsigned int hash(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual unsigned int hash(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual unsigned int hash(parser::ASTLiteralNode<cp_string>*) = 0;
	};
}

#endif // VISITOR_HPP
