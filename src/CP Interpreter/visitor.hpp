#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

namespace visitor {
	class Value;
}

namespace parser {
	enum class Type {
		T_UNDEFINED, T_VOID, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_ARRAY, T_STRUCT, T_ANY, T_FUNCTION
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
	bool is_function(parser::Type);
	bool is_text(parser::Type);
	bool is_numeric(parser::Type);
	bool is_collection(parser::Type);
	bool is_iterable(parser::Type);

	class ASTProgramNode;

	class ASTUsingNode;
	class ASTAsNamespaceNode;
	class ASTDeclarationNode;
	class ASTAssignmentNode;
	class ASTFunctionCallNode;
	class ASTReturnNode;
	class ASTBlockNode;
	class ASTContinueNode;
	class ASTBreakNode;
	class ASTSwitchNode;
	class ASTExitNode;
	class ASTEnumNode;
	class ASTElseIfNode;
	class ASTIfNode;
	class ASTTryCatchNode;
	class ASTThrowNode;
	class ASTReticencesNode;
	class ASTForNode;
	class ASTForEachNode;
	class ASTWhileNode;
	class ASTDoWhileNode;
	class ASTFunctionDefinitionNode;
	class ASTStructDefinitionNode;
	template <typename T> class ASTLiteralNode;
	class ASTFunctionExpression;
	class ASTExprNode;
	class ASTArrayConstructorNode;
	class ASTStructConstructorNode;
	class ASTBinaryExprNode;
	class ASTUnaryExprNode;
	class ASTIdentifierNode;
	class ASTTernaryNode;
	class ASTInNode;
	class ASTFunctionCallNode;
	class ASTTypeParseNode;
	class ASTNullNode;
	class ASTThisNode;
	class ASTTypingNode;
}

using namespace parser;

typedef bool cp_bool;
typedef int64_t cp_int;
typedef long double cp_float;
typedef char cp_char;
typedef std::string cp_string;
typedef std::vector<visitor::Value*> cp_array;
typedef std::map<std::string, visitor::Value*> cp_struct_values;
typedef std::tuple<std::string, std::string, cp_struct_values> cp_struct;
typedef std::pair<void*, std::string> cp_function;

extern std::string default_namespace;
extern std::vector<std::string> std_libs;
extern std::vector<std::string> built_in_libs;

namespace visitor {
	class SemanticVariable;
	class Variable;

	class CodePosition {
	public:
		unsigned int row;
		unsigned int col;

		CodePosition(unsigned int row = 0, unsigned int col = 0) : row(row), col(col) {};
	};

	class TypeDefinition {
	public:
		parser::Type type;
		std::string type_name;
		std::string type_name_space;
		parser::Type array_type;
		std::vector<parser::ASTExprNode*> dim;

		TypeDefinition(parser::Type type, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space);

		TypeDefinition();

		static TypeDefinition get_basic(parser::Type type);
		static TypeDefinition get_array(parser::Type type, parser::Type array_type, std::vector<parser::ASTExprNode*>&& dim = std::vector<parser::ASTExprNode*>());
		static TypeDefinition get_struct(parser::Type type, const std::string& type_name, const std::string& type_name_space);

