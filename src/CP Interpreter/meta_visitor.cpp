#include "meta_visitor.hpp"
#include "vendor/axeutils.hpp"

using namespace visitor;

MetaVisitor::MetaVisitor() {};

const std::string& MetaVisitor::get_namespace() const {
	return current_namespace.top();
}

//std::vector<std::string> MetaVisitor::get_unique_namespaces() {
//	const auto& curr_program_nmspaces = program_nmspaces[get_namespace()];
//	std::vector<std::string> nmspaces;
//
//	for (const auto& nmspace : curr_program_nmspaces) {
//		if (!axe::CollectionUtils::contains(nmspaces, nmspace)) {
//			nmspaces.push_back(nmspace);
//		}
//	}
//
//	return nmspaces;
//}

bool MetaVisitor::push_namespace(const std::string nmspace) {
	if (!nmspace.empty()) {
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
