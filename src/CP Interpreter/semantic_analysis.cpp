#include <utility>
#include <iostream>

#include "semantic_analysis.hpp"
#include "util.hpp"


using namespace visitor;


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), current_program(programs[0]), current_expression_type(parser::Type::T_ND), current_expression_array_type(parser::Type::T_ND),
	is_function_definition_context(false), current_expression_is_constant(false), current_expression_has_value(false) {
	// add global scope
	scopes.push_back(global_scope);
};

SemanticAnalyser::~SemanticAnalyser() = default;

void SemanticAnalyser::start() {
	visit(current_program);
}

void SemanticAnalyser::visit(parser::ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTUsingNode* astnode) {
	for (auto& program : programs) {
		if (astnode->library == program->name) {
			auto prev_program = current_program;
			current_program = program;
			start();
			current_program = prev_program;
		}
	}
}

void SemanticAnalyser::visit(parser::ASTDeclarationNode* astnode) {
	// current scope is the scope at the back
	SemanticScope* current_scope = scopes.back();

	// if variable already declared, throw error
	if (current_scope->already_declared_variable(astnode->identifier)) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "variable '" + astnode->identifier + "' already declared");
	}

	bool has_value = false;

	// visit the expression to update current type
	if (astnode->expr) {
		astnode->expr->accept(this);
		has_value = current_expression_has_value;
	}
	else {
		current_expression_type = astnode->type;
		current_expression_array_type = astnode->array_type;
		current_expression_type_name = astnode->type_name;
		current_expression_has_value = has_value;
	}

	if (astnode->is_const && !current_expression_is_constant) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "initializer of '" + astnode->identifier + "' is not a constant");
	}

	// similar types
	if (astnode->type == parser::Type::T_FLOAT && current_expression_type == parser::Type::T_INT
		|| astnode->type == parser::Type::T_STRING && current_expression_type == parser::Type::T_CHAR) {
		current_scope->declare_variable(astnode->identifier, astnode->type, astnode->type_name, astnode->array_type, astnode->dim, astnode->expr, astnode->is_const, has_value, astnode->row, astnode->col, false);
	}
	// equal types and special types (any, struct and array)
	else if (astnode->type == current_expression_type || astnode->type == parser::Type::T_ANY || astnode->type == parser::Type::T_STRUCT || astnode->type == parser::Type::T_ARRAY) { // types match
		// handle struct
		if (astnode->type == parser::Type::T_STRUCT || current_expression_type == parser::Type::T_STRUCT) {
			if (current_expression_type == parser::Type::T_STRUCT && astnode->type != parser::Type::T_STRUCT && astnode->type != parser::Type::T_ANY) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + parser::type_str(astnode->type) + ")' with type '" + current_expression_type_name + "'");
			}
			if (current_expression_type == parser::Type::T_STRUCT && astnode->type == parser::Type::T_STRUCT && astnode->type_name != current_expression_type_name) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + astnode->type_name + ")' with type '" + current_expression_type_name + "'");
			}

			parser::ASTStructConstructorNode* str_expr = nullptr;

			// has expression to declare
			if (astnode->expr) {
				if (current_expression_type != parser::Type::T_STRUCT && current_expression_type != parser::Type::T_NULL) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "found " + parser::type_str(current_expression_type) + (current_expression_type_name.empty() ? "" : " (" + current_expression_type_name + ")") + " in definition of '" + astnode->identifier + "', expected " + parser::type_str(astnode->type) + (astnode->type_name.empty() ? "" : " (" + astnode->type_name + ")") + "");
				}
				// TODO: need more tests
				if (current_expression_type == parser::Type::T_STRUCT) {
					str_expr = static_cast<parser::ASTStructConstructorNode*>(astnode->expr);
				}
			}

			auto str_type = astnode->type;
			auto str_type_name = astnode->type_name;
			// check if is any var
			if (astnode->type == parser::Type::T_ANY) {
				str_type = current_expression_type;
				str_type_name = current_expression_type_name;
			}

			current_scope->declare_variable(astnode->identifier, str_type, str_type_name, astnode->array_type, astnode->dim, astnode->expr, astnode->is_const, has_value, astnode->row, astnode->col, false);
			if (str_expr && !is_function_definition_context) {
				declare_structure_definition_variables(astnode->identifier, str_expr);
			}
			declare_structure_definition_first_level_variables(astnode->identifier, str_type_name);
		}
		else if (astnode->type == parser::Type::T_ARRAY || current_expression_type == parser::Type::T_ARRAY) {
			if (astnode->type != parser::Type::T_ANY && astnode->type != parser::Type::T_ARRAY && current_expression_type == parser::Type::T_ARRAY) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + parser::type_str(astnode->type) + ")' with type '" + parser::type_str(current_expression_type) + "'");
			}

			if (astnode->expr) {
				if (current_expression_type != parser::Type::T_ARRAY && current_expression_type != parser::Type::T_NULL) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array expression assigning '" + astnode->identifier + "'");
				}

				// TODO: need more tests
				parser::ASTArrayConstructorNode* arr = dynamic_cast<parser::ASTArrayConstructorNode*>(astnode->expr);
				if (arr) {
					determine_array_type(arr);

					auto expr_dim = calculate_array_dim_size(arr);

					if (astnode->dim.size() != expr_dim.size()) {
						throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier + "'");
					}
				}

			}

			current_expression_array_type = current_expression_array_type == parser::Type::T_NULL ? parser::Type::T_ANY : current_expression_array_type;

			current_scope->declare_variable(astnode->identifier, astnode->type, astnode->type_name, current_expression_array_type, astnode->dim, astnode->expr, astnode->is_const, has_value, astnode->row, astnode->col, false);
		}
		else if (astnode->type == current_expression_type || astnode->type == parser::Type::T_ANY) {
			current_scope->declare_variable(astnode->identifier, current_expression_type, astnode->type_name, astnode->array_type, astnode->dim, astnode->expr, astnode->is_const, has_value, astnode->row, astnode->col, false);
		}
		else {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "found " + parser::type_str(current_expression_type) + " defining '" + astnode->identifier + "', expected " + parser::type_str(astnode->type) + "");
		}
	}
	else if (current_expression_type == parser::Type::T_NULL) {
		current_scope->declare_variable(astnode->identifier, astnode->type, astnode->type_name, astnode->array_type, astnode->dim, astnode->expr, astnode->is_const, has_value, astnode->row, astnode->col, false);
	}
	else { // types don't match
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "found " + parser::type_str(current_expression_type) + " in definition of '" + astnode->identifier + "', expected " + parser::type_str(astnode->type) + "");
	}
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* astnode) {
	std::string actual_identifier = astnode->identifier;
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::join(astnode->identifier_vector, ".");
	}

	// determine the inner-most scope in which the value is declared
	int i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(is_function_definition_context ? astnode->identifier : actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier + "' being reassigned was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// get base variable
	parser::VariableDefinition_t declared_variable = scopes[i]->find_declared_variable(astnode->identifier);
	parser::Type type;

	// check if the base variable is not null
	if (!declared_variable.has_value && !declared_variable.is_parameter) {
		// struct
		if (astnode->identifier_vector.size() > 1) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying assign '" + actual_identifier + "' but '" + astnode->identifier + "' is null");
		}
		// array
		if (astnode->access_vector.size() > 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying assign '" + actual_identifier + "' array position but it's null");
		}
	}

	// visit the expression to update current type
	astnode->expr->accept(this);

	// evaluate array access vector
	evaluate_access_vector(astnode->access_vector);

	// get the type of the originally declared variable
	if (declared_variable.is_parameter) {
		declared_variable = find_declared_variable_recursively(actual_identifier);
	}
	// assign if it has or not a value
	else {
		scopes[i]->assign_variable(actual_identifier, current_expression_type != parser::Type::T_NULL);
		declared_variable = scopes[i]->find_declared_variable(actual_identifier);
	}

	if (declared_variable.is_const) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + actual_identifier + "' constant being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
	}

	type = declared_variable.type;

	if (type == parser::Type::T_ARRAY) {
		if (current_expression_type == parser::Type::T_ARRAY) {

			try {
				parser::ASTArrayConstructorNode* arrExpr = dynamic_cast<parser::ASTArrayConstructorNode*>(astnode->expr);
				std::vector<unsigned int> exprDim;

				if (arrExpr) {
					determine_array_type(arrExpr);
					exprDim = calculate_array_dim_size(arrExpr);
				}
				else {
					parser::ASTIdentifierNode* idExpr = dynamic_cast<parser::ASTIdentifierNode*>(astnode->expr);
					auto exprVariable = find_declared_variable_recursively(idExpr->identifier);
					exprDim = evaluate_access_vector(exprVariable.dim);;
					current_expression_array_type = exprVariable.array_type;
				}

				if (declared_variable.dim.size() != exprDim.size()) {
					throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier + "'");
				}
			}
			catch (...) {
				throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid assignment of '" + actual_identifier + "' array");
			}
		}

		if (astnode->access_vector.size() == 0 && declared_variable.array_type != parser::Type::T_ANY && declared_variable.array_type != current_expression_array_type) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier + "', expected '" + parser::type_str(declared_variable.array_type) + "' array, found '" + parser::type_str(current_expression_array_type) + "' array");
		}
	}

	if (astnode->access_vector.size() > 0) {
		type = declared_variable.array_type;
	}

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::Type::T_FLOAT && current_expression_type == parser::Type::T_INT ||
		type == parser::Type::T_INT && current_expression_type == parser::Type::T_FLOAT) {
	}
	else if (type == parser::Type::T_ANY) {
		scopes[i]->change_variable_type(actual_identifier, current_expression_type);
		scopes[i]->change_variable_type_name(actual_identifier, current_expression_type_name);
	}
	else if (type == parser::Type::T_STRUCT && (current_expression_type == parser::Type::T_STRUCT || current_expression_type == parser::Type::T_NULL)) {
		std::string type_name = declared_variable.type_name;
		if (current_expression_type_name != type_name && current_expression_type != parser::Type::T_NULL) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier + "' struct, expected '" + type_name + "', found '" + current_expression_type_name + "'");
		}

		if (typeid(astnode->expr) == typeid(parser::ASTStructConstructorNode*)) {
			parser::ASTStructConstructorNode* str_expr = static_cast<parser::ASTStructConstructorNode*>(astnode->expr);

			if (str_expr) {
				declare_structure_definition_variables(actual_identifier, str_expr);
			}
		}
		else {
			declare_structure_definition_first_level_variables(actual_identifier, type_name);
		}

	}
	// otherwise throw error
	else if (current_expression_type != type && type != parser::Type::T_ANY && type != parser::Type::T_NULL) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched type for '" + actual_identifier + "', expected " + parser::type_str(type) + ", found " + parser::type_str(current_expression_type) + "");
	}
}

