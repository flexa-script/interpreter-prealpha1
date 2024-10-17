#ifndef META_VISITOR_HPP
#define META_VISITOR_HPP

#include <string>
#include <vector>
#include <stack>
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

		MetaVisitor() = default;
		virtual ~MetaVisitor() = default;

		bool push_namespace(const std::string nmspace);
		void pop_namespace(bool pop);
		const std::string& get_namespace() const;
	};
}

#endif // !META_VISITOR_HPP
