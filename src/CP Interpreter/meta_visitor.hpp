#ifndef META_VISITOR_HPP
#define META_VISITOR_HPP

#include <string>
#include <vector>
#include <map>

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
		std::stack<std::string> current_namespace;
		std::map<std::string, std::vector<std::string>> program_nmspaces;

		MetaVisitor();
		virtual ~MetaVisitor() = 0;

		//std::vector<std::string> get_unique_namespaces();
		bool push_namespace(const std::string nmspace);
		void pop_namespace(bool pop);
		const std::string& get_namespace() const;

		virtual long long hash(ASTExprNode*) = 0;
		virtual long long hash(ASTIdentifierNode*) = 0;
		virtual long long hash(ASTLiteralNode<cp_bool>*) = 0;
		virtual long long hash(ASTLiteralNode<cp_int>*) = 0;
		virtual long long hash(ASTLiteralNode<cp_float>*) = 0;
		virtual long long hash(ASTLiteralNode<cp_char>*) = 0;
		virtual long long hash(ASTLiteralNode<cp_string>*) = 0;
	};
}

#endif // !META_VISITOR_HPP
