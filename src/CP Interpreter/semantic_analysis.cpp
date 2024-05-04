#include <utility>
#include <iostream>

#include "semantic_analysis.hpp"
#include "util.hpp"


using namespace visitor;
using namespace parser;


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<ASTProgramNode*> programs)
	: current_expression(SemanticExpression_t()), Visitor(programs, programs[0], programs[0]) {
	// add global scope
	scopes.push_back(global_scope);
};

SemanticAnalyser::~SemanticAnalyser() = default;

void SemanticAnalyser::start() {
	visit(current_program);
}

std::string SemanticAnalyser::get_namespace() {
	if (!current_program->alias.empty() && functions.empty() && current_program->alias != main_program->name) {
		return current_program->alias + ".";
	}
	return "";
}

void SemanticAnalyser::visit(ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void SemanticAnalyser::visit(ASTUsingNode* astnode) {
	for (auto& program : programs) {
		if (astnode->library == program->name) {
			auto prev_program = current_program;
			program->alias = astnode->alias;
			current_program = program;
			start();
			current_program = prev_program;
		}
	}
}

void SemanticAnalyser::visit(ASTDeclarationNode* astnode) {
	// current scope is the scope at the back
	SemanticScope* current_scope = scopes.back();

	// if variable already declared, throw error
	if (current_scope->already_declared_variable(astnode->identifier)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "variable '" + astnode->identifier + "' already declared");
	}

	// visit the expression to update current type
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		current_expression = SemanticExpression_t(Type::T_UNDEF, Type::T_UNDEF, "", nullptr, false, astnode->row, astnode->col);;
	}

	auto decl_current_expr = current_expression;
	decl_current_expr.array_type = is_undefined(current_expression.array_type) ? astnode->array_type : current_expression.array_type;
	decl_current_expr.type_name = current_expression.type_name.empty() ? astnode->type_name : current_expression.type_name;

	if (astnode->is_const && !current_expression.is_const) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "initializer of '" + astnode->identifier + "' is not a constant");
	}

	// similar types
	if (is_float(astnode->type) && is_int(current_expression.type)
		|| is_string(astnode->type) && is_char(current_expression.type)) {
		current_scope->declare_variable(get_namespace() + astnode->identifier, astnode->type, astnode->is_const, &decl_current_expr, astnode->row, astnode->col);
	}
	// equal types and special types (any, struct and array)
	else if (match_type(astnode->type, current_expression.type) || is_any(astnode->type)
		|| is_struct(astnode->type) || is_array(astnode->type) || is_void(current_expression.type)) { // types match

		// handle struct
		if (is_struct(astnode->type) || is_struct(current_expression.type)) {

			if (is_struct(current_expression.type) && !is_struct(astnode->type) && !is_any(astnode->type)) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier +
					"(" + type_str(astnode->type) + ")' with type '" + current_expression.type_name + "'");
			}

			if (!astnode->type_name.empty()) {
				long long i;
				for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(astnode->type_name); i--) {
					if (i <= 0) {
						throw std::runtime_error(msg_header(astnode->row, astnode->col) + "struct '" + astnode->type_name + "' defining '"
							+ astnode->identifier + "' was never declared");
					}
				}
			}

			if (is_struct(astnode->type) && is_struct(current_expression.type) && !is_any(current_expression.type) && astnode->type_name != current_expression.type_name) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier +
					"(" + astnode->type_name + ")' with type '" + current_expression.type_name + "'");
			}

			// check if is any var
			if (is_any(astnode->type)) {
				decl_current_expr.type = current_expression.type;
				decl_current_expr.type_name = current_expression.type_name;
			}

			current_scope->declare_variable(get_namespace() + astnode->identifier, astnode->type, astnode->is_const, &decl_current_expr, astnode->row, astnode->col);

			auto decl_var = current_scope->find_declared_variable(get_namespace() + astnode->identifier);

			if (auto str_expr = dynamic_cast<ASTStructConstructorNode*>(astnode->expr)) {
				assign_structure(current_scope, decl_var->expr, str_expr);
			}
		}
		// handle array
		else if (is_array(astnode->type)) {

			for (auto& d : astnode->dim) {
				if (d) {
					d->accept(this);
					if (!current_expression.is_const) {
						throw std::runtime_error(msg_header(astnode->row, astnode->col) + "array size must be a constant expression");
					}
				}
			}

			if (astnode->expr) {
				if (!is_array(decl_current_expr.type) && !is_void(decl_current_expr.type)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array expression assigning '" + astnode->identifier + "'");
				}

				if (auto arr = dynamic_cast<ASTArrayConstructorNode*>(astnode->expr)) {

					auto expr_dim = calculate_array_dim_size(arr);
					auto pars_dim = evaluate_access_vector(astnode->dim);

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
			current_scope->declare_variable(get_namespace() + astnode->identifier, astnode->type, astnode->is_const, &decl_current_expr, astnode->row, astnode->col);
		}
		// handle equal types or any
		else if (match_type(astnode->type, current_expression.type) || is_any(astnode->type)) {
			current_scope->declare_variable(get_namespace() + astnode->identifier, astnode->type, astnode->is_const, &decl_current_expr, astnode->row, astnode->col);
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

void SemanticAnalyser::visit(ASTAssignmentNode* astnode) {
	auto& actual_identifier = astnode->identifier_vector[0];
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::Util::join(astnode->identifier_vector, ".");
	}

	if (astnode->op != "=" && astnode->op != "+=" && astnode->op != "-=" && astnode->op != "*=" && astnode->op != "/=" && astnode->op != "%=") {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid assignment operator '" + astnode->op + "'");
	}

	// determine the inner-most scope in which the value is declared
	long long i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(/*!functions.empty() ? astnode->identifier_vector[0] : */actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier +
				"' being reassigned was never declared");
		}
	}

	// get base variable
	auto declared_variable = scopes[i]->find_declared_variable(astnode->identifier_vector[0]);
	Type type;
	Type any_type;

	// check if the base variable is not null
	if (is_void(declared_variable->type)/* && !declared_variable->is_parameter*/) {
		auto dectype = is_void(declared_variable->type) ? "null" : "undefined";
		// struct
		if (astnode->identifier_vector.size() > 1) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying assign '" + actual_identifier +
				"' but '" + astnode->identifier_vector[0] + "' is " + dectype);
		}
		// array
		if (astnode->access_vector.size() > 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying assign '" + actual_identifier + "' array position but it's " + dectype);
		}
	}

	// visit the expression to update current type
	astnode->expr->accept(this);

	// evaluate array access vector
	evaluate_access_vector(astnode->access_vector);

	// get the type of the originally declared variable
	if (declared_variable->is_parameter) {
		declared_variable = find_declared_variable_recursively(actual_identifier);
	}
	// assign if it has or not a value
	else {
		scopes[i]->change_current_variable_type(actual_identifier, current_expression.type);
		scopes[i]->change_variable_type_name(actual_identifier, current_expression.type_name);
		declared_variable = scopes[i]->find_declared_variable(actual_identifier);
	}

	if (declared_variable->is_const) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + actual_identifier + "' constant being reassigned");
	}

	type = declared_variable->type;
	any_type = declared_variable->expr->type;

	if (astnode->op == "+=" || astnode->op == "-=" || astnode->op == "*=" || astnode->op == "/=") {
		if (!is_int(current_expression.type) && !is_float(current_expression.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid right value type '" +
				type_str(current_expression.array_type) + "' to execute '" + astnode->op + "' operation");
		}
		if (!is_int(any_type) && !is_float(current_expression.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid left value type '" +
				type_str(any_type) + "' to execute '" + astnode->op + "' operation");
		}
	}
	else if (astnode->op == "%=") {
		if (!is_int(current_expression.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid right value type '" +
				type_str(current_expression.array_type) + "' to execute '" + astnode->op + "' operation");
		}
		if (!is_int(any_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid left value type '" +
				type_str(any_type) + "' to execute '" + astnode->op + "' operation");
		}
	}

	if (is_array(any_type)) {
		if (is_array(current_expression.type)) {

			std::vector<unsigned int> expr_dim;
			std::vector<unsigned int> pars_dim = evaluate_access_vector(declared_variable->expr->dim);;

			if (auto arr_expr = dynamic_cast<ASTArrayConstructorNode*>(astnode->expr)) {
				determine_array_type(arr_expr);
				expr_dim = calculate_array_dim_size(arr_expr);
			}
			else if (auto id_expr = dynamic_cast<ASTIdentifierNode*>(astnode->expr)) {
				auto expr_variable = find_declared_variable_recursively(id_expr->identifier_vector[0]);
				expr_dim = evaluate_access_vector(expr_variable->expr->dim);
				current_expression.array_type = expr_variable->expr->array_type;
			}
			else {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid assignment of '" + actual_identifier + "' array");
			}

			if (declared_variable->expr->dim.size() != expr_dim.size()) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier_vector[0] + "'");
			}

			for (size_t dc = 0; dc < declared_variable->expr->dim.size(); ++dc) {
				if (declared_variable->expr->dim.at(dc) && pars_dim.at(dc) != expr_dim.at(dc)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array size assigning '" + astnode->identifier_vector[0] + "'");
				}
			}
		}

		if (astnode->access_vector.size() == 0 && !is_any(declared_variable->expr->array_type) && !match_type(declared_variable->expr->array_type, current_expression.array_type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier +
				"', expected '" + type_str(declared_variable->expr->array_type) + "' array, found '" + type_str(current_expression.array_type) + "' array");
		}
	}

	if (astnode->access_vector.size() > 0) {
		type = declared_variable->expr->array_type;
	}

	// allow mismatched type in the case of declaration of int to real
	if (is_float(type) && is_int(current_expression.type)) {
	}
	else if (is_any(type)) {
		scopes[i]->change_current_variable_type(actual_identifier, current_expression.type);
		scopes[i]->change_variable_type_name(actual_identifier, current_expression.type_name);
		// TODO: VERIFICAR SE VAI PRECISAR TOMAR ALGUMA AÇÃO PARA STRUCT
	}
	else if (is_struct(type) && (is_struct(current_expression.type) || is_void(current_expression.type))) {
		auto& type_name = declared_variable->expr->type_name;
		if (current_expression.type_name != type_name && !is_void(current_expression.type)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier +
				"' struct, expected '" + type_name + "', found '" + current_expression.type_name + "'");
		}

		if (auto str_expr = dynamic_cast<ASTStructConstructorNode*>(astnode->expr)) {
			assign_structure(scopes[i], declared_variable->expr, str_expr);
		}
		// TODO: VERIFICAR SE VAI PRECISAR TOMAR ALGUMA AÇÃO PARA STRUCT
	}
	// otherwise throw error
	else if (!match_type(current_expression.type, type) && !is_any(type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier +
			"', expected " + type_str(type) + ", found " + type_str(current_expression.type) + "");
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

void SemanticAnalyser::assign_structure(SemanticScope* curr_scope, parser::SemanticExpression_t* expression, ASTStructConstructorNode* expr) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(expr->type_name); --i);
	auto struct_def = scopes[i]->find_declared_structure_definition(expr->type_name);

	for (auto const& expr_value : expr->values) {

		VariableDefinition_t str_var_def;
		auto found = false;
		for (size_t i = 0; i < struct_def.variables.size(); ++i) {
			str_var_def = struct_def.variables[i];
			if (str_var_def.identifier == expr_value.first) {
				found = true;
				break;
			}
		}
		expr_value.second->accept(this);

		if (!found) {
			throw std::runtime_error(msg_header(expr->row, expr->col) + "'" + expr_value.first + "' is not a member of '" + expression->type_name + "'");
		}

		auto new_var = new SemanticExpression_t(str_var_def.type, str_var_def.array_type, str_var_def.dim, str_var_def.type_name,
			std::map<std::string, SemanticExpression*>(), expr_value.second, false, expr->row, expr->col);

		if (is_struct(str_var_def.type)) {
			// TODO: CHECK TYPES

			if (auto str_expr = dynamic_cast<ASTStructConstructorNode*>(expr_value.second)) {
				assign_structure(curr_scope, new_var, str_expr);
			}
			else {
				throw std::runtime_error(msg_header(expr->row, expr->col) + "expected struct constructor");
			}

		}

		expression->struct_vars[str_var_def.identifier] = new_var;
	}
}

