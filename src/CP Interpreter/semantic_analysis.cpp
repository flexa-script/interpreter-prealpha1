#include <utility>
#include <iostream>

#include "semantic_analysis.hpp"
#include "util.hpp"


using namespace visitor;
using namespace parser;


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: current_expression(SemanticValue()),
	Visitor(programs, main_program, main_program ? main_program->name : "main") {
	scopes["main"].push_back(global_scope);
	register_built_in_functions();
};

void SemanticAnalyser::start() {
	visit(current_program);
}

std::string SemanticAnalyser::get_namespace(std::string nmspace) {
	return get_namespace(current_program, nmspace);
}

std::string SemanticAnalyser::get_namespace(ASTProgramNode* program, std::string nmspace) {
	return nmspace.empty() ? (program->alias.empty() ? "main" : program->alias) : nmspace;
}

void SemanticAnalyser::visit(ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void SemanticAnalyser::visit(ASTUsingNode* astnode) {
	std::string libname = axe::Util::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "lib '" + libname + "' not found");
	}

	auto program = programs[libname];

	// add lib to current program
	if (axe::Util::contains(current_program->libs, libname)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "lib '" + libname + "' already declared in " + current_program->name);
	}
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::Util::contains(libs, libname)) {
		libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;
		if (!program->alias.empty()) {
			scopes[program->alias].push_back(new SemanticScope());
		}
		start();
		current_program = prev_program;
	}
}

VariableDefinition SemanticAnalyser::access_struct_variable(std::vector<Identifier> identifier_vector, std::string type_name, std::string nmspace, unsigned int i) {
	SemanticScope* curr_scope;
	try {
		curr_scope = get_inner_most_struct_definition_scope(get_namespace(nmspace), type_name);
	}
	catch (...) {
		throw std::runtime_error("SERR: can't find struct");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(type_name);

	for (auto& var_type_struct : type_struct.variables) {
		if (identifier_vector[i].identifier == var_type_struct.identifier) {
			if ((is_struct(var_type_struct.type) || is_any(var_type_struct.type)) && identifier_vector.size() - 1 > i) {
				return access_struct_variable(identifier_vector, var_type_struct.type_name, var_type_struct.type_name_space, ++i);
			}
			else {
				if (identifier_vector.size() - 1 > i) {
					throw std::runtime_error("member '" + var_type_struct.identifier + "' of '" + type_name + "' is not a struct");
				}
				return var_type_struct;
			}
		}
	}
	throw std::runtime_error("SERR: can't determine type of '(" + type_name + ")" + identifier_vector[i].identifier + "'");
}

void SemanticAnalyser::visit(ASTDeclarationNode* astnode) {
	// current scope is the scope at the back
	SemanticScope* current_scope = scopes[get_namespace()].back();

	// if variable already declared, throw error
	if (current_scope->already_declared_variable(astnode->identifier)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "variable '" + astnode->identifier + "' already declared");
	}

	if (is_void(astnode->type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "variables can not be assigned with void type '" + astnode->identifier + "'");
	}

	// visit the expression to update current type
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		current_expression = SemanticValue(Type::T_UNDEF, astnode->row, astnode->col);
	}

	auto decl_current_expr = new SemanticValue();
	decl_current_expr->copy_from(&current_expression);
	decl_current_expr->hash = astnode->expr ? astnode->expr->hash(this) : 0;

	if (astnode->is_const && !current_expression.is_const) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "initializer of '" + astnode->identifier + "' is not a constant");
	}

	auto astnode_type = is_undefined(astnode->type) ? Type::T_ANY : astnode->type;
	auto astnode_array_type = is_undefined(astnode->array_type) && astnode->dim.size() > 0 ? Type::T_ANY : astnode->array_type;
	auto astnode_type_name = astnode->type_name.empty() ? decl_current_expr->type_name : astnode->type_name;

	// similar types
	if (is_float(astnode->type) && is_int(current_expression.type)
		|| is_string(astnode->type) && is_char(current_expression.type)) {
		current_scope->declare_variable(astnode->identifier, astnode_type, astnode_array_type,
			astnode->dim, astnode_type_name, astnode->type_name_space, decl_current_expr, astnode->is_const, astnode->row, astnode->col);
	}
	// equal types and special types (any, struct and array)
	else if (match_type(astnode->type, current_expression.type) // types match
		|| is_any(astnode->type) || is_struct(astnode->type) || is_array(astnode->type) // variable special types
		|| is_void(current_expression.type) || is_undefined(current_expression.type)) { // special expressions

		// handle struct
		if (is_struct(astnode->type) || is_struct(current_expression.type)) {

			if (is_struct(current_expression.type) && !is_struct(astnode->type) && !is_any(astnode->type)) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier +
					"(" + type_str(astnode->type) + ")' with type '" + current_expression.type_name + "'");
			}

			if (is_struct(astnode->type) && is_struct(current_expression.type) && !is_any(current_expression.type) && astnode->type_name != current_expression.type_name) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier +
					"(" + astnode->type_name + ")' with type '" + current_expression.type_name + "'");
			}

			// check if is any var
			if (is_any(astnode->type)) {
				decl_current_expr->type = current_expression.type;
				decl_current_expr->type_name = current_expression.type_name;
			}

			current_scope->declare_variable(astnode->identifier, astnode_type, astnode_array_type,
				astnode->dim, astnode_type_name, astnode->type_name_space, decl_current_expr, astnode->is_const, astnode->row, astnode->col);

		}
		// handle array
		else if (is_array(astnode->type)) {

			//for (auto& d : astnode->dim) {
			//	if (d) {
			//		d->accept(this);
			//		if (!current_expression.is_const) {
			//			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "array size must be a constant expression");
			//		}
			//	}
			//}

			if (astnode->expr) {
				if (!is_array(decl_current_expr->type) && !is_void(decl_current_expr->type)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array expression assigning '" + astnode->identifier + "'");
				}

				if (auto arr = dynamic_cast<ASTArrayConstructorNode*>(astnode->expr)) {

					auto expr_dim = calculate_array_dim_size(arr);
					auto pars_dim = evaluate_access_vector(astnode->dim);

					if (expr_dim.size() != 1) {
						if (astnode->dim.size() != expr_dim.size()) {
							throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier + "'");
						}

						for (size_t dc = 0; dc < astnode->dim.size(); ++dc) {
							if (astnode->dim.at(dc) && pars_dim.at(dc) != expr_dim.at(dc)) {
								throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array size assigning '" + astnode->identifier + "'");
							}
						}
					}
				}

			}
			current_scope->declare_variable(astnode->identifier, astnode_type, astnode_array_type,
				astnode->dim, astnode_type_name, astnode->type_name_space, decl_current_expr, astnode->is_const, astnode->row, astnode->col);
		}
		// handle equal types or any
		else if (match_type(astnode->type, current_expression.type) || is_any(astnode->type)
			|| is_void(current_expression.type) || is_undefined(current_expression.type)) {
			current_scope->declare_variable(astnode->identifier, astnode_type, astnode_array_type,
				astnode->dim, astnode_type_name, astnode->type_name_space, decl_current_expr, astnode->is_const, astnode->row, astnode->col);
		}
		else {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "found " + type_str(current_expression.type) + " defining '"
				+ astnode->identifier + "', expected " + type_str(astnode->type) + "");
		}
	}
	else { // types don't match
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "found " + type_str(current_expression.type) + " in definition of '"
			+ astnode->identifier + "', expected " + type_str(astnode->type) + "");
	}
}

