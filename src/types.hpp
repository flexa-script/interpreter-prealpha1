#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

#include "gcobject.hpp"

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
	class ASTBlockNode;
}

using namespace parser;

namespace visitor {
	typedef std::function<std::vector<unsigned int>(const std::vector<std::shared_ptr<ASTExprNode>>&)> dim_eval_func_t;
};

using namespace visitor;

class RuntimeValue;

typedef bool cp_bool;
typedef int64_t cp_int;
typedef long double cp_float;
typedef char cp_char;
typedef std::string cp_string;
typedef std::vector<RuntimeValue*> cp_array;
typedef std::unordered_map<std::string, RuntimeValue*> cp_struct;
typedef std::pair<std::string, std::string> cp_function;

class SemanticVariable;
class RuntimeVariable;

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
	std::vector<std::shared_ptr<ASTExprNode>> dim;
	bool use_ref;

	TypeDefinition(Type type, Type array_type,
		const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::string& type_name, const std::string& type_name_space);

	TypeDefinition(Type type);

	TypeDefinition();

	static TypeDefinition get_basic(Type type);
	static TypeDefinition get_array(Type array_type,
		const std::vector<std::shared_ptr<ASTExprNode>>& dim = std::vector<std::shared_ptr<ASTExprNode>>());
	static TypeDefinition get_struct(const std::string& type_name,
		const std::string& type_name_space);

	static bool is_any_or_match_type(TypeDefinition ltype, TypeDefinition rtype,
		dim_eval_func_t evaluate_access_vector, bool strict = false, bool strict_array = false);
	static bool match_type(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector, bool strict = false, bool strict_array = false);
	static bool match_type_bool(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_int(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_float(TypeDefinition ltype, TypeDefinition rtype, bool strict = false);
	static bool match_type_char(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_string(TypeDefinition ltype, TypeDefinition rtype, bool strict = false);
	static bool match_type_array(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector, bool strict = false, bool strict_array = false);
	static bool match_type_struct(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_type_function(TypeDefinition ltype, TypeDefinition rtype);
	static bool match_array_dim(TypeDefinition ltype, TypeDefinition rtype, dim_eval_func_t evaluate_access_vector);

	virtual void reset_ref();
};

class VariableDefinition : public TypeDefinition, public CodePosition {
public:
	std::string identifier;
	std::shared_ptr<ASTExprNode> default_value;
	bool is_rest;

	VariableDefinition();

	VariableDefinition(const std::string& identifier, Type type,
		const std::string& type_name, const std::string& type_name_space,
		Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		std::shared_ptr<ASTExprNode> default_value, bool is_rest, unsigned int row, unsigned int col);

	VariableDefinition(const std::string& identifier, Type type,
		std::shared_ptr<ASTExprNode> default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

	VariableDefinition(const std::string& identifier,
		Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::string& type_name = "", const std::string& type_name_space = "",
		std::shared_ptr<ASTExprNode> default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

	VariableDefinition(const std::string& identifier, const std::string& type_name, const std::string& type_name_space,
		std::shared_ptr<ASTExprNode> default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);
};

class UnpackedVariableDefinition : public TypeDefinition {
public:
	std::vector<VariableDefinition> variables;
	std::shared_ptr<ASTExprNode> assign_value;

	UnpackedVariableDefinition(Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim, const std::string& type_name,
		const std::string& type_name_space, const std::vector<VariableDefinition>& variables);

	UnpackedVariableDefinition(TypeDefinition type_definition, const std::vector<VariableDefinition>& variables);
};

class FunctionDefinition : public TypeDefinition, public CodePosition {
public:
	std::string identifier;
	std::vector<TypeDefinition*> parameters;
	size_t pointer;
	std::shared_ptr<ASTBlockNode> block;
	bool is_var = false;

	FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
		const std::string& type_name_space, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::vector<TypeDefinition*>& parameters,
		std::shared_ptr<ASTBlockNode> block, unsigned int row, unsigned int col);

	FunctionDefinition(const std::string& identifier, Type type,
		const std::vector<TypeDefinition*>& parameters);

	FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
		const std::string& type_name_space, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim);

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

	StructureDefinition(const std::string& identifier);

	StructureDefinition();
};

class Variable : public TypeDefinition {
public:
	std::string identifier;

	Variable(const std::string& identifier, Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::string& type_name, const std::string& type_name_space);

	Variable(TypeDefinition value);

	Variable();

	virtual ~Variable() = default;
};

class Value : public TypeDefinition {
public:
	std::weak_ptr<Variable> ref;

	Value(Type type, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
		const std::string& type_name, const std::string& type_name_space);
	Value(TypeDefinition type);
	Value();
	virtual ~Value() = default;
};

class SemanticValue : public Value, public CodePosition {
public:
	long long hash;
	bool is_const;
	bool is_sub;

	// complete constructor
	SemanticValue(Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::string& type_name, const std::string& type_name_space, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	SemanticValue(Type type, long long hash, bool is_const, unsigned int row, unsigned int col);

	SemanticValue(TypeDefinition type_definition, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	SemanticValue(VariableDefinition variable_definition, long long hash,
		bool is_const, unsigned int row, unsigned int col);

	// simplified constructor
	SemanticValue(Type type, unsigned int row, unsigned int col);

	SemanticValue();

	void copy_from(const SemanticValue& value);
};

class SemanticVariable : public Variable, public CodePosition, public std::enable_shared_from_this<SemanticVariable> {
public:
	std::shared_ptr<SemanticValue> value;
	bool is_const;

	SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim,
		const std::string& type_name, const std::string& type_name_space, bool is_const, unsigned int row, unsigned int col);

	SemanticVariable(const std::string& identifier, Type type, bool is_const, unsigned int row, unsigned int col);

	SemanticVariable();

	void set_value(std::shared_ptr<SemanticValue> value);
	std::shared_ptr<SemanticValue> get_value();

	Type def_type(Type type);
	Type def_array_type(Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim);

	void reset_ref() override;
};

class RuntimeValue : public Value, public GCObject {
private:
	cp_bool* b = nullptr;
	cp_int* i = nullptr;
	cp_float* f = nullptr;
	cp_char* c = nullptr;
	cp_string* s = nullptr;
	cp_array* arr = nullptr;
	cp_struct* str = nullptr;
	cp_function* fun = nullptr;

public:
	RuntimeValue(Type type, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
		const std::string& type_name, const std::string& type_name_space,
		unsigned int row, unsigned int col);
	RuntimeValue(cp_bool);
	RuntimeValue(cp_int);
	RuntimeValue(cp_float);
	RuntimeValue(cp_char);
	RuntimeValue(cp_string);
	RuntimeValue(cp_array);
	RuntimeValue(cp_array, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name = "", std::string type_name_space = "");
	RuntimeValue(cp_struct, std::string type_name, std::string type_name_space);
	RuntimeValue(cp_function);
	RuntimeValue(Type type);
	RuntimeValue(Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name = "", std::string type_name_space = "");
	RuntimeValue(std::string type_name, std::string type_name_space);
	RuntimeValue(RuntimeValue*);
	RuntimeValue(TypeDefinition type);
	RuntimeValue();
	~RuntimeValue();

	void set(cp_bool);
	void set(cp_int);
	void set(cp_float);
	void set(cp_char);
	void set(cp_string);
	void set(cp_array);
	void set(cp_array, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim, std::string type_name = "", std::string type_name_space = "");
	void set(cp_struct, std::string type_name, std::string type_name_space);
	void set(cp_function);
	void set_sub(std::string identifier, RuntimeValue* sub_value);
	void set_sub(size_t index, RuntimeValue* sub_value);

	cp_bool get_b() const;
	cp_int get_i() const;
	cp_float get_f() const;
	cp_char get_c() const;
	cp_string get_s() const;
	cp_array get_arr() const;
	cp_struct get_str() const;
	cp_function get_fun() const;

	cp_bool* get_raw_b();
	cp_int* get_raw_i();
	cp_float* get_raw_f();
	cp_char* get_raw_c();
	cp_string* get_raw_s();
	cp_array* get_raw_arr();
	cp_struct* get_raw_str();
	cp_function* get_raw_fun();

	void set_null();
	void set_undefined();

	void set_type(Type type);
	void set_arr_type(Type arr_type);

	bool has_value();

	long double value_hash() const;

	void copy_array(cp_array arr);
	void copy_from(RuntimeValue* value);

	virtual std::vector<GCObject*> get_references() override;

private:
	void unset();
};

class RuntimeVariable : public Variable, public GCObject, public std::enable_shared_from_this<RuntimeVariable> {
public:
	RuntimeValue* value;

	RuntimeVariable(const std::string& identifier, Type type, Type array_type, std::vector<std::shared_ptr<ASTExprNode>> dim,
		const std::string& type_name, const std::string& type_name_space);
	RuntimeVariable(const std::string& identifier, TypeDefinition value);
	RuntimeVariable();
	~RuntimeVariable();

	void set_value(RuntimeValue* value);
	RuntimeValue* get_value();

	Type def_type(Type type);
	Type def_array_type(Type array_type, const std::vector<std::shared_ptr<ASTExprNode>>& dim);

	void reset_ref() override;

	virtual std::vector<GCObject*> get_references() override;
};

#endif // !TYPES_HPP
