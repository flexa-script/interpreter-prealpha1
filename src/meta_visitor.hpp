#ifndef META_VISITOR_HPP
#define META_VISITOR_HPP

#include <string>
#include <vector>
#include <stack>
#include <map>

#include "scope.hpp"
#include "types.hpp"
#include "namespace_manager.hpp"

namespace parser {
	class ASTProgramNode;
	template <typename T> class ASTLiteralNode;
	class ASTFunctionExpression;
	class ASTIdentifierNode;
}

using namespace parser;

namespace visitor {

	class MetaVisitor: public NamespaceManager {
	public:
		std::unordered_map<std::string, std::vector<std::shared_ptr<visitor::Scope>>> scopes;
		std::stack<std::string> current_namespace;
		std::map<std::string, std::vector<std::string>> program_nmspaces;

		MetaVisitor() = default;
		virtual ~MetaVisitor() = default;

		void validates_reference_type_assignment(TypeDefinition owner, Value* value);

		std::shared_ptr<Variable> find_inner_most_variable(std::string& nmspace, const std::string& identifier);

		std::shared_ptr<Scope> get_inner_most_struct_definition_scope(std::string& nmspace, const std::string& identifier);
		std::shared_ptr<Scope> get_inner_most_function_scope(std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition*>* signature, dim_eval_func_t evaluate_access_vector_ptr, bool strict = true);
		std::shared_ptr<Scope> get_inner_most_functions_scope(std::string& nmspace, const std::string& identifier);
		std::shared_ptr<Scope> get_inner_most_variable_scope(std::string& nmspace, const std::string& identifier);

		bool push_namespace(const std::string nmspace);
		void pop_namespace(bool pop);
		const std::string& get_namespace() const;
	};
}

#endif // !META_VISITOR_HPP