		static bool is_any_or_match_type(TypeDefinition ltype, TypeDefinition rtype);
		static bool is_any_or_match_type(TypeDefinition vtype, TypeDefinition ltype, TypeDefinition rtype);
		static bool is_any_or_match_type(TypeDefinition lvtype, TypeDefinition ltype, TypeDefinition rvtype, TypeDefinition rtype);
		static bool match_type(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_bool(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_int(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_float(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_char(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_string(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_array(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_struct(TypeDefinition ltype, TypeDefinition rtype);
		static bool match_type_function(TypeDefinition ltype, TypeDefinition rtype);
	};

	class VariableDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		parser::ASTExprNode* default_value;
		bool is_rest;

		VariableDefinition(const std::string& identifier, parser::Type type, const std::string& type_name,
			const std::string& type_name_space, parser::Type array_type, std::vector<parser::ASTExprNode*>&& dim,
			parser::ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col);

		VariableDefinition();

		static VariableDefinition get_basic(const std::string& identifier, parser::Type type,
			parser::ASTExprNode* default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

		static VariableDefinition get_array(const std::string& identifier, parser::Type type, parser::Type array_type,
			std::vector<parser::ASTExprNode*>&& dim = std::vector<parser::ASTExprNode*>(), parser::ASTExprNode* default_value = nullptr,
			bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

		static VariableDefinition get_struct(const std::string& identifier, parser::Type type,
			const std::string& type_name, const std::string& type_name_space, parser::ASTExprNode* default_value = nullptr,
			bool is_rest = false, unsigned int row = 0, unsigned int col = 0);
	};

	class FunctionDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		std::vector<TypeDefinition> signature;
		std::vector<VariableDefinition> parameters;

		FunctionDefinition(const std::string& identifier, parser::Type type, const std::string& type_name,
			const std::string& type_name_space, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters,
			unsigned int row, unsigned int col);

		FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col);

		FunctionDefinition();

		void check_signature() const;
	};

	class StructureDefinition : public CodePosition {
	public:
		std::string identifier;
		std::map<std::string, VariableDefinition> variables;

		StructureDefinition(const std::string& identifier, const std::map<std::string, VariableDefinition>& variables,
			unsigned int row, unsigned int col);

		StructureDefinition();
	};

	class SemanticValue : public TypeDefinition, public CodePosition {
	public:
		SemanticVariable* ref;
		long long hash;
		bool is_const;
		bool is_sub;

		// complete constructor
		SemanticValue(parser::Type type, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space, long long hash,
			bool is_const, unsigned int row, unsigned int col);

		SemanticValue(TypeDefinition type_definition, long long hash,
			bool is_const, unsigned int row, unsigned int col);

		SemanticValue(VariableDefinition variable_definition, long long hash,
			bool is_const, unsigned int row, unsigned int col);

		// simplified constructor
		SemanticValue(parser::Type type, unsigned int row, unsigned int col);

		SemanticValue();

		void resetref();
		void copy_from(SemanticValue* value);
	};

	class SemanticVariable : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		SemanticValue* value;
		bool is_const;

		SemanticVariable(const std::string& identifier, parser::Type type, parser::Type array_type, const std::vector<parser::ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col);

		SemanticVariable();

		void copy_from(SemanticVariable* var);
	};

	class Value : public TypeDefinition {
	public:
		cp_bool b = false;
		cp_int i = 0;
		cp_float f = 0;
		cp_char c = '\0';
		cp_string s;
		cp_array arr;
		cp_struct* str;
		cp_function fun;
		Variable* ref;

		Value(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
			const std::string& type_name, const std::string& type_name_space,
			unsigned int row, unsigned int col);
		Value(cp_bool);
		Value(cp_int);
		Value(cp_float);
		Value(cp_char);
		Value(cp_string);
		Value(cp_array, Type);
		Value(cp_struct*);
		Value(cp_function);
		Value(Variable*);
		Value(Type type);
		Value(Type type, parser::Type arr_type, std::vector<ASTExprNode*> dim);
		Value(Value*);
		Value(TypeDefinition type);
		Value();
		~Value();

		void set(cp_bool);
		void set(cp_int);
		void set(cp_float);
		void set(cp_char);
		void set(cp_string);
		void set(cp_array, Type);
		void set(cp_struct*);
		void set(cp_function);

		void set_null();
		void set_undefined();
		void set_empty(parser::Type empty_type);

		void set_type(parser::Type type);
		void set_curr_type(TypeDefinition curr_type);
		void set_arr_type(parser::Type arr_type);
		void def_ref();

		bool has_value();

		void copy_array(cp_array arr);
		void copy_from(Value* value);

		bool equals_array(cp_array arr);
		bool equals(Value* value);
	};

	class Variable : public TypeDefinition {
	public:
		Value* value;
		bool ref = false;

		Variable(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*> dim,
			const std::string& type_name, const std::string& type_name_space, Value* value);
		Variable(Value* value);
		Variable();
		~Variable();

		void set(Value* value);
		void tset(Value* value);
	};

	class Visitor {
	public:
		std::map<std::string, parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* main_program;
		parser::ASTProgramNode* current_program;
		int curr_row;
		int curr_col;

		Visitor(const std::map<std::string, parser::ASTProgramNode*>& programs, parser::ASTProgramNode* main_program, const std::string& current_name)
			: programs(programs), main_program(main_program), current_program(main_program), curr_row(0), curr_col(0) { };

		virtual const std::string& get_namespace(const std::string& nmspace = "") const = 0;
		virtual const std::string& get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace = "") const = 0;

		virtual void set_curr_pos(unsigned int row, unsigned int col) = 0;
		virtual std::string msg_header() = 0;

		virtual void visit(parser::ASTProgramNode*) = 0;
		virtual void visit(parser::ASTUsingNode*) = 0;
		virtual void visit(parser::ASTAsNamespaceNode*) = 0;
		virtual void visit(parser::ASTDeclarationNode*) = 0;
		virtual void visit(parser::ASTAssignmentNode*) = 0;
		virtual void visit(parser::ASTReturnNode*) = 0;
		virtual void visit(parser::ASTBlockNode*) = 0;
		virtual void visit(parser::ASTContinueNode*) = 0;
		virtual void visit(parser::ASTBreakNode*) = 0;
		virtual void visit(parser::ASTExitNode*) = 0;
		virtual void visit(parser::ASTSwitchNode*) = 0;
		virtual void visit(parser::ASTElseIfNode*) = 0;
		virtual void visit(parser::ASTEnumNode*) = 0;
		virtual void visit(parser::ASTTryCatchNode*) = 0;
		virtual void visit(parser::ASTThrowNode*) = 0;
		virtual void visit(parser::ASTReticencesNode*) = 0;
		virtual void visit(parser::ASTIfNode*) = 0;
		virtual void visit(parser::ASTForNode*) = 0;
		virtual void visit(parser::ASTForEachNode*) = 0;
		virtual void visit(parser::ASTWhileNode*) = 0;
		virtual void visit(parser::ASTDoWhileNode*) = 0;
		virtual void visit(parser::ASTFunctionDefinitionNode*) = 0;
		virtual void visit(parser::ASTStructDefinitionNode*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual void visit(parser::ASTLiteralNode<cp_string>*) = 0;
		virtual void visit(parser::ASTFunctionExpression*) = 0;
		virtual void visit(parser::ASTArrayConstructorNode*) = 0;
		virtual void visit(parser::ASTStructConstructorNode*) = 0;
		virtual void visit(parser::ASTBinaryExprNode*) = 0;
		virtual void visit(parser::ASTUnaryExprNode*) = 0;
		virtual void visit(parser::ASTIdentifierNode*) = 0;
		virtual void visit(parser::ASTTernaryNode*) = 0;
		virtual void visit(parser::ASTInNode*) = 0;
		virtual void visit(parser::ASTFunctionCallNode*) = 0;
		virtual void visit(parser::ASTTypeParseNode*) = 0;
		virtual void visit(parser::ASTNullNode*) = 0;
		virtual void visit(parser::ASTThisNode*) = 0;
		virtual void visit(parser::ASTTypingNode*) = 0;

		virtual long long hash(parser::ASTExprNode*) = 0;
		virtual long long hash(parser::ASTIdentifierNode*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_bool>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_int>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_float>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_char>*) = 0;
		virtual long long hash(parser::ASTLiteralNode<cp_string>*) = 0;
	};
}

#endif // !VISITOR_HPP