void SemanticAnalyser::determine_array_type(parser::ASTArrayConstructorNode* astnode) {
	auto aux_curr_type = current_expression_type;
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);

		if (current_expression_type == parser::Type::T_ARRAY) {
			determine_array_type(static_cast<parser::ASTArrayConstructorNode*>(astnode->values.at(i)));
		}
		else {
			check_array_type(astnode->values.at(i), astnode->row, astnode->col);
		}
	}
	current_expression_type = aux_curr_type;
}

void SemanticAnalyser::check_array_type(parser::ASTExprNode* astnode, unsigned int row, unsigned int col) {
	auto aux_curr_type = current_expression_type;
	astnode->accept(this);

	if (current_expression_array_type == parser::Type::T_ANY || current_expression_array_type == parser::Type::T_ND || current_expression_array_type == parser::Type::T_NULL || current_expression_array_type == parser::Type::T_VOID) {
		current_expression_array_type = current_expression_type;
	}
	if (current_expression_array_type != current_expression_type) {
		throw std::runtime_error(msg_header(row, col) + "mismatched type in array definition");
	}
	current_expression_type = aux_curr_type;
}

std::vector<unsigned int> SemanticAnalyser::calculate_array_dim_size(parser::ASTArrayConstructorNode* arr) {
	auto dim = std::vector<unsigned int>();

	dim.push_back(arr->values.size());

	parser::ASTArrayConstructorNode* sub_arr = dynamic_cast<parser::ASTArrayConstructorNode*>(arr->values.at(0));
	if (sub_arr) {
		auto dim2 = calculate_array_dim_size(sub_arr);
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

void SemanticAnalyser::declare_structure_definition_first_level_variables(std::string identifier, std::string type_name) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(type_name); --i);
	auto type_struct = scopes[i]->find_declared_structure_definition(type_name);

	for (auto var_type_struct : type_struct.variables) {
		auto current_identifier = identifier + '.' + var_type_struct.identifier;
		scopes.back()->declare_variable(current_identifier, var_type_struct.type, var_type_struct.type_name, var_type_struct.array_type, var_type_struct.dim, nullptr, var_type_struct.is_const, false, var_type_struct.row, var_type_struct.col, false);
	}
}

