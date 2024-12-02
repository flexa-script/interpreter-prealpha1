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

std::shared_ptr<Scope> MetaVisitor::get_inner_most_variable_scope(std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_variable(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					// returns real namespace
					nmspace = prgnmspace;
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' variable");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> MetaVisitor::get_inner_most_struct_definition_scope(std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			bool found = false;
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_structure_definition(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					// returns real namespace
					nmspace = prgnmspace;
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' struct");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> MetaVisitor::get_inner_most_function_scope(std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition*>* signature, dim_eval_func_t evaluate_access_vector_ptr, bool strict) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr, strict); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					// returns real namespace
					nmspace = prgnmspace;
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' function");
		}
	}
	return scopes[nmspace][i];
}

std::shared_ptr<Scope> MetaVisitor::get_inner_most_functions_scope(std::string& nmspace, const std::string& identifier) {
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function_name(identifier); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function_name(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					// returns real namespace
					nmspace = prgnmspace;
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("something went wrong searching '" + identifier + "' fuction");
		}
	}
	return scopes[nmspace][i];
}
