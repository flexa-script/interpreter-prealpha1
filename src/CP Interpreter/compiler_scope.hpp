#ifndef SEMANTIC_SCOPE_HPP
#define SEMANTIC_SCOPE_HPP

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "types.hpp"
#include "ast.hpp"

using namespace parser;

namespace visitor {

	typedef struct {
		cp_int id;
		std::string identifier;
		std::vector<TypeDefinition> signature;
		//std::vector<VariableDefinition> parameters;
		//ASTBlockNode* block;
		bool is_var = false;
	} compiler_func_t;

	class CompilerScope {
	private:
		std::map<std::string, std::pair<StructureDefinition, size_t>> structure_symbol_table;
		std::multimap<std::string, std::pair<FunctionDefinition, size_t>> function_symbol_table;
		std::map<std::string, std::pair<TypeDefinition, size_t>> variable_symbol_table;

	public:
		CompilerScope();
		~CompilerScope();

		bool already_declared_structure_definition(const std::string& identifier);
		bool already_declared_variable(const std::string& identifier);
		bool already_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
			std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict = true);

		size_t declare_structure_definition(const std::string& name, const std::map<std::string, VariableDefinition>& variables, unsigned int row, unsigned int col);

		size_t declare_variable(const std::string& identifier, TypeDefinition var);
		size_t declare_variable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space, unsigned int row, unsigned int col);

		size_t declare_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
			Type array_type, const std::vector<ASTExprNode*>& dim, const std::vector<TypeDefinition>& signature,
			const std::vector<VariableDefinition>& parameters, ASTBlockNode* block, unsigned int row, unsigned int col);

		size_t declare_variable_function(const std::string& identifier, unsigned int row, unsigned int col);

		size_t declare_basic_function(const std::string& identifier, Type type, std::vector<TypeDefinition> signature,
			std::vector<VariableDefinition> parameters, ASTBlockNode* block = nullptr, unsigned int row = 0, unsigned int col = 0);

		size_t declare_array_function(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
			const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, ASTBlockNode* block = nullptr,
			unsigned int row = 0, unsigned int col = 0);

		size_t declare_struct_function(const std::string& identifier, Type type, const std::string& type_name, const std::string& type_name_space,
			const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters, ASTBlockNode* block = nullptr,
			unsigned int row = 0, unsigned int col = 0);

		std::pair<StructureDefinition, size_t> find_declared_structure_definition(const std::string& identifier);
		std::pair<FunctionDefinition, size_t>& find_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
			std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict = true);
		std::pair<TypeDefinition, size_t> find_declared_variable(const std::string& identifier);

	};
}

#endif // !SEMANTIC_SCOPE_HPP