void SemanticAnalyser::declare_structure_definition_variables(std::string identifier, parser::ASTStructConstructorNode* expr) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_structure_definition(expr->type_name); --i);
	auto type_struct = scopes[i]->find_declared_structure_definition(expr->type_name);

	for (auto const& str_value : expr->values) {
		auto current_identifier = identifier + '.' + str_value.first;

		bool found = false;
		parser::VariableDefinition_t* var_type_struct = nullptr;
		for (size_t i = 0; i < type_struct.variables.size(); ++i) {
			var_type_struct = &type_struct.variables[i];
			if (var_type_struct->identifier == str_value.first) {
				found = true;
				break;
			}
		}
		if (!found) throw std::runtime_error(msg_header(expr->row, expr->col) + "'" + str_value.first + "' is not a member of '" + expr->type_name + "'");

		str_value.second->accept(this);
		bool has_value = current_expression_type != parser::Type::T_NULL;

		if (var_type_struct->type != parser::Type::T_ANY && current_expression_type != parser::Type::T_NULL && var_type_struct->type != current_expression_type) {
			throw std::runtime_error(msg_header(expr->row, expr->col) + "invalid type " + parser::type_str(var_type_struct->type) + " trying to assign '" + current_identifier + "'");
		}

		if (var_type_struct->type == parser::Type::T_STRUCT) {
			scopes.back()->declare_variable(current_identifier, var_type_struct->type, var_type_struct->identifier, parser::Type::T_ND, var_type_struct->dim, nullptr, false, has_value, expr->row, expr->col, false);

			parser::ASTStructConstructorNode* str_expr = nullptr;

			if (typeid(parser::ASTStructConstructorNode*) == typeid(str_value.second)) {
				str_expr = static_cast<parser::ASTStructConstructorNode*>(str_value.second);
			}

			if (str_expr) {
				declare_structure_definition_variables(current_identifier, str_expr);
			}
		}
		else {
			scopes.back()->declare_variable(current_identifier, var_type_struct->type, var_type_struct->type_name, var_type_struct->array_type, var_type_struct->dim, nullptr, false, has_value, expr->row, expr->col, false);
		}
	}
}