SemanticScope* SemanticAnalyser::get_inner_most_variable_scope(std::string nmspace, std::string identifier) {
	// determine the inner-most scope in which the value is declared
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_variable(identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error("identifier '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

void SemanticAnalyser::visit(ASTAssignmentNode* astnode) {
	if (astnode->op != "=" && astnode->op != "+=" && astnode->op != "-=" && astnode->op != "*=" && astnode->op != "/=" && astnode->op != "%=") {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid assignment operator '" + astnode->op + "'");
	}

	// determine the inner-most scope in which the value is declared
	SemanticScope* curr_scope;
	try {
		curr_scope = get_inner_most_variable_scope(astnode->nmspace.empty() ? get_namespace() : astnode->nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (...) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + astnode->identifier_vector[0].identifier +
			"' being reassigned was never declared");
	}

	// get base variable
	auto declared_variable = curr_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	auto decl_var_expression = declared_variable->value;

	if (declared_variable->is_const) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + astnode->identifier_vector[0].identifier + "' constant being reassigned");
	}

	// visit the expression to update current type
	astnode->expr->accept(this);
	auto assignment_expr = current_expression;

	if (astnode->op == "+=") {
		if (!is_int(assignment_expr.type) && !is_float(assignment_expr.type)
			&& !is_int(decl_var_expression->type) && !is_float(decl_var_expression->type)
			&& !is_string(assignment_expr.type) && !is_string(decl_var_expression->type)
			&& !is_string(decl_var_expression->type) && !is_char(assignment_expr.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid value type '" +
				type_str(decl_var_expression->type) + "' and '" +
				type_str(assignment_expr.type) + "' to '" + astnode->op + "' operation");
		}
	}
	else if (astnode->op == "-=" || astnode->op == "*=" || astnode->op == "/=") {
		if (!is_int(assignment_expr.type) && !is_float(assignment_expr.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid right value type '" +
				type_str(assignment_expr.type) + "' to execute '" + astnode->op + "' operation");
		}
		if (!is_int(decl_var_expression->type) && !is_float(decl_var_expression->type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid left value type '" +
				type_str(decl_var_expression->type) + "' to execute '" + astnode->op + "' operation");
		}
	}
	else if (astnode->op == "%=") {
		if (!is_int(assignment_expr.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid right value type '" +
				type_str(assignment_expr.type) + "' to execute '" + astnode->op + "' operation");
		}
		if (!is_int(decl_var_expression->type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid left value type '" +
				type_str(decl_var_expression->type) + "' to execute '" + astnode->op + "' operation");
		}
	}

	// allow mismatched type in the case of declaration of int to real
	if (is_float(declared_variable->type) && is_int(assignment_expr.type) ||
		is_string(declared_variable->type) && is_char(assignment_expr.type)) {
		delete declared_variable->value;
		declared_variable->value = new SemanticValue();
		declared_variable->value->copy_from(&assignment_expr);
	}
	else if (is_any(declared_variable->type)) {
		if (declared_variable->value == decl_var_expression) {
			delete declared_variable->value;
			declared_variable->value = new SemanticValue();
			declared_variable->value->copy_from(&assignment_expr);
		}
		else {
			decl_var_expression->copy_from(&assignment_expr);
		}
	}
	else if (is_struct(declared_variable->type)) {

		auto str_curr_var_type = decl_var_expression->type;
		auto str_curr_var_type_name = decl_var_expression->type_name;

		if (astnode->identifier_vector.size() > 1) {
			auto sub_var = access_struct_variable(astnode->identifier_vector, decl_var_expression->type_name, decl_var_expression->type_name_space, 1);
			str_curr_var_type = sub_var.type;
			str_curr_var_type_name = sub_var.type_name;
		}

		if (assignment_expr.type_name != str_curr_var_type_name && !is_void(current_expression.type) && !is_any(decl_var_expression->type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + astnode->identifier_vector[0].identifier +
				"' struct, expected '" + str_curr_var_type_name + "', found '" + assignment_expr.type_name + "'");
		}

		if (auto str_expr = dynamic_cast<ASTStructConstructorNode*>(astnode->expr)) {
			validate_struct_assign(curr_scope, decl_var_expression, str_expr);
		}
		else if (auto str_expr = dynamic_cast<ASTIdentifierNode*>(astnode->expr)) {
			// determine the inner-most scope in which the value is declared
			SemanticScope* assng_scope;
			try {
				assng_scope = get_inner_most_variable_scope(get_namespace(str_expr->nmspace), str_expr->identifier_vector[0].identifier);
			}
			catch (...) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + str_expr->identifier_vector[0].identifier +
					"' being reassigned was never declared");
			}

			// get base variable
			auto expr_declared_variable = assng_scope->find_declared_variable(str_expr->identifier_vector[0].identifier);
			auto expr_variable_expression = expr_declared_variable->value;

			curr_scope->declare_variable(astnode->identifier_vector[0].identifier, declared_variable->type, declared_variable->array_type,
				declared_variable->dim, declared_variable->type_name, declared_variable->type_name_space, expr_variable_expression,
				declared_variable->is_const, declared_variable->row, declared_variable->col);
		}
	}
	else if (is_array(declared_variable->type)) { // variable is array

		if (is_undefined(decl_var_expression->type) && astnode->identifier_vector[0].access_vector.size() > 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "can't assing position of undefinded '"
				+ astnode->identifier_vector[0].identifier + "' array");
		}

		if (!is_array(assignment_expr.type) && astnode->identifier_vector[0].access_vector.size() == 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + type_str(assignment_expr.type)
				+ "' type assigning '" + astnode->identifier_vector[0].identifier + "' array");
		}

		if (is_array(assignment_expr.type) && is_array(decl_var_expression->type) && astnode->identifier_vector[0].access_vector.size() == 0) {

			std::vector<unsigned int> var_dim = evaluate_access_vector(decl_var_expression->dim);
			std::vector<unsigned int> expr_dim = evaluate_access_vector(assignment_expr.dim);

			if (var_dim.size() != expr_dim.size()) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier_vector[0].identifier + "'");
			}

			for (size_t dc = 0; dc < var_dim.size(); ++dc) {
				if (declared_variable->dim.at(dc) && var_dim.at(dc) != expr_dim.at(dc)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array size assigning '" + astnode->identifier_vector[0].identifier + "'");
				}
			}
		}

		if (declared_variable->value == decl_var_expression) { // is root value, assign a new one
			delete declared_variable->value;
			declared_variable->value = new SemanticValue();
			declared_variable->value->copy_from(&assignment_expr);
		}
		else { // is subvalue, assign specific value
			decl_var_expression->copy_from(&assignment_expr);
		}
	}
	else if (match_type(declared_variable->type, assignment_expr.type)) {
		delete declared_variable->value;
		declared_variable->value = new SemanticValue();
		declared_variable->value->copy_from(&assignment_expr);
	}
	// otherwise throw error
	else if (!match_type(declared_variable->type, assignment_expr.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + astnode->identifier_vector[0].identifier +
			"', expected " + type_str(declared_variable->type) + ", found " + type_str(assignment_expr.type) + "");
	}

}

