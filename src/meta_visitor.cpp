#include "meta_visitor.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;

const std::string& MetaVisitor::get_namespace() const {
	if (current_namespace.empty()) {
		return "";
	}
	return current_namespace.top();
}

bool MetaVisitor::push_namespace(const std::string nmspace) {
	if (!nmspace.empty() && nmspace != get_namespace()) {
		current_namespace.push(nmspace);
		return true;
	}
	return false;
}

void MetaVisitor::pop_namespace(bool pop) {
	if (pop) {
		current_namespace.pop();
	}
}