parser::VariableDefinition_t SemanticAnalyser::find_declared_variable_recursively(std::string identifier) {
	int i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(identifier); --i) {
		if (i <= 0) {
			i = -1;
			break;
		}
	}
	if (i >= 0) {
		return scopes[i]->find_declared_variable(identifier);
	}

	if (axe::contains(identifier, ".")) {
		std::list<std::string> identifiers = axe::split_list(identifier, '.');
		parser::StructureDefinition_t str_def;
		std::string type_name = "";

		for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(identifiers.front()); --i) {
			if (i <= 0) {
				i = -1;
				break;
			}
		}
		if (i >= 0) {
			type_name = scopes[i]->find_declared_variable(identifiers.front()).type_name;
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
				return str_def.variables.at(vari);
			}
		}
	}

	throw std::runtime_error(msg_header(0, 0) + "error: '" + identifier + "' variable not found");
}

std::vector<unsigned int> SemanticAnalyser::evaluate_access_vector(std::vector<parser::ASTExprNode*> exprAcessVector) {
	auto access_vector = std::vector<unsigned int>();
	for (auto expr : exprAcessVector) {
		expr->accept(this);
		access_vector.push_back(0);
		if (current_expression_type != parser::Type::T_INT) {
			throw std::runtime_error(msg_header(0, 0) + "array index access must be a integer value");
		}
	}
	return access_vector;
}

