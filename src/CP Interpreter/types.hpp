#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

namespace parser {
	enum class Type {
		T_UNDEFINED, T_VOID, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING, T_ARRAY, T_STRUCT, T_ANY, T_FUNCTION
	};
	
	std::string type_str(Type);

	bool match_type(Type, Type);
	bool is_undefined(Type);
	bool is_void(Type);
	bool is_bool(Type);
	bool is_int(Type);
	bool is_float(Type);
	bool is_char(Type);
	bool is_string(Type);
	bool is_any(Type);
	bool is_array(Type);
	bool is_struct(Type);
	bool is_function(Type);
	bool is_text(Type);
	bool is_numeric(Type);
	bool is_collection(Type);
	bool is_iterable(Type);

	class ASTExprNode;
}

using namespace parser;

namespace visitor {
	typedef std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> dim_eval_func_t;
};

using namespace visitor;

class Value;

typedef bool cp_bool;
typedef int64_t cp_int;
typedef long double cp_float;
typedef char cp_char;
typedef std::string cp_string;
typedef std::vector<Value*> cp_array;
typedef std::map<std::string, Value*> cp_struct_values;
typedef std::tuple<std::string, std::string, cp_struct_values> cp_struct;
typedef std::pair<std::string, std::string> cp_function;

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
	Type type;
	std::string type_name;
	std::string type_name_space;
	Type array_type;
	std::vector<ASTExprNode*> dim;
	bool use_ref;

	TypeDefinition(Type type, Type array_type,
		const std::vector<ASTExprNode*>& dim,
		const std::string& type_name, const std::string& type_name_space);

	TypeDefinition();

	static TypeDefinition get_basic(Type type);
	static TypeDefinition get_array(Type array_type,
		std::vector<ASTExprNode*>&& dim = std::vector<ASTExprNode*>());
	static TypeDefinition get_struct(const std::string& type_name,
		const std::string& type_name_space);

	static bool is_any_or_match_type(TypeDefinition* lvtype, TypeDefinition ltype, TypeDefinition* rvtype, TypeDefinition rtype,
		dim_eval_func_t evaluate_access_vector, bool strict = false);
	static bool match_type(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector, bool strict = false);
	static bool match_type_bool(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_int(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_float(TypeDefinition ltype, TypeDefinition rtype, bool strict = false);
	static bool match_type_char(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_string(TypeDefinition ltype, TypeDefinition rtype, bool strict = false);
	static bool match_type_array(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector);
	static bool match_type_struct(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_function(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_array_dim(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector);

	void reset_ref();
};

class VariableDefinition : public TypeDefinition, public CodePosition {
public:
	std::string identifier;
	ASTExprNode* default_value;
	bool is_rest;

	VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
		const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
		ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col);

	VariableDefinition();

	static VariableDefinition get_basic(const std::string& identifier, Type type,
		ASTExprNode* default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

	static VariableDefinition get_array(const std::string& identifier, Type array_type,
		std::vector<ASTExprNode*>&& dim = std::vector<ASTExprNode*>(), ASTExprNode* default_value = nullptr,
		bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

	static VariableDefinition get_struct(const std::string& identifier,
		const std::string& type_name, const std::string& type_name_space, ASTExprNode* default_value = nullptr,
		bool is_rest = false, unsigned int row = 0, unsigned int col = 0);
};

class FunctionDefinition : public TypeDefinition, public CodePosition {
public:
	std::string identifier;
	std::vector<TypeDefinition> signature;
	std::vector<VariableDefinition> parameters;

	FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
		const std::string& type_name_space, Type array_type, const std::vector<ASTExprNode*>& dim,
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
	SemanticValue(Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
		const std::string& type_name, const std::string& type_name_space, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	SemanticValue(TypeDefinition type_definition, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	SemanticValue(VariableDefinition variable_definition, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	// simplified constructor
	SemanticValue(Type type, unsigned int row, unsigned int col);

	SemanticValue();

	void copy_from(SemanticValue* value);
};

class SemanticVariable : public TypeDefinition, public CodePosition {
public:
	std::string identifier;
	SemanticValue* value;
	bool is_const;

	SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
		const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col);

	SemanticVariable();

	Type def_type(Type type);
	Type def_array_type(Type array_type, const std::vector<ASTExprNode*>& dim);
};

class Value : public TypeDefinition {
public:
	cp_bool b = false;
	cp_int i = 0;
	cp_float f = 0;
	cp_char c = '\0';
	cp_string s;
	cp_array arr;
	cp_struct* str = nullptr;
	cp_function fun;
	Variable* ref = nullptr;

	Value(Type type, Type array_type, std::vector<ASTExprNode*> dim,
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
	Value(Type type, Type arr_type, std::vector<ASTExprNode*> dim);
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

	void set_type(Type type);
	void set_arr_type(Type arr_type);

	bool has_value();

	void copy_array(cp_array arr);
	void copy_struct(cp_struct* str);
	void copy_from(Value* value);

	bool equals_array(cp_array arr);
	bool equals(Value* value);
};

class Variable : public TypeDefinition {
public:
	Value* value;

	Variable(Type type, Type array_type, std::vector<ASTExprNode*> dim,
		const std::string& type_name, const std::string& type_name_space, Value* value);
	Variable(Value* value);
	Variable();
	~Variable();

	void set_value(Value* value);
	Value* get_value();

	Type def_type(Type type);
	Type def_array_type(Type array_type, const std::vector<ASTExprNode*>& dim);
};

#endif // !TYPES_HPP