void SemanticAnalyser::determine_array_type(ASTArrayConstructorNode* astnode) {
	auto aux_curr_type = current_expression.type;
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);

		if (auto expr = dynamic_cast<ASTArrayConstructorNode*>(astnode->values.at(i))) {
			determine_array_type(expr);
		}
		else {
			check_array_type(astnode->values.at(i), astnode->row, astnode->col);
		}
	}
	current_expression.type = aux_curr_type;
}

void SemanticAnalyser::check_array_type(ASTExprNode* astnode, unsigned int row, unsigned int col) {
	auto aux_curr_type = current_expression.type;
	astnode->accept(this);

	if (is_any(current_expression.array_type) || is_undefined(current_expression.array_type) || is_void(current_expression.array_type)) {
		current_expression.array_type = current_expression.type;
	}
	if (!match_type(current_expression.array_type, current_expression.type)) {
		throw std::runtime_error(msg_header(row, col) + "mismatched type in array definition");
	}
	current_expression.type = aux_curr_type;
}

std::vector<unsigned int> SemanticAnalyser::calculate_array_dim_size(ASTArrayConstructorNode* arr) {
	auto dim = std::vector<unsigned int>();

	dim.push_back(arr->values.size());

	if (auto sub_arr = dynamic_cast<ASTArrayConstructorNode*>(arr->values.at(0))) {
		auto dim2 = calculate_array_dim_size(sub_arr);
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

SemanticScope* SemanticAnalyser::get_inner_most_struct_definition_scope(std::string nmspace, std::string identifier) {
	// determine the inner-most scope in which the value is declared
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error("identifier '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

void SemanticAnalyser::validate_struct_assign(SemanticScope* curr_scope, SemanticValue* expression, ASTStructConstructorNode* expr) {
	// determine the inner-most scope in which the value is declared
	SemanticScope* tn_scope;
	try {
		auto nmspace = get_namespace(expr->nmspace);
		tn_scope = get_inner_most_struct_definition_scope(nmspace, expr->type_name);
	}
	catch (...) {
		throw std::runtime_error(msg_header(expr->row, expr->col) + "struct '" + expr->type_name +
			"' was not declared");
	}
	auto struct_def = tn_scope->find_declared_structure_definition(expr->type_name);

	for (auto const& expr_value : expr->values) {

		VariableDefinition str_var_def;
		auto found = false;
		for (size_t i = 0; i < struct_def.variables.size(); ++i) {
			str_var_def = struct_def.variables[i];
			if (str_var_def.identifier == expr_value.first) {
				found = true;
				break;
			}
		}

		if (!found) {
			throw std::runtime_error(msg_header(expr->row, expr->col) + "'" + expr_value.first + "' is not a member of '" + expression->type_name + "'");
		}

		expr_value.second->accept(this);
		auto hash = expr_value.second ? expr_value.second->hash(this) : 0;

		if (!match_type(current_expression.type, str_var_def.type) && !is_any(str_var_def.type) && !is_void(current_expression.type)) {
			throw std::runtime_error(msg_header(expr->row, expr->col) + "invalid type assigning " + str_var_def.identifier);
		}

		if (is_array(str_var_def.type) && !match_type(current_expression.array_type, str_var_def.array_type) && !is_any(str_var_def.array_type) && !is_void(current_expression.type)) {
			throw std::runtime_error(msg_header(expr->row, expr->col) + "invalid type assigning " + str_var_def.identifier);
		}

		if (is_array(str_var_def.type)) {
			std::vector<unsigned int> var_dim = evaluate_access_vector(str_var_def.dim);
			std::vector<unsigned int> expr_dim = evaluate_access_vector(current_expression.dim);

			if (var_dim.size() != expr_dim.size()) {
				throw std::runtime_error(msg_header(expr->row, expr->col) + "invalid array dimension assigning '" + str_var_def.identifier + "'");
			}

			for (size_t dc = 0; dc < var_dim.size(); ++dc) {
				if (str_var_def.dim.at(dc) && var_dim.at(dc) != expr_dim.at(dc)) {
					throw std::runtime_error(msg_header(expr->row, expr->col) + "invalid array size assigning '" + str_var_def.identifier + "'");
				}
			}
		}

		if (is_struct(str_var_def.type)) {
			if (str_var_def.type_name != current_expression.type_name && !is_void(current_expression.type)) {
				throw std::runtime_error(msg_header(expr->row, expr->col) + "mismatched type for '" + str_var_def.identifier +
					"' struct, expected '" + current_expression.type_name + "', found '" + current_expression.type_name + "'");
			}
		}

		if (is_struct(str_var_def.type)) {
			if (is_void(current_expression.type)) {
			}
			else if (auto str_expr = dynamic_cast<ASTStructConstructorNode*>(expr_value.second)) {
			}
			else {
				throw std::runtime_error(msg_header(expr->row, expr->col) + "expected struct constructor");
			}
		}

	}
}

std::vector<unsigned int> SemanticAnalyser::evaluate_access_vector(std::vector<ASTExprNode*> exprAcessVector) {
	auto access_vector = std::vector<unsigned int>();
	for (auto& expr : exprAcessVector) {
		unsigned int val = 0;
		if (expr) {
			expr->accept(this);
			val = expr->hash(this);
			if (!is_int(current_expression.type)) {
				throw std::runtime_error(msg_header(0, 0) + "array index access must be a integer value");
			}
		}
		access_vector.push_back(val);
	}
	return access_vector;
}

void SemanticAnalyser::visit(ASTPrintNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
}

void SemanticAnalyser::visit(ASTReturnNode* astnode) {
	// update current expression
	astnode->expr->accept(this);

	// if we are not global, check that we return current function return type
	if (!current_function.empty()) {

		auto true_type = is_array(current_function.top().type) ? current_function.top().array_type : current_function.top().type;

		if (!is_any(true_type) && !is_void(current_expression.type)
			&& current_expression.type != true_type) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' function return type");
		}
		if (is_array(true_type)) {

			std::vector<unsigned int> expr_dim;
			std::vector<unsigned int> pars_dim = evaluate_access_vector(current_function.top().dim);

			if (auto arr_expr = dynamic_cast<ASTArrayConstructorNode*>(astnode->expr)) {
				determine_array_type(arr_expr);
				expr_dim = calculate_array_dim_size(arr_expr);
			}
			else if (auto id_expr = dynamic_cast<ASTIdentifierNode*>(astnode->expr)) {
				SemanticScope* curr_scope;
				try {
					auto nmspace = get_namespace(id_expr->nmspace);
					curr_scope = get_inner_most_variable_scope(nmspace, id_expr->identifier_vector[0].identifier);
				}
				catch (...) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + id_expr->identifier_vector[0].identifier +
						"' was not declared");
				}

				// get base variable
				auto declared_variable = curr_scope->find_declared_variable(id_expr->identifier_vector[0].identifier);
				expr_dim = evaluate_access_vector(declared_variable->value->dim);
				current_expression.array_type = declared_variable->value->array_type;
			}
			else {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' function return expression");
			}

			if (current_function.top().array_type != Type::T_ANY && current_function.top().array_type != current_expression.array_type) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' function array type return");
			}

			if (current_function.top().dim.size() != expr_dim.size()) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' array dimension return");
			}

			for (size_t dc = 0; dc < current_function.top().dim.size(); ++dc) {
				if (current_function.top().dim.at(dc) && pars_dim.at(dc) != expr_dim.at(dc)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' array size return");
				}
			}
		}
		if (is_struct(true_type)) {
			if (current_function.top().type_name != current_expression.type_name) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.top().identifier + "' function struct type return");
			}
		}

	}
}