void SemanticAnalyser::visit(parser::ASTPrintNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
}

void SemanticAnalyser::visit(parser::ASTReturnNode* astnode) {
	// update current expression
	astnode->expr->accept(this);

	// if we are not global, check that we return current function return type
	if (!functions.empty() && current_expression_type != functions.top()) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid return type, expected " + parser::type_str(functions.top()) + ", found " + parser::type_str(current_expression_type));
	}
}

void SemanticAnalyser::visit(parser::ASTBlockNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

	// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
	for (auto param : current_function_parameters) {
		scopes.back()->declare_variable(param.identifier, param.type, param.type_name, param.array_type, param.dim, nullptr, param.is_const, param.has_value, param.row, param.col, true);
	}

	// clear the global function parameters vector
	current_function_parameters.clear();

	// visit each statement in the block
	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTBreakNode* astnode) {

}

void SemanticAnalyser::visit(parser::ASTSwitchNode* astnode) {
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
		if (!current_expression_is_constant) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "error: expression is not an constant expression");
		}
	}

	// visit each statement in the block
	for (auto& stmt : *astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTElseIfNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the if block
	astnode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTIfNode* astnode) {
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

void SemanticAnalyser::visit(parser::ASTForNode* astnode) {
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

void SemanticAnalyser::visit(parser::ASTForEachNode* astnode) {
	parser::Type decl_type;
	parser::Type col_type;

	scopes.push_back(new SemanticScope());

	astnode->itdecl->accept(this);
	decl_type = current_expression_type;

	astnode->collection->accept(this);
	col_type = current_expression_array_type;

	if (current_expression_type != parser::Type::T_ARRAY) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected array in foreach");
	}

	if (decl_type != col_type && decl_type != parser::Type::T_ANY) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched types");
	}

	astnode->block->accept(this);

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTWhileNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the while block
	astnode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* astnode) {
	is_function_definition_context = true;

	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->already_declared_function(astnode->identifier, astnode->signature)) {
			std::string signature = "(";
			for (auto param : astnode->signature) {
				signature += parser::type_str(param) + ", ";
			}
			if (astnode->signature.size() > 0) {
				signature.pop_back();   // remove last whitespace
				signature.pop_back();   // remove last comma
			}
			signature += ")";


			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "error: function " + astnode->identifier + signature + " already defined");
		}
	}

	// add function to symbol table
	scopes.back()->declare_function(astnode->identifier, astnode->type, astnode->type_name, astnode->signature, astnode->type == parser::Type::T_ANY, astnode->row, astnode->row);

	// push current function type into function stack
	functions.push(astnode->type);

	// empty and update current function parameters vector
	current_function_parameters.clear();
	current_function_parameters = astnode->parameters;

	// check semantics of function block by visiting nodes
	astnode->block->accept(this);

	// check that the function body returns
	if (!returns(astnode->block) && astnode->type != parser::Type::T_VOID) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "defined function " + astnode->identifier + " is not guaranteed to return a value");
	}

	// end the current function
	functions.pop();

	is_function_definition_context = false;
}