SemanticVariable_t* SemanticAnalyser::find_declared_variable_recursively(std::string identifier) {
	long long i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(identifier); --i) {
		if (i <= 0) {
			i = -1;
			break;
		}
	}
	if (i >= 0) {
		return scopes[i]->find_declared_variable(identifier);
	}

	if (axe::Util::contains(identifier, ".")) {
		auto identifiers = axe::Util::split_list(identifier, '.');
		StructureDefinition_t str_def;
		std::string type_name = "";

		for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(identifiers.front()); --i) {
			if (i <= 0) {
				i = -1;
				break;
			}
		}
		if (i >= 0) {
			type_name = scopes[i]->find_declared_variable(identifiers.front())->expr->type_name;
		}

		size_t i;
		for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(type_name); --i);
		str_def = scopes[i]->find_declared_structure_definition(type_name);

		identifiers.pop_front();

		while (identifiers.size() > 1) {
			type_name = "";

			for (auto variable : str_def.variables) {
				if (variable.identifier == identifiers.front()) {
					type_name = variable.type_name;
					break;
				}
			}

			size_t i;
			for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(type_name); --i);
			str_def = scopes[i]->find_declared_structure_definition(type_name);

			identifiers.pop_front();
		}

		for (size_t vari = 0; vari < str_def.variables.size(); ++vari) {
			if (str_def.variables.at(vari).identifier == identifiers.front()) {
				// TODO: REMEMBER WHAT RETURN HERE
				//return str_def.variables.at(vari);
			}
		}
	}

	throw std::runtime_error(msg_header(0, 0) + "error: '" + identifier + "' variable not found");
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
	if (!functions.empty()) {

		auto true_type = is_array(current_function.any_type) ? current_function.array_type : current_function.any_type;

		if (!is_any(true_type) && !is_void(current_expression.type)
			&& current_expression.type != true_type) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' function return type");
		}
		if (is_array(true_type)) {

			std::vector<unsigned int> expr_dim;
			std::vector<unsigned int> pars_dim = evaluate_access_vector(current_function.dim);

			if (auto arr_expr = dynamic_cast<ASTArrayConstructorNode*>(astnode->expr)) {
				determine_array_type(arr_expr);
				expr_dim = calculate_array_dim_size(arr_expr);
			}
			else if (auto id_expr = dynamic_cast<ASTIdentifierNode*>(astnode->expr)) {
				auto expr_variable = find_declared_variable_recursively(id_expr->identifier_vector[0]);
				expr_dim = evaluate_access_vector(expr_variable->expr->dim);
				current_expression.array_type = expr_variable->expr->array_type;
			}
			else {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' function return expression");
			}

			if (current_function.array_type != Type::T_ANY && current_function.array_type != current_expression.array_type) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' function array type return");
			}

			if (current_function.dim.size() != expr_dim.size()) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' array dimension return");
			}

			for (size_t dc = 0; dc < current_function.dim.size(); ++dc) {
				if (current_function.dim.at(dc) && pars_dim.at(dc) != expr_dim.at(dc)) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' array size return");
				}
			}
		}
		if (is_struct(true_type)) {
			if (current_function.type_name != current_expression.type_name) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + current_function.identifier + "' function struct type return");
			}
		}

	}
}