SemanticScope* SemanticAnalyser::get_inner_most_function_scope(std::string nmspace, std::string identifier, std::vector<Type> signature) {
	// determine the inner-most scope in which the value is declared
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature); i--) {
		if (i <= 0) {
			throw std::runtime_error("identifier '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

void SemanticAnalyser::register_built_in_functions() {
	builtin_functions["system"] = [](std::vector<Type> args) {
		if (args.size() != 1) {
			return false;
		}
		if (!is_string(args[0])) {
			return false;
		}
		return true;
	};

	builtin_functions["test"] = [](std::vector<Type> args) {
		return true;
	};
}

void SemanticAnalyser::visit(ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<Type> signature;

	// for each parameter
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(current_expression.type);
	}

	// make sure the function exists in some scope i
	SemanticScope* curr_scope;
	try {
		auto nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_function_scope(nmspace, astnode->identifier, signature);
	}
	catch (...) {
		auto res = builtin_functions[astnode->identifier](signature);

		if (!res) {
			std::string func_name = astnode->identifier + "(";
			for (auto param : signature) {
				func_name += type_str(param) + ", ";
			}
			if (signature.size() > 0) {
				func_name.pop_back();   // remove last whitespace
				func_name.pop_back();   // remove last comma
			}
			func_name += ")";
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "function '" + func_name + "' was never declared");
		}
		return;
	}

	auto curr_function = curr_scope->find_declared_function(astnode->identifier, signature);

	current_expression = SemanticValue();
	current_expression.type = curr_function.type;
	current_expression.array_type = curr_function.array_type;
	current_expression.type_name = curr_function.type_name;
	current_expression.dim = curr_function.dim;
	current_expression.row = curr_function.row;
	current_expression.col = curr_function.col;
}

void SemanticAnalyser::visit(ASTFunctionDefinitionNode* astnode) {
	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes[get_namespace()]) {
		if (scope->already_declared_function(astnode->identifier, astnode->signature)) {
			std::string signature = "(";
			for (auto param : astnode->signature) {
				signature += type_str(param) + ", ";
			}
			if (astnode->signature.size() > 0) {
				signature.pop_back();   // remove last whitespace
				signature.pop_back();   // remove last comma
			}
			signature += ")";


			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "function " + astnode->identifier + signature + " already defined");
		}
	}

	auto has_return = returns(astnode->block);
	auto type = is_void(astnode->type) && has_return ? Type::T_ANY : astnode->type;
	auto array_type = (is_void(astnode->array_type) || is_undefined(astnode->array_type)) && has_return ? Type::T_ANY : astnode->type;

	// add function to symbol table
	scopes[get_namespace()].back()->declare_function(astnode->identifier, type, astnode->type_name, astnode->type_name_space,
		array_type, astnode->dim, astnode->signature, astnode->parameters, astnode->block, astnode->row, astnode->row);

	auto curr_function = scopes[get_namespace()].back()->find_declared_function(astnode->identifier, astnode->signature);

	// push current function type into function stack
	current_function.push(curr_function);

	// check semantics of function block by visiting nodes
	astnode->block->accept(this);

	if (!is_void(type)) {
		// check that the function body returns
		if (!has_return) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "defined function '" + astnode->identifier + "' is not guaranteed to return a value");
		}
	}

	// end the current function
	current_function.pop();
}