void SemanticAnalyser::visit(parser::ASTStructDefinitionNode* astnode) {
	// first check that all enclosing scopes have not already defined the struct
	for (auto& scope : scopes) {
		if (scope->already_declared_structure_definition(astnode->identifier)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "error: struct " + astnode->identifier + " already defined");
		}
	}

	scopes.back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_bool>*) {
	current_expression_type = parser::Type::T_BOOL;
	current_expression_type_name = "";
	current_expression_is_constant = true;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_int>*) {
	current_expression_type = parser::Type::T_INT;
	current_expression_type_name = "";
	current_expression_is_constant = true;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_float>*) {
	current_expression_type = parser::Type::T_FLOAT;
	current_expression_type_name = "";
	current_expression_is_constant = true;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_char>*) {
	current_expression_type = parser::Type::T_CHAR;
	current_expression_type_name = "";
	current_expression_is_constant = true;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_string>*) {
	current_expression_type = parser::Type::T_STRING;
	current_expression_type_name = "";
	current_expression_is_constant = true;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTArrayConstructorNode* astnode) {
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
	}
	current_expression_type = parser::Type::T_ARRAY;
	current_expression_type_name = "";
	current_expression_is_constant = false;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTStructConstructorNode* astnode) {
	for (auto& expr : astnode->values) {
		expr.second->accept(this);
	}
	current_expression_type = parser::Type::T_STRUCT;
	current_expression_type_name = astnode->type_name;
	current_expression_is_constant = false;
	current_expression_has_value = true;
}