void SemanticAnalyser::visit(ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<Type> signature;

	// for each parameter,
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(current_expression.type);
	}

	// make sure the function exists in some scope i
	long long i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_function(astnode->identifier, signature); i--) {
		if (i <= 0) {
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
	}

	current_function = scopes[i]->find_declared_function(astnode->identifier, signature);

	// push current function type into function stack
	functions.push(current_function.any_type);

	// check semantics of function block by visiting nodes
	current_function.block->accept(this);

	// end the current function
	functions.pop();

	// set current expression type to the return value of the function
	//current_expression.type = current_function.any_type;
	//current_expression.array_type = current_function.array_type;
	//current_expression.type_name = current_function.type_name;
	//current_expression.is_constant = false;
}

void SemanticAnalyser::visit(ASTFunctionDefinitionNode* astnode) {
	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
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

	// add function to symbol table
	scopes.back()->declare_function(get_namespace() + astnode->identifier, type, astnode->type_name, type,
		astnode->array_type, astnode->dim, astnode->signature, astnode->parameters, astnode->block, astnode->row, astnode->row);

	if (!is_void(type)) {
		// check that the function body returns
		if (!has_return) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "defined function '" + astnode->identifier + "' is not guaranteed to return a value");
		}
	}
}

void SemanticAnalyser::visit(ASTBlockNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

	if (!functions.empty()) {
		// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
		for (auto param : current_function.parameters) {
			// TODO: need to declare structs
			//scopes.back()->declare_variable(param.identifier, param.type, param.type_name, param.type, param.array_type,
			//	param.dim, nullptr, std::map<std::string, SemanticVariable_t*>(), param.row, param.col, true);
		}
	}

	// visit each statement in the block
	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(ASTContinueNode* astnode) { }

void SemanticAnalyser::visit(ASTBreakNode* astnode) { }

void SemanticAnalyser::visit(ASTSwitchNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

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
	scopes.pop_back();
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
	scopes.push_back(new SemanticScope());

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
	scopes.pop_back();
}

void SemanticAnalyser::visit(ASTForEachNode* astnode) {
	Type decl_type;
	Type col_type;

	scopes.push_back(new SemanticScope());

	astnode->itdecl->accept(this);
	decl_type = current_expression.type;

	astnode->collection->accept(this);
	col_type = current_expression.array_type;

	if (is_any(decl_type)) {
		decl_type = col_type;
		auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl);

		int i;
		for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(idnode->identifier); i--);

		auto declared_variable = scopes[i]->find_declared_variable(idnode->identifier);

		scopes[i]->change_current_variable_type(idnode->identifier, col_type);
		scopes[i]->change_variable_type_name(idnode->identifier, current_expression.type_name);
	}

	if (!is_array(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array in foreach");
	}

	if (!match_type(decl_type, col_type) && !is_any(decl_type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched types");
	}

	astnode->block->accept(this);

	// close scope
	scopes.pop_back();
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
	for (auto& scope : scopes) {
		if (scope->already_declared_structure_definition(astnode->identifier)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "struct " + astnode->identifier + " already defined");
		}
	}

	scopes.back()->declare_structure_definition(get_namespace() + astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_bool>*) {
	current_expression.type = Type::T_BOOL;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_int>*) {
	current_expression.type = Type::T_INT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_float>*) {
	current_expression.type = Type::T_FLOAT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_char>*) {
	current_expression.type = Type::T_CHAR;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_string>*) {
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTArrayConstructorNode* astnode) {
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
	}
	determine_array_type(astnode);
	current_expression.type = Type::T_ARRAY;
	current_expression.type_name = "";
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTStructConstructorNode* astnode) {
	long long i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(astnode->type_name); --i) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "structure '" + astnode->type_name +
				"' was never declared");
		}
	}

	auto type_struct = scopes[i]->find_declared_structure_definition(astnode->type_name);

	current_expression.struct_vars = std::map<std::string, SemanticExpression_t*>();

	for (auto& expr : astnode->values) {
		bool found = false;
		VariableDefinition_t var_type_struct;
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

	current_expression.type = Type::T_STRUCT;
	current_expression.type_name = astnode->type_name;
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;

	declare_structure();
}