void SemanticAnalyser::visit(ASTBlockNode* astnode) {
	// create new scope
	scopes[get_namespace()].push_back(new SemanticScope());

	if (!current_function.empty()) {
		// add function parameters to the current scope
		for (size_t i = 0; i < current_function.top().parameters.size(); ++i) {
			auto param = current_function.top().parameters[i];

			auto var_expr = new SemanticValue();
			var_expr->type = param.type;
			var_expr->array_type = param.array_type;
			var_expr->type_name = param.type_name;
			var_expr->dim = param.dim;
			var_expr->row = param.row;
			var_expr->col = param.col;

			scopes[get_namespace()].back()->declare_variable(param.identifier, param.type, param.array_type,
				param.dim, param.type_name, param.type_name_space, var_expr, false, param.row, param.col);
		}
	}

	// visit each statement in the block
	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes[get_namespace()].pop_back();
}

void SemanticAnalyser::visit(ASTContinueNode* astnode) { }

void SemanticAnalyser::visit(ASTBreakNode* astnode) { }

void SemanticAnalyser::visit(ASTSwitchNode* astnode) {
	// create new scope
	scopes[get_namespace()].push_back(new SemanticScope());

	astnode->parsed_case_blocks = new std::map<unsigned int, unsigned int>();

	astnode->condition->accept(this);

	// visit each case expresion in the block
	for (auto& expr : *astnode->case_blocks) {
		expr.first->accept(this);
		auto hash = expr.first->hash(this);
		if (astnode->parsed_case_blocks->contains(hash)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "duplicated case value: '" + std::to_string(hash) + "'");
		}
		astnode->parsed_case_blocks->emplace(hash, expr.second);
		if (!current_expression.is_const) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expression is not an constant expression");
		}
	}

	// visit each statement in the block
	for (auto& stmt : *astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes[get_namespace()].pop_back();
}