void SemanticAnalyser::visit(parser::ASTBinaryExprNode* astnode) {
	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	parser::Type l_type = current_expression_type;

	// then right node
	astnode->right->accept(this);
	parser::Type r_type = current_expression_type;

	// these only work for int/float
	if (op == "-" || op == "/" || op == "*" || op == "%") {
		if ((l_type != parser::Type::T_INT && l_type != parser::Type::T_FLOAT) || (r_type != parser::Type::T_INT && r_type != parser::Type::T_FLOAT)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected numerical operands for '" + op + "' operator.");
		}

		if (op == "%" && (l_type == parser::Type::T_FLOAT || r_type == parser::Type::T_FLOAT)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid operands to binary expression ('" + parser::type_str(l_type) + " and '" + parser::type_str(r_type) + "').");
		}

		// if both int, then expression is int, otherwise float
		current_expression_type = (l_type == parser::Type::T_INT && r_type == parser::Type::T_INT) ? parser::Type::T_INT : parser::Type::T_FLOAT;
	}
	else if (op == "+") {
		// + works for all types except bool
		if (l_type == parser::Type::T_BOOL || r_type == parser::Type::T_BOOL) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid operand for '+' operator, expected numerical or string operand.");
		}
		if ((l_type == parser::Type::T_STRING || l_type == parser::Type::T_CHAR) && (r_type == parser::Type::T_STRING || r_type == parser::Type::T_CHAR)) { // If both string, no error
			current_expression_type = (l_type == parser::Type::T_CHAR && r_type == parser::Type::T_CHAR) ? parser::Type::T_CHAR : parser::Type::T_STRING;
		}
		else if ((l_type == parser::Type::T_STRING || l_type == parser::Type::T_CHAR) || (r_type == parser::Type::T_STRING || r_type == parser::Type::T_CHAR)) { // only one is string, error
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "mismatched operands for '+' operator, found " + parser::type_str(l_type) + " on the left, but " + parser::type_str(r_type) + " on the right.");
		}
		else { // real/int possibilities remain. If both int, then result is int, otherwise result is real
			current_expression_type = (l_type == parser::Type::T_INT && r_type == parser::Type::T_INT) ? parser::Type::T_INT : parser::Type::T_FLOAT;
		}
	}
	else if (op == "and" || op == "or") {
		// and/or only work for bool
		if (l_type == parser::Type::T_BOOL && r_type == parser::Type::T_BOOL) {
			current_expression_type = parser::Type::T_BOOL;
		}
		else {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected two boolean-type operands for '" + op + "' operator.");
		}
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
		// rel-ops only work for numeric types
		if ((l_type != parser::Type::T_FLOAT && l_type != parser::Type::T_INT) || (r_type != parser::Type::T_FLOAT && r_type != parser::Type::T_INT)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected two numerical operands for '" + op + "' operator.");
		}
		current_expression_type = parser::Type::T_BOOL;
	}
	else if (op == "==" || op == "!=") {
		// == and != only work for like types
		if (l_type != r_type && (l_type != parser::Type::T_FLOAT || r_type != parser::Type::T_INT) && (l_type != parser::Type::T_INT || r_type != parser::Type::T_FLOAT)) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "expected arguments of the same type '" + op + "' operator.");
		}
		current_expression_type = parser::Type::T_BOOL;
	}
	else {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "unhandled semantic error in binary operator.");
	}
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTIdentifierNode* astnode) {
	std::string actual_identifier = astnode->identifier;
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::join(astnode->identifier_vector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(is_function_definition_context ? astnode->identifier : actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + "");
		}
	}

	parser::VariableDefinition_t declared_variable;

	// update current expression type
	if (is_function_definition_context) {
		declared_variable = find_declared_variable_recursively(actual_identifier);
	}
	else {
		declared_variable = scopes[i]->find_declared_variable(actual_identifier);
	}

	current_expression_type = declared_variable.type;
	current_expression_type_name = declared_variable.type_name;
	current_expression_array_type = declared_variable.array_type;
	current_expression_is_constant = declared_variable.is_const;
	current_expression_has_value = declared_variable.has_value;

	if (astnode->access_vector.size() > 0 && current_expression_type != parser::Type::T_ARRAY && current_expression_type != parser::Type::T_STRING) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "'" + actual_identifier + "' is not an array or string");
	}
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* astnode) {
	current_expression_is_constant = false;

	// determine expression type
	astnode->expr->accept(this);

	// handle different cases
	switch (current_expression_type) {
	case parser::Type::T_INT:
	case parser::Type::T_FLOAT:
		if (astnode->unary_op != "+" && astnode->unary_op != "-") {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "operator '" + astnode->unary_op + "' in front of numerical expression.");
		}
		break;
	case parser::Type::T_BOOL:
	case parser::Type::T_STRUCT:
		if (astnode->unary_op != "not") {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "operator '" + astnode->unary_op + "' in front of boolean expression.");
		}
		break;
	default:
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "incompatible unary operator '" + astnode->unary_op + "' in front of expression.");
	}
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<parser::Type> signature;

	// for each parameter,
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(current_expression_type);
	}

	// make sure the function exists in some scope i
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_function(astnode->identifier, signature); i--) {
		if (i <= 0) {
			std::string func_name = astnode->identifier + "(";
			for (auto param : signature) {
				func_name += parser::type_str(param) + ", ";
			}
			if (signature.size() > 0) {
				func_name.pop_back();   // remove last whitespace
				func_name.pop_back();   // remove last comma
			}
			func_name += ")";
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "function '" + func_name + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	auto declared_function = scopes[i]->find_declared_function(astnode->identifier, signature);

	// set current expression type to the return value of the function
	current_expression_type = declared_function.type;
	//current_expression_array_type = declared_function;
	current_expression_type_name = declared_function.type_name;
	current_expression_has_value = returns(astnode); // TODO: fix function return  has value
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTTypeParseNode* astnode) {
	astnode->expr->accept(this);
	current_expression_type = astnode->type;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTReadNode* astnode) {
	if (current_expression_type != parser::Type::T_STRING) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "trying to assing an invalid type");
	}
	current_expression_has_value = true;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTTypeNode* astnode) {
	astnode->expr->accept(this);
	current_expression_type = parser::Type::T_STRING;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTLenNode* astnode) {
	astnode->expr->accept(this);

	if (current_expression_type != parser::Type::T_ARRAY && current_expression_type != parser::Type::T_STRING) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "can't read len of type " + parser::type_str(current_expression_type));
	}

	current_expression_type = parser::Type::T_INT;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTRoundNode* astnode) {
	astnode->expr->accept(this);

	if (current_expression_type != parser::Type::T_FLOAT) {
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "can't round type " + parser::type_str(current_expression_type));
	}

	current_expression_type = parser::Type::T_FLOAT;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTNullNode* astnode) {
	current_expression_type = parser::Type::T_ND;
	current_expression_has_value = false;
	current_expression_is_constant = false;
}