void SemanticAnalyser::declare_structure() {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(current_expression.type_name); --i);
	auto type_struct = scopes[i]->find_declared_structure_definition(current_expression.type_name);

	for (auto var_type_struct : type_struct.variables) {

		try {
			current_expression.struct_vars[var_type_struct.identifier];
		}
		catch (...) {
			auto new_var = new SemanticExpression_t(var_type_struct.type, var_type_struct.array_type, var_type_struct.dim, var_type_struct.type_name,
				std::map<std::string, SemanticExpression*>(), nullptr, false, 0, 0);

			current_expression.struct_vars[var_type_struct.identifier] = new_var;
		}
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
		if ((is_string(l_type) || is_char(l_type)) && (is_string(r_type) || is_char(r_type))) { // If both string, no error
			current_expression.type = is_char(l_type) && is_char(r_type) ? Type::T_CHAR : Type::T_STRING;
		}
		else if (is_string(l_type) || is_char(l_type) || is_string(r_type) || is_char(r_type)) { // only one is string, error
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched operands for '+' operator, found " + type_str(l_type) + " on the left, but " + type_str(r_type) + " on the right");
		}
		else { // real/int possibilities remain. If both int, then result is int, otherwise result is real
			current_expression.type = (is_int(l_type) && is_int(r_type)) ? Type::T_INT : Type::T_FLOAT;
		}
	}
	else if (op == "and" || op == "or") {
		// and/or only work for bool and struct
		if ((is_bool(l_type) || is_struct(l_type)) && (is_bool(r_type) || is_struct(r_type))) {
			current_expression.type = Type::T_BOOL;
		}
		else if (is_bool(l_type) || is_struct(l_type) || is_bool(r_type) || is_struct(r_type)) { // only one is bool, error
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched operands for '+' operator, found " + type_str(l_type) + " on the left, but " + type_str(r_type) + " on the right");
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
		if (!match_type(l_type, r_type) && (!is_float(l_type) || !is_int(r_type)) && (!is_int(l_type) || !is_float(r_type))) {
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
	std::string actual_identifier = astnode->identifier_vector[0];
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::Util::join(astnode->identifier_vector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(!functions.empty() ? astnode->identifier_vector[0] : actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier + "' was never declared");
		}
	}

	SemanticVariable_t* declared_variable;

	// update current expression type
	if (!functions.empty()) {
		declared_variable = find_declared_variable_recursively(actual_identifier);
	}
	else {
		declared_variable = scopes[i]->find_declared_variable(actual_identifier);
	}

	current_expression.type = declared_variable->expr->type;
	current_expression.type_name = declared_variable->expr->type_name;
	current_expression.array_type = declared_variable->expr->array_type;
	current_expression.is_const = declared_variable->expr->is_const;

	if (astnode->access_vector.size() > 0 && !is_array(current_expression.type) && !is_string(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + actual_identifier + "' is not an array or string");
	}

	if (astnode->access_vector.size() > 0 && is_array(current_expression.type)) {
		std::vector<unsigned int> expr_dim;
		std::vector<unsigned int> pars_dim = evaluate_access_vector(astnode->access_vector);

		auto expr_variable = find_declared_variable_recursively(astnode->identifier_vector[0]);
		expr_dim = evaluate_access_vector(expr_variable->expr->dim);
		current_expression.array_type = expr_variable->expr->array_type;

		if (astnode->access_vector.size() != expr_dim.size()) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + astnode->identifier_vector[0] + "' array dimension access");
		}

		for (size_t dc = 0; dc < astnode->access_vector.size(); ++dc) {
			if (astnode->access_vector.at(dc) && pars_dim.at(dc) > expr_dim.at(dc)) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid '" + astnode->identifier_vector[0] + "' array size access");
			}
		}
	}
}