void SemanticAnalyser::visit(ASTElseIfNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the if block
	astnode->block->accept(this);
}

void SemanticAnalyser::visit(ASTIfNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the if block
	astnode->if_block->accept(this);

	for (auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	// if there is an else block, check it too
	if (astnode->else_block) {
		astnode->else_block->accept(this);
	}
}

void SemanticAnalyser::visit(ASTForNode* astnode) {
	scopes[get_namespace()].push_back(new SemanticScope());

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[2]->accept(this);
	}
	astnode->block->accept(this);

	// close scope
	scopes[get_namespace()].pop_back();
}

void SemanticAnalyser::visit(ASTForEachNode* astnode) {
	Type decl_type;
	Type col_type;

	scopes[get_namespace()].push_back(new SemanticScope());

	astnode->itdecl->accept(this);
	decl_type = current_expression.type;

	astnode->collection->accept(this);
	col_type = current_expression.array_type;

	if (is_any(decl_type)) {
		decl_type = col_type;
		auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl);

		SemanticScope* curr_scope;
		try {
			curr_scope = get_inner_most_variable_scope(get_namespace(), idnode->identifier);
		}
		catch (...) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + idnode->identifier +
				"' was not declared");
		}

		auto declared_variable = curr_scope->find_declared_variable(idnode->identifier);

		curr_scope->change_current_variable_type(idnode->identifier, col_type);
		curr_scope->change_variable_type_name(idnode->identifier, current_expression.type_name);
	}

	if (!is_array(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array in foreach");
	}

	if (!match_type(decl_type, col_type) && !is_any(decl_type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched types");
	}

	astnode->block->accept(this);

	// close scope
	scopes[get_namespace()].pop_back();
}

void SemanticAnalyser::visit(ASTWhileNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the while block
	astnode->block->accept(this);
}

// determines whether a statement definitely returns or not
bool SemanticAnalyser::returns(ASTNode* astnode) {
	// base case: if the statement is a return statement, then it definitely returns
	if (dynamic_cast<ASTReturnNode*>(astnode)) {
		return true;
	}

	// for a block, if at least one statement returns, then the block returns
	if (auto block = dynamic_cast<ASTBlockNode*>(astnode)) {
		for (auto& blk_stmt : block->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	// an if-elif-else block returns only if both the if and the else statement return
	if (auto ifstmt = dynamic_cast<ASTIfNode*>(astnode)) {
		auto ifreturn = returns(ifstmt->if_block);
		auto elifreturn = true;
		auto elsereturn = true;
		for (auto& elif : ifstmt->else_ifs) {
			if (!returns(elif->block)) {
				elifreturn = false;
				break;
			}
		}
		if (ifstmt->else_block) {
			elsereturn = returns(ifstmt->else_block);
		}
		return ifreturn && elifreturn && elsereturn;
	}

	// in switch block, if at least one statement returns, then the block returns
	if (auto switchstmt = dynamic_cast<ASTSwitchNode*>(astnode)) {
		for (auto& blk_stmt : *switchstmt->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	// others blocks returns if its block returns
	if (auto forstmt = dynamic_cast<ASTForNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (auto forstmt = dynamic_cast<ASTForEachNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (auto forstmt = dynamic_cast<ASTForEachNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (auto whilestmt = dynamic_cast<ASTWhileNode*>(astnode)) {
		return returns(whilestmt->block);
	}

	// other statements do not return
	return false;
}

void SemanticAnalyser::visit(ASTStructDefinitionNode* astnode) {
	// first check that all enclosing scopes have not already defined the struct
	try {
		SemanticScope* curr_scope = get_inner_most_struct_definition_scope(get_namespace(), astnode->identifier);
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "struct '" + astnode->identifier +
			"' already defined");
	}
	catch (...) {}

	scopes[get_namespace()].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_bool>*) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_int>*) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_float>*) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_char>*) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_CHAR;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_string>*) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTArrayConstructorNode* astnode) {
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
		auto new_val = new SemanticValue();
		new_val->copy_from(&current_expression);
	}

	determine_array_type(astnode);
	auto current_expression_array_type = current_expression.array_type;
	current_expression = SemanticValue();
	current_expression.array_type = current_expression_array_type;
	current_expression.type = Type::T_ARRAY;
	current_expression.type_name = "";
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTStructConstructorNode* astnode) {
	// determine the inner-most scope in which the value is declared
	SemanticScope* curr_scope;
	try {
		auto nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "struct '" + astnode->type_name +
			"' was not declared");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);

	for (auto& expr : astnode->values) {
		bool found = false;
		VariableDefinition var_type_struct;
		for (size_t i = 0; i < type_struct.variables.size(); ++i) {
			var_type_struct = type_struct.variables[i];
			if (var_type_struct.identifier == expr.first) {
				found = true;
				break;
			}
		}
		if (!found) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + expr.first + "' is not a member of '" + astnode->type_name + "'");
		}

		expr.second->accept(this);

		if (!is_any(var_type_struct.type) && !is_void(current_expression.type) && !match_type(var_type_struct.type, current_expression.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid type " + type_str(var_type_struct.type) +
				" trying to assign '" + astnode->type_name + "' struct");
		}
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRUCT;
	current_expression.type_name = astnode->type_name;
	current_expression.type_name_space = astnode->nmspace;
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;

	declare_structure();
}