void SemanticAnalyser::visit(parser::ASTThisNode* astnode) {
	current_expression_type = parser::Type::T_STRING;
	current_expression_has_value = true;
	current_expression_is_constant = false;
}

std::string SemanticAnalyser::msg_header(unsigned int row, unsigned int col) {
	return "(SERR) " + current_program->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
}

// determines whether a statement definitely returns or not
bool SemanticAnalyser::returns(parser::ASTNode* astnode) {
	// base case: if the statement is a return statement, then it definitely returns
	if (dynamic_cast<parser::ASTReturnNode*>(astnode)) {
		return true;
	}

	// for a block, if at least one statement returns, then the block returns
	if (auto block = dynamic_cast<parser::ASTBlockNode*>(astnode)) {
		for (auto& blk_stmt : block->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	// an if-(else) block returns only if both the if and the else statement return.
	if (auto ifstmt = dynamic_cast<parser::ASTIfNode*>(astnode)) {
		if (ifstmt->else_block) {
			return (returns(ifstmt->if_block) && returns(ifstmt->else_block));
		}
	}

	// a while block returns if its block returns
	if (auto whilestmt = dynamic_cast<parser::ASTWhileNode*>(astnode)) {
		return returns(whilestmt->block);
	}
	// other statements do not return
	else {
		return false;
	}
}

unsigned int SemanticAnalyser::hash(parser::ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(parser::ASTLiteralNode<cp_int>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(parser::ASTLiteralNode<cp_float>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(parser::ASTLiteralNode<cp_char>* astnode) {
	return static_cast<unsigned int>(astnode->val);
}

unsigned int SemanticAnalyser::hash(parser::ASTLiteralNode<cp_string>* astnode) {
	return axe::hashcode(astnode->val);
}

unsigned int SemanticAnalyser::hash(parser::ASTIdentifierNode* astnode) {
	std::string actual_identifier = astnode->identifier;
	if (astnode->identifier_vector.size() > 1) {
		actual_identifier = axe::join(astnode->identifier_vector, ".");
	}

	// determine the inner-most scope in which the value is declared
	int i;
	for (i = scopes.size() - 1; !scopes[i]->already_declared_variable(is_function_definition_context ? astnode->identifier : actual_identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msg_header(astnode->row, astnode->col) + "identifier '" + actual_identifier + "' was not declared");
		}
	}

	// get base variable
	parser::VariableDefinition_t declared_variable = scopes[i]->find_declared_variable(astnode->identifier);

	switch (declared_variable.type) {
	case parser::Type::T_BOOL: {
		auto value = static_cast<parser::ASTLiteralNode<cp_bool>*>(declared_variable.expr);
		return static_cast<unsigned int>(value->val);
	}
	case parser::Type::T_INT: {
		auto value = static_cast<parser::ASTLiteralNode<cp_int>*>(declared_variable.expr);
		return static_cast<unsigned int>(value->val);
	}
	case parser::Type::T_FLOAT: {
		auto value = static_cast<parser::ASTLiteralNode<cp_float>*>(declared_variable.expr);
		return static_cast<unsigned int>(value->val);
	}
	case parser::Type::T_CHAR: {
		auto value = static_cast<parser::ASTLiteralNode<cp_char>*>(declared_variable.expr);
		return static_cast<unsigned int>(value->val);
	}
	case parser::Type::T_STRING: {
		auto value = static_cast<parser::ASTLiteralNode<cp_string>*>(declared_variable.expr);
		return axe::hashcode(value->val);
	}
	default:
		throw std::runtime_error(msg_header(astnode->row, astnode->col) + "invalid type");
	}
}