void SemanticAnalyser::visit(ASTUnaryExprNode* astnode) {
	current_expression.is_const = false;

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
	case Type::T_STRUCT:
		if (astnode->unary_op != "not") {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "operator '" + astnode->unary_op + "' in front of boolean expression");
		}
		break;
	default:
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "incompatible unary operator '" + astnode->unary_op + "' in front of expression");
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

	current_expression.type = astnode->type;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTReadNode* astnode) {
	if (!is_string(current_expression.type)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying to assing an invalid type");
	}
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTTypeNode* astnode) {
	astnode->expr->accept(this);
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

	current_expression.type = Type::T_FLOAT;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTNullNode* astnode) {
	current_expression.type = Type::T_VOID;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTThisNode* astnode) {
	current_expression.type = Type::T_STRING;
	current_expression.type_name = "";
	current_expression.array_type = Type::T_UNDEF;
	current_expression.is_const = false;
}

std::string SemanticAnalyser::msg_header(unsigned int row, unsigned int col) {
	return "(SERR) " + current_program->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
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
	auto actual_identifier = astnode->identifier_vector[0];
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::Util::join(astnode->identifier_vector, ".");
	}

	// determine the inner-most scope in which the value is declared
	int i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(!functions.empty() ? astnode->identifier_vector[0] : actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier + "' was not declared");
		}
	}

	// get base variable
	auto declared_variable = scopes[i]->find_declared_variable(astnode->identifier_vector[0]);

	switch (declared_variable->type) {
	case Type::T_BOOL:
		return static_cast<ASTLiteralNode<cp_bool>*>(declared_variable->expr->expr)->hash(this);
	case Type::T_INT:
		return static_cast<ASTLiteralNode<cp_int>*>(declared_variable->expr->expr)->hash(this);
	case Type::T_FLOAT:
		return static_cast<ASTLiteralNode<cp_float>*>(declared_variable->expr->expr)->hash(this);
	case Type::T_CHAR:
		return static_cast<ASTLiteralNode<cp_char>*>(declared_variable->expr->expr)->hash(this);
	case Type::T_STRING:
		return static_cast<ASTLiteralNode<cp_string>*>(declared_variable->expr->expr)->hash(this);
	default:
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid type");
	}
}