void SemanticAnalyser::declare_structure() {
	// determine the inner-most scope in which the value is declared
	SemanticScope* curr_scope;
	try {
		curr_scope = get_inner_most_struct_definition_scope(get_namespace(current_expression.type_name_space), current_expression.type_name);
	}
	catch (...) {
		throw std::runtime_error("SERR: can't find struct");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(current_expression.type_name);

	for (auto& var_type_struct : type_struct.variables) {

	}
}

void SemanticAnalyser::visit(ASTBinaryExprNode* astnode) {
	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	Type l_type = current_expression.type;
	if (is_array(current_expression.type)) {
		l_type = current_expression.array_type;
	}

	// then right node
	astnode->right->accept(this);
	Type r_type = current_expression.type;
	if (is_array(current_expression.type)) {
		r_type = current_expression.array_type;
	}

	// these only work for int/float
	if (op == "-" || op == "/" || op == "*" || op == "%") {
		if (!is_int(l_type) && !is_float(l_type) || !is_int(r_type) && !is_float(r_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected numerical operands for '" + op + "' operator");
		}
		if (op == "%" && (is_float(l_type) || is_float(r_type))) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid operands to mod expression ('" + type_str(l_type) + "' and '" + type_str(r_type) + "')");
		}

		// if both int, then expression is int, otherwise float
		current_expression.type = (is_int(l_type) && is_int(r_type)) ? Type::T_INT : Type::T_FLOAT;
	}
	else if (op == "+") {
		// + works for all types except bool
		if (is_bool(l_type) || is_array(l_type) || is_struct(l_type) || is_bool(r_type) || is_array(r_type) || is_struct(r_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid operand for '+' operator, expected numerical or string operand");
		}
		if ((is_string(l_type) || is_char(l_type)) && (is_string(r_type) || is_char(r_type))) {
			current_expression.type = is_char(l_type) && is_char(r_type) ? Type::T_CHAR : Type::T_STRING;
		}
		else if (is_string(l_type) || is_char(l_type) || is_string(r_type) || is_char(r_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched operands for '+' operator, found " + type_str(l_type) + " on the left, but " + type_str(r_type) + " on the right");
		}
		else { // real/int possibilities remain. If both int, then result is int, otherwise result is real
			current_expression.type = (is_int(l_type) && is_int(r_type)) ? Type::T_INT : Type::T_FLOAT;
		}
	}
	else if (op == "and" || op == "or") {
		// and/or only work for bool
		if (is_bool(l_type) && is_bool(r_type)) {
			current_expression.type = Type::T_BOOL;
		}
		else {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected two boolean operands for '" + op + "' operator");
		}
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
		// rel-ops only work for numeric types
		if ((!is_float(l_type) && !is_int(l_type)) || (!is_float(r_type) && !is_int(r_type))) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected two numerical operands for '" + op + "' operator");
		}
		current_expression.type = Type::T_BOOL;
	}
	else if (op == "==" || op == "!=") {
		// == and != only work for like types
		if (!match_type(l_type, r_type) && (!is_float(l_type) || !is_int(r_type)) && (!is_int(l_type) || !is_float(r_type)) && !is_void(l_type) && !is_void(r_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected arguments of the same type '" + op + "' operator");
		}
		current_expression.type = Type::T_BOOL;
	}
	else {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "unhandled semantic error in binary operator");
	}
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTIdentifierNode* astnode) {
	auto identifier = astnode->identifier_vector[0].identifier;

	// determine the inner-most scope in which the value is declared
	SemanticScope* curr_scope;
	auto nmspace = get_namespace(astnode->nmspace);
	try {
		curr_scope = get_inner_most_variable_scope(nmspace, identifier);
	}
	catch (...) {
		current_expression = SemanticValue();
		if (identifier == "bool" || identifier == "int" || identifier == "float"
			|| identifier == "char" || identifier == "string") {
			return;
		}
		try {
			curr_scope = get_inner_most_struct_definition_scope(nmspace, identifier);
			return;
		}
		catch (...) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + identifier +
				"' was not declared");
		}
	}

	auto declared_variable = curr_scope->find_declared_variable(identifier);
	auto variable_expr = declared_variable->value;

	if (is_undefined(variable_expr->type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "variable '" + identifier + "' is undefined");
	}

	current_expression = *variable_expr;
	current_expression.type = is_any(declared_variable->type) ? variable_expr->type : declared_variable->type;

	if (astnode->identifier_vector.size() > 1) {
		auto sub_var = access_struct_variable(astnode->identifier_vector, declared_variable->type_name, declared_variable->type_name_space, 1);
		current_expression.type = sub_var.type;
		current_expression.type_name = sub_var.type_name;
		current_expression.array_type = sub_var.array_type;
		current_expression.dim = sub_var.dim;
	}

}

