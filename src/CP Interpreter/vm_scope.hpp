#ifndef INTERPRETER_SCOPE_HPP
#define INTERPRETER_SCOPE_HPP

#include <map>
#include <stack>

#include "visitor.hpp"
#include "ast.hpp"

using namespace visitor;
using namespace parser;

namespace visitor {
	class VirtualMachineScope {
	public:
		bool has_string_access = false;

	private:
		std::string name;
		std::unordered_map<std::string, StructureDefinition> struct_symbol_table;
		std::unordered_multimap<std::string, FunctionDefinition> function_symbol_table;
		std::unordered_map<std::string, std::shared_ptr<RuntimeVariable>> variable_symbol_table;

	public:
		VirtualMachineScope();
		VirtualMachineScope(const std::string& name);
		~VirtualMachineScope() = default;

		bool already_declared_structure_definition(const std::string& identifier);
		bool already_declared_variable(const std::string& identifier);
		bool already_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
			std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict = true);
		bool already_declared_function_name(const std::string& identifier);

		std::shared_ptr<RuntimeVariable> declare_variable(const std::string&, std::shared_ptr<RuntimeVariable>);
		void declare_function(const std::string& identifier, std::vector<VariableDefinition> variables, ASTBlockNode* block, TypeDefinition type);
		void declare_structure_definition(const std::string&, std::map<std::string, VariableDefinition>, unsigned int, unsigned int);

		StructureDefinition find_declared_structure_definition(const std::string& identifier);
		std::shared_ptr<RuntimeVariable> find_declared_variable(const std::string& identifier);
		FunctionDefinition& find_declared_function(const std::string& identifier, const std::vector<TypeDefinition>* signature,
			std::function<std::vector<unsigned int>(const std::vector<ASTExprNode*>&)> evaluate_access_vector, bool strict = true);
		std::pair<std::unordered_multimap<std::string, FunctionDefinition>::iterator,
			std::unordered_multimap<std::string, FunctionDefinition>::iterator> find_declared_functions(const std::string& identifier);

		const std::string& get_name();
		void set_name(const std::string& name);
	};
}

#endif // !INTERPRETER_SCOPE_HPP