void SemanticAnalyser::visit(ASTUnaryExprNode* astnode) {
	// determine expression type
	astnode->expr->accept(this);

	// handle different cases
	switch (current_expression.type) {
	case Type::T_INT:
	case Type::T_FLOAT:
		if (astnode->unary_op != "+" && astnode->unary_op != "-" && astnode->unary_op != "--" && astnode->unary_op != "++") {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "operator '" + astnode->unary_op + "' in front of numerical expression");
		}
		break;
	case Type::T_BOOL:
		if (astnode->unary_op != "not") {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "operator '" + astnode->unary_op + "' in front of boolean expression");
		}
		break;
	default:
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "incompatible unary operator '" + astnode->unary_op +
			"' in front of " + type_str(current_expression.type) + " expression");
	}
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTTypeParseNode* astnode) {
	astnode->expr->accept(this);

	if ((is_array(current_expression.type) || is_struct(current_expression.type))
		&& !is_string(astnode->type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid type conversion from "
			+ type_str(current_expression.type) + " to " + type_str(astnode->type));
	}

	current_expression = SemanticValue();
	current_expression.type = astnode->type;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTReadNode* astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTTypeNode* astnode) {
	astnode->expr->accept(this);
	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTLenNode* astnode) {
	astnode->expr->accept(this);

	if (!is_array(current_expression.type) && !is_string(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "can't read len of type " + type_str(current_expression.type));
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTRoundNode* astnode) {
	astnode->expr->accept(this);

	if (!is_float(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "can't round type " + type_str(current_expression.type));
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTNullNode* astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_VOID;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTThisNode* astnode) {
	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

std::string SemanticAnalyser::msg_header(unsigned int row, unsigned int col) {
	return "(SERR) " + current_program->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
}

unsigned int SemanticAnalyser::hash(ASTExprNode* astnode) {
	return 0;
}

unsigned int SemanticAnalyser::hash(ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(ASTLiteralNode<cp_int>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(ASTLiteralNode<cp_float>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(ASTLiteralNode<cp_char>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(ASTLiteralNode<cp_string>* astnode) {
	return axe::Util::hashcode(astnode->val);
}

unsigned int SemanticAnalyser::hash(ASTIdentifierNode* astnode) {
	// determine the inner-most scope in which the value is declared
	SemanticScope* curr_scope;
	try {
		auto nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (...) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + astnode->identifier_vector[0].identifier +
			"' was not declared");
	}

	// get base variable
	auto declared_variable = curr_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	auto variable_expression = declared_variable->value;

	return variable_expression->hash;
}
