#include <utility>
#include <iostream>

#include "semantic_analysis.hpp"
#include "token.hpp"
#include "exception_handler.hpp"
#include "vendor/axeutils.hpp"
#include "graphics.hpp"


using namespace visitor;
using namespace parser;
using namespace lexer;


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	scopes[default_namespace].push_back(global_scope);
	register_built_in_functions();
};

void SemanticAnalyser::start() {
	visit(current_program);
}

void SemanticAnalyser::visit(ASTProgramNode* astnode) {
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			if (curr_row == 0 || curr_col == 0) {
				set_curr_pos(statement->row, statement->col);
			}
			if (exception) {
				throw std::runtime_error(ex.what());
			}
			exception = true;
			throw std::runtime_error(msg_header() + ex.what());
		}
	}
}

void SemanticAnalyser::visit(ASTUsingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (programs.find(libname) == programs.end()) {
		throw std::runtime_error("lib '" + libname + "' not found");
	}

	auto program = programs[libname];

	// add lib to current program
	if (axe::StringUtils::contains(current_program->libs, libname)) {
		throw std::runtime_error("lib '" + libname + "' already declared in " + current_program->name);
	}
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::StringUtils::contains(parsed_libs, libname)) {
		if (!program->alias.empty()) {
			nmspaces.push_back(program->alias);
		}
		parsed_libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;

		if (current_program->alias == default_namespace) {
			throw std::runtime_error("namespace '" + current_program->alias + "' is not valid ");
		}

		program_nmspaces[get_namespace(current_program->alias)].push_back(default_namespace);

		if (!program->alias.empty()) {
			scopes[program->alias].push_back(new SemanticScope());
		}
		start();
		current_program = prev_program;
	}
}

void SemanticAnalyser::visit(ASTAsNamespaceNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	if (!axe::StringUtils::contains(nmspaces, astnode->nmspace)) {
		throw std::runtime_error("namespace '" + astnode->nmspace + "' not found");
	}
	if (astnode->nmspace == default_namespace) {
		throw std::runtime_error("namespace '" + astnode->nmspace + "' is not valid ");
	}
	program_nmspaces[get_namespace(current_program->alias)].push_back(astnode->nmspace);
}

void SemanticAnalyser::visit(ASTEnumNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		auto decl_current_expr = new SemanticValue();
		decl_current_expr->type = Type::T_INT;
		decl_current_expr->row = astnode->row;
		decl_current_expr->col = astnode->col;
		decl_current_expr->is_const = true;
		decl_current_expr->hash = i;
		scopes[nmspace].back()->declare_variable(astnode->identifiers[i], Type::T_INT, Type::T_UNDEFINED,
			std::vector<ASTExprNode*>(), "", "", decl_current_expr, true, astnode->row, astnode->col);
	}
}

void SemanticAnalyser::visit(ASTDeclarationNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	SemanticScope* current_scope = scopes[nmspace].back();

	if (current_scope->already_declared_variable(astnode->identifier)) {
		throw std::runtime_error("variable '" + astnode->identifier + "' already declared");
	}

	if (is_void(astnode->type)) {
		throw std::runtime_error("variables cannot be declared as void type: '" + astnode->identifier + "'");
	}

	if (is_struct(astnode->type)) {
		try {
			SemanticScope* curr_scope = get_inner_most_struct_definition_scope(get_namespace(astnode->type_name_space), astnode->type_name);
		}
		catch (...) {
			throw std::runtime_error("struct '" + astnode->type_name + "' not found");
		}
	}

	if (astnode->expr) {
		identifier_call_name = astnode->identifier;
		astnode->expr->accept(this);
		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}
		identifier_call_name = "";
	}
	else {
		current_expression = SemanticValue(Type::T_UNDEFINED, astnode->row, astnode->col);
	}

	auto new_value = new SemanticValue();
	new_value->copy_from(&current_expression);
	new_value->hash = astnode->expr ? astnode->expr->hash(this) : 0;

	if (astnode->is_const && !new_value->is_const) {
		throw std::runtime_error("initializer of '" + astnode->identifier + "' is not a constant");
	}

	auto astnode_type = is_undefined(astnode->type) ? Type::T_ANY : astnode->type;
	auto astnode_array_type = is_undefined(astnode->array_type) && astnode->dim.size() > 0 ? Type::T_ANY : astnode->array_type;
	auto astnode_type_name = astnode->type_name.empty() ? new_value->type_name : astnode->type_name;

	SemanticVariable* new_var = new SemanticVariable(astnode->identifier, astnode_type,
		astnode_array_type, astnode->dim,
		astnode_type_name, astnode->type_name_space,
		new_value, astnode->is_const,
		astnode->row, astnode->col);
	new_value->ref = new_var;

	if (!TypeDefinition::is_any_or_match_type(new_var, *new_var, nullptr, *new_value, evaluate_access_vector_ptr)
		&& !is_undefined(new_value->type)) {
		ExceptionHandler::throw_declaration_type_err(astnode->identifier, new_var->type, new_value->type);
	}

	if (new_value->dim.size() < new_var->dim.size() && new_value->dim.size() == 1) {
		new_value->dim = new_var->dim;
	}

	current_scope->declare_variable(astnode->identifier, new_var);
}

void SemanticAnalyser::visit(ASTUnpackedDeclarationNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();
	SemanticScope* current_scope = scopes[nmspace].back();

	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void SemanticAnalyser::visit(ASTAssignmentNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	if (!Token::is_assignment_op(astnode->op)) {
		throw std::runtime_error("expected assignment operator, but found '" + astnode->op + "'");
	}

	const auto& identifier = astnode->identifier;
	const auto& nmspace = get_namespace(astnode->nmspace);
	SemanticScope* curr_scope;
	try {
		curr_scope = get_inner_most_variable_scope(nmspace, identifier);
	}
	catch (...) {
		bool isfunc = false;
		try {
			curr_scope = get_inner_most_function_scope(nmspace, identifier, std::vector<TypeDefinition>());
			isfunc = true;
			throw std::runtime_error("function '" + identifier + "' can't be assigned");
		}
		catch (std::exception ex) {
			if (isfunc) {
				throw std::runtime_error(ex.what());
			}
			else {
				throw std::runtime_error("identifier '" + identifier + "' being reassigned was never declared");
			}
		}
	}

	identifier_call_name = astnode->identifier;
	astnode->expr->accept(this);
	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}
	identifier_call_name = "";
	auto assignment_expr = current_expression;

	auto declared_variable = curr_scope->find_declared_variable(identifier);
	auto decl_var_expression = access_value(declared_variable->value, astnode->identifier_vector);

	if (declared_variable->is_const) {
		throw std::runtime_error("'" + identifier + "' constant being reassigned");
	}

	assignment_expr = SemanticValue(do_operation(astnode->op, *declared_variable, *decl_var_expression,
		nullptr, assignment_expr, false), 0, false, astnode->row, astnode->col);

	if (declared_variable->value == decl_var_expression) {
		declared_variable->value->copy_from(&assignment_expr);
	}
}

void SemanticAnalyser::visit(ASTReturnNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);
	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!current_function.empty()) {
		auto& currfun = current_function.top();
		if (!TypeDefinition::is_any_or_match_type(&currfun, currfun, nullptr, current_expression, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_return_type_err(currfun.identifier,
				currfun.type,
				current_expression.type);
		}
	}
}

void SemanticAnalyser::visit(ASTFunctionCallNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::vector<TypeDefinition> signature = std::vector<TypeDefinition>();

	for (const auto& param : astnode->parameters) {
		param->accept(this);
		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		auto td = TypeDefinition(current_expression.type, current_expression.array_type, current_expression.dim,
			current_expression.type_name, current_expression.type_name_space);

		signature.push_back(td);
	}

	SemanticScope* curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_function_scope(nmspace, astnode->identifier, signature);
	}
	catch (...) {
		std::string func_name = astnode->identifier + "(";
		for (const auto& param : signature) {
			func_name += type_str(param.type) + ", ";
		}
		if (signature.size() > 0) {
			func_name.pop_back();
			func_name.pop_back();
		}
		func_name += ")";

		throw std::runtime_error("function '" + func_name + "' was never declared");
	}

	const auto& curr_function = curr_scope->find_declared_function(astnode->identifier, signature, evaluate_access_vector_ptr);

	if (is_void(curr_function.type)) {
		current_expression = SemanticValue(Type::T_UNDEFINED, 0, 0);
	}
	else {
		current_expression = SemanticValue(curr_function.type,
			curr_function.array_type, curr_function.dim,
			curr_function.type_name,
			curr_function.type_name_space.empty() ?
			astnode->nmspace : curr_function.type_name_space,
			0, false, curr_function.row, curr_function.col
		);
	}
}

void SemanticAnalyser::visit(ASTFunctionDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	for (const auto& scope : scopes[nmspace]) {
		if (scope->already_declared_function(astnode->identifier, astnode->signature, evaluate_access_vector_ptr)) {
			std::string signature = "(";
			for (const auto& param : astnode->signature) {
				signature += type_str(param.type) + ", ";
			}
			if (astnode->signature.size() > 0) {
				signature.pop_back();
				signature.pop_back();
			}
			signature += ")";

			throw std::runtime_error("function " + astnode->identifier + signature + " already defined");
		}
	}

	if (astnode->block) {
		auto has_return = returns(astnode->block);
		auto type = is_void(astnode->type) && has_return ? Type::T_ANY : astnode->type;
		auto array_type = (is_void(astnode->array_type) || is_undefined(astnode->array_type)) && has_return ? Type::T_ANY : astnode->type;

		scopes[nmspace].back()->declare_function(astnode->identifier, type, astnode->type_name, astnode->type_name_space,
			array_type, astnode->dim, astnode->signature, astnode->parameters, astnode->row, astnode->row);

		auto curr_function = scopes[nmspace].back()->find_declared_function(astnode->identifier, astnode->signature, evaluate_access_vector_ptr);

		current_function.push(curr_function);

		astnode->block->accept(this);

		if (!is_void(type)) {
			if (!has_return) {
				throw std::runtime_error("defined function '" + astnode->identifier + "' is not guaranteed to return a value");
			}
		}

		current_function.pop();
	}
	else {
		if (!astnode->block) {
			// todo: check if is builtin
		}
		scopes[nmspace].back()->declare_function(astnode->identifier, astnode->type, astnode->type_name, astnode->type_name_space,
			astnode->array_type, astnode->dim, astnode->signature, astnode->parameters, astnode->row, astnode->row);
	}
}

void SemanticAnalyser::visit(ASTFunctionExpression* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	astnode->fun->identifier = identifier_call_name;
	astnode->fun->accept(this);

	current_expression = SemanticValue();
	current_expression.type = Type::T_FUNCTION;
	current_expression.dim = astnode->fun->dim;
	current_expression.row = astnode->fun->row;
	current_expression.col = astnode->fun->col;
}

void SemanticAnalyser::visit(ASTBlockNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(new SemanticScope());

	if (!current_function.empty()) {
		for (const auto& param : current_function.top().parameters) {
			if (is_function(param.type) || is_any(param.type)) {
				scopes[nmspace].back()->declare_variable_function(param.identifier, param.row, param.row);
			}

			if (!is_function(param.type)) {
				auto var_expr = new SemanticValue();
				var_expr->type = param.type;
				var_expr->array_type = param.array_type;
				var_expr->type_name = param.type_name;
				var_expr->dim = param.dim;
				var_expr->row = param.row;
				var_expr->col = param.col;

				scopes[nmspace].back()->declare_variable(param.identifier, param.type, param.array_type,
					param.dim, param.type_name, param.type_name_space, var_expr, false, param.row, param.col);
			}
		}
	}

	for (const auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	scopes[nmspace].pop_back();
}

void SemanticAnalyser::visit(ASTExitNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->exit_code->accept(this);
	
	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_int(current_expression.type)) {
		throw std::runtime_error("expected int value");
	}
}

void SemanticAnalyser::visit(ASTContinueNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	if (!is_loop) {
		throw std::runtime_error("continue must be inside a loop");
	}
}

void SemanticAnalyser::visit(ASTBreakNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	if (!is_loop && !is_switch) {
		throw std::runtime_error("break must be inside a loop or switch");
	}
}

void SemanticAnalyser::visit(ASTSwitchNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_switch = true;
	const auto& nmspace = get_namespace();
	scopes[nmspace].push_back(new SemanticScope());

	astnode->parsed_case_blocks = std::map<unsigned int, unsigned int>();

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto cond_type = static_cast<TypeDefinition>(current_expression);
	auto case_type = TypeDefinition::get_basic(Type::T_UNDEFINED);

	for (const auto& expr : astnode->case_blocks) {
		expr.first->accept(this);

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		if (!current_expression.is_const) {
			throw std::runtime_error("case expression is not an constant expression");
		}

		if (is_undefined(case_type.type)) {
			if (is_undefined(current_expression.type)
				|| is_void(current_expression.type)
				|| is_any(current_expression.type)) {
				throw std::runtime_error("case values cannot be undefined");
			}
			case_type = current_expression;
		}

		if (!TypeDefinition::match_type(case_type, current_expression, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_mismatched_type_err(case_type.type, current_expression.type);
		}

		auto hash = expr.first->hash(this);
		if (astnode->parsed_case_blocks.contains(hash)) {
			throw std::runtime_error("duplicated case value: '" + std::to_string(hash) + "'");
		}

		astnode->parsed_case_blocks.emplace(hash, expr.second);
	}

	if (!TypeDefinition::is_any_or_match_type(&cond_type, cond_type, nullptr, case_type, evaluate_access_vector_ptr)) {
		ExceptionHandler::throw_mismatched_type_err(cond_type.type, case_type.type);
	}

	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	scopes[nmspace].pop_back();
	is_switch = false;
}

void SemanticAnalyser::visit(ASTElseIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
}

void SemanticAnalyser::visit(ASTIfNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->if_block->accept(this);

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		astnode->else_block->accept(this);
	}
}

void SemanticAnalyser::visit(ASTForNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(new SemanticScope());

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		if (!is_bool(current_expression.type)
			&& !is_any(current_expression.type)) {
			ExceptionHandler::throw_condition_type_err();
		}
	}
	if (astnode->dci[2]) {
		astnode->dci[2]->accept(this);
	}
	astnode->block->accept(this);

	scopes[nmspace].pop_back();
	is_loop = false;
}

void SemanticAnalyser::visit(ASTForEachNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	TypeDefinition col_type;
	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(new SemanticScope());
	SemanticScope* back_scope = scopes[nmspace].back();

	astnode->itdecl->accept(this);

	astnode->collection->accept(this);
	col_type = current_expression;

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->itdecl)) {
		if (!is_struct(col_type.type)) {
			throw std::runtime_error("[key, value] can only be used with struct");
		}

		if (idnode->declarations.size() != 2) {
			throw std::runtime_error("invalid number of values");
		}

		auto decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		decl_key->value = new SemanticValue(Type::T_STRING, astnode->row, astnode->col);

		back_scope = scopes[nmspace].back();
		auto decl_val = back_scope->find_declared_variable(idnode->declarations[1]->identifier);
		decl_val->value = new SemanticValue(Type::T_ANY, astnode->row, astnode->col);
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl)) {
		if (!is_array(col_type.type)
			&& !is_string(col_type.type)
			&& !is_struct(col_type.type)
			&& !is_any(col_type.type)) {
			throw std::runtime_error("expected iterable in foreach");
		}

		if (is_struct(col_type.type)) {
			try {
				auto s = get_inner_most_struct_definition_scope("cp", "Pair");
			}
			catch (...) {
				throw std::runtime_error("struct 'cp::Pair' not found");
			}
			col_type = TypeDefinition::get_struct("Pair", "cp");
		}

		SemanticVariable* declared_variable = back_scope->find_declared_variable(idnode->identifier);

		if (!match_type(declared_variable->type, col_type.type)
			&& !match_type(declared_variable->type, col_type.array_type)
			&& !is_any(declared_variable->type)
			&& !is_any(col_type.type)
			&& !is_any(col_type.array_type)) {
			throw std::runtime_error("mismatched types");
		}

		if (is_struct(col_type.type)) {
			declared_variable->value->type = Type::T_STRUCT;
			declared_variable->value->type_name = "Pair";
			declared_variable->value->type_name_space = "cp";
			declared_variable->value->array_type = Type::T_UNDEFINED;
		}
		else if (is_string(col_type.type)) {
			declared_variable->value->type = Type::T_CHAR;
			declared_variable->value->type_name = "";
			declared_variable->value->type_name_space = "";
			declared_variable->value->array_type = Type::T_UNDEFINED;
		}
		else if (col_type.dim.size() > 1) {
			declared_variable->value->type = col_type.type;
			declared_variable->value->type_name = current_expression.type_name;
			declared_variable->value->type_name_space = current_expression.type_name_space;
			declared_variable->value->array_type = current_expression.array_type;
		}
		else {
			declared_variable->value->type = col_type.array_type;
			declared_variable->value->type_name = current_expression.type_name;
			declared_variable->value->type_name_space = current_expression.type_name_space;
			declared_variable->value->array_type = current_expression.array_type;
		}
		
	}
	else {
		throw std::runtime_error("expected declaration");
	}

	astnode->block->accept(this);

	scopes[nmspace].pop_back();
	is_loop = false;
}

void SemanticAnalyser::visit(ASTTryCatchNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	scopes[nmspace].push_back(new SemanticScope());
	astnode->try_block->accept(this);
	scopes[nmspace].pop_back();

	scopes[nmspace].push_back(new SemanticScope());

	astnode->decl->accept(this);

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->decl)) {
		if (idnode->declarations.size() != 1) {
			throw std::runtime_error("invalid number of values");
		}

		SemanticScope* back_scope = scopes[nmspace].back();
		auto decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		decl_key->value = new SemanticValue(Type::T_STRING, astnode->row, astnode->col);

	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}

		auto declared_variable = current_expression.ref;
		declared_variable->value->type = Type::T_STRUCT;
		declared_variable->value->type_name = "Exception";
		declared_variable->value->type_name_space = "cp";
	}
	else if (!dynamic_cast<ASTReticencesNode*>(astnode->decl)) {
		throw std::runtime_error("expected declaration");
	}

	astnode->catch_block->accept(this);

	scopes[nmspace].pop_back();
}

void SemanticAnalyser::visit(parser::ASTThrowNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->error->accept(this);

	if (is_struct(current_expression.type)
		&& current_expression.type_name == "Exception") {
		try {
			get_inner_most_struct_definition_scope("cp", "Exception");
		}
		catch (...) {
			throw std::runtime_error("struct 'cp::Exception' not found");
		}
	}
	else if (!is_string(current_expression.type)) {
		throw std::runtime_error("expected Exception or string in throw");
	}
}

void SemanticAnalyser::visit(ASTReticencesNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
}

void SemanticAnalyser::visit(ASTWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void SemanticAnalyser::visit(ASTDoWhileNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	is_loop = true;
	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (!is_bool(current_expression.type)
		&& !is_any(current_expression.type)) {
		ExceptionHandler::throw_condition_type_err();
	}

	astnode->block->accept(this);
	is_loop = false;
}

void SemanticAnalyser::visit(ASTStructDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	try {
		SemanticScope* curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->identifier);
		throw std::runtime_error("struct '" + astnode->identifier +
			"' already defined");
	}
	catch (...) {}

	scopes[nmspace].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_bool>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_int>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_float>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_char>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_CHAR;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTLiteralNode<cp_string>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.is_const = true;
}

void SemanticAnalyser::visit(ASTArrayConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
	cp_int arr_size = 0;

	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		++arr_size;
	}

	determine_array_type(astnode);
	auto current_expression_array_type = current_expression.array_type;
	current_expression = SemanticValue();
	current_expression.array_type = current_expression_array_type;
	current_expression.dim = std::vector<ASTExprNode*>{ new ASTLiteralNode<cp_int>(arr_size, astnode->row, astnode->col) };
	current_expression.type = Type::T_ARRAY;
}

void SemanticAnalyser::visit(ASTStructConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	SemanticScope* curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error("struct '" + astnode->type_name +
			"' was not declared");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);

	for (const auto& expr : astnode->values) {
		if (type_struct.variables.find(expr.first) == type_struct.variables.end()) {
			ExceptionHandler::throw_struct_member_err(astnode->type_name, expr.first);
		}
		VariableDefinition var_type_struct = type_struct.variables[expr.first];
		expr.second->accept(this);

		if (!TypeDefinition::is_any_or_match_type(&var_type_struct, var_type_struct,
			nullptr, current_expression, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_struct_type_err(astnode->type_name, var_type_struct.type);
		}
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRUCT;
	current_expression.type_name = astnode->type_name;
	current_expression.type_name_space = astnode->nmspace;
}

void SemanticAnalyser::visit(ASTIdentifierNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	SemanticScope* curr_scope;
	const auto& nmspace = get_namespace(astnode->nmspace);
	try {
		curr_scope = get_inner_most_variable_scope(nmspace, astnode->identifier);
	}
	catch (...) {
		current_expression = SemanticValue();
		if (astnode->identifier == "bool") {
			current_expression.type = Type::T_BOOL;
			return;
		}
		else if (astnode->identifier == "int") {
			current_expression.type = Type::T_INT;
			return;
		}
		else if (astnode->identifier == "float") {
			current_expression.type = Type::T_FLOAT;
			return;
		}
		else if (astnode->identifier == "char") {
			current_expression.type = Type::T_CHAR;
			return;
		}
		else if (astnode->identifier == "string") {
			current_expression.type = Type::T_STRING;
			return;
		}
		try {
			curr_scope = get_inner_most_struct_definition_scope(
				nmspace, astnode->identifier);
			current_expression.type = Type::T_STRUCT;
			return;
		}
		catch (...) {
			try {
				curr_scope = get_inner_most_function_scope(nmspace,
					astnode->identifier, std::vector<TypeDefinition>());
				current_expression.type = Type::T_FUNCTION;
				return;
			}
			catch (...) {
				throw std::runtime_error("identifier '" + astnode->identifier +
					"' was not declared");
			}
		}
	}

	auto declared_variable = curr_scope->find_declared_variable(astnode->identifier);
	auto variable_expr = access_value(declared_variable->value, astnode->identifier_vector);

	declared_variable->reset_ref();
	variable_expr->reset_ref();

	if (is_undefined(variable_expr->type)) {
		throw std::runtime_error("variable '" + astnode->identifier + "' is undefined");
	}

	current_expression = *variable_expr;
	current_expression.resetref();
	current_expression.is_sub = declared_variable->value != variable_expr;

}

void SemanticAnalyser::visit(ASTBinaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->left->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto lexpr = current_expression;

	astnode->right->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto rexpr = current_expression;

	current_expression = SemanticValue(do_operation(astnode->op, lexpr, lexpr, nullptr, rexpr, true), 0, false, 0, 0);
	current_expression.is_const = lexpr.is_const && rexpr.is_const;
}

void SemanticAnalyser::visit(ASTUnaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (astnode->unary_op == "ref" || astnode->unary_op == "unref") {
		if (astnode->unary_op == "ref") {
			if (current_expression.ref) {
				current_expression.ref->use_ref = true;
			}
			current_expression.use_ref = true;
		}
		if (astnode->unary_op == "unref") {
			if (current_expression.ref) {
				current_expression.ref->use_ref = false;
			}
			current_expression.use_ref = false;
		}
	}
	else {
		switch (current_expression.type) {
		case Type::T_INT:
			if (astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++"
				&& astnode->unary_op != "~") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of int expression");
			}
			break;
		case Type::T_FLOAT:
			if (astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of float expression");
			}
			break;
		case Type::T_BOOL:
			if (astnode->unary_op != "not") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
			}
			break;
		case Type::T_ANY:
			if (astnode->unary_op != "not" && astnode->unary_op != "~"
				&& astnode->unary_op != "+" && astnode->unary_op != "-"
				&& astnode->unary_op != "--" && astnode->unary_op != "++") {
				throw std::runtime_error("operator '" + astnode->unary_op + "' in front of boolean expression");
			}
			break;
		default:
			throw std::runtime_error("incompatible unary operator '" + astnode->unary_op +
				"' in front of " + type_str(current_expression.type) + " expression");
		}
	}
	current_expression.is_const = false;
}

void SemanticAnalyser::visit(ASTTernaryNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->condition->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	astnode->value_if_true->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	astnode->value_if_false->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}
}

void SemanticAnalyser::visit(ASTInNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->value->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto valtype = current_expression.type;

	astnode->collection->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	auto coltype = current_expression.type;
	auto colarrtype = current_expression.type;

	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;

	if (is_any(valtype)
		&& (is_any(coltype)
			|| is_array(coltype)
			&& is_any(colarrtype))) {
		return;
	}

	if (!match_type(valtype, colarrtype)
		&& !is_any(valtype) && !is_any(coltype) && !is_any(colarrtype)
		&& is_string(valtype) && !is_string(coltype)
		&& is_char(valtype) && !is_string(coltype)) {
		throw std::runtime_error("types don't match '" + type_str(valtype) + "' and '" + type_str(coltype) + "'");
	}

	if (!is_collection(coltype) && !is_any(coltype)) {
		throw std::runtime_error("invalid type '" + type_str(coltype) + "', value must be a array or string");
	}

}

void SemanticAnalyser::visit(ASTTypeParseNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if ((is_array(current_expression.type) || is_struct(current_expression.type))
		&& !is_string(astnode->type)) {
		throw std::runtime_error("invalid type conversion from "
			+ type_str(current_expression.type) + " to " + type_str(astnode->type));
	}

	current_expression = SemanticValue();
	current_expression.type = astnode->type;
}

void SemanticAnalyser::visit(ASTNullNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_VOID;
}

void SemanticAnalyser::visit(ASTThisNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
}

void SemanticAnalyser::visit(ASTTypingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	current_expression = SemanticValue();
	if (astnode->image == "typeid") {
		current_expression.type = Type::T_INT;
	}
	else if (astnode->image == "typeof") {
		current_expression.type = Type::T_STRING;
	}
	else {
		current_expression.type = Type::T_BOOL;
	}
}

bool SemanticAnalyser::namespace_exists(const std::string& nmspace) {
	return scopes.find(nmspace) != scopes.end();
}

VariableDefinition SemanticAnalyser::access_struct_variable(std::vector<Identifier> identifier_vector, std::string type_name, std::string nmspace, unsigned int i) {
	SemanticScope* curr_scope;
	try {
		curr_scope = get_inner_most_struct_definition_scope(get_namespace(nmspace), type_name);
	}
	catch (...) {
		throw std::runtime_error("can't find struct");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(type_name);

	if (type_struct.variables.find(identifier_vector[i].identifier) == type_struct.variables.end()) {
		ExceptionHandler::throw_struct_member_err(type_name, identifier_vector[i].identifier);
	}
	VariableDefinition var_type_struct = type_struct.variables[identifier_vector[i].identifier];

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

SemanticScope* SemanticAnalyser::get_inner_most_variable_scope(const std::string& nmspace, const std::string& identifier) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
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
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("identifier '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

SemanticScope* SemanticAnalyser::get_inner_most_struct_definition_scope(const std::string& nmspace, const std::string& identifier) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_structure_definition(identifier); i--) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_structure_definition(identifier); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("struct '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

SemanticScope* SemanticAnalyser::get_inner_most_function_scope(const std::string& nmspace, const std::string& identifier, const std::vector<TypeDefinition>& signature) {
	if (!namespace_exists(nmspace)) {
		throw std::runtime_error("namespace '" + nmspace + "' was not declared");
	}
	long long i;
	for (i = scopes[nmspace].size() - 1; !scopes[nmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr); --i) {
		if (i <= 0) {
			for (const auto& prgnmspace : program_nmspaces[get_namespace()]) {
				for (i = scopes[prgnmspace].size() - 1; !scopes[prgnmspace][i]->already_declared_function(identifier, signature, evaluate_access_vector_ptr); --i) {
					if (i <= 0) {
						i = -1;
						break;
					}
				}
				if (i >= 0) {
					return scopes[prgnmspace][i];
				}
			}
			throw std::runtime_error("function '" + identifier + "' was not declared");
		}
	}
	return scopes[nmspace][i];
}

TypeDefinition SemanticAnalyser::do_operation(const std::string& op, TypeDefinition lvar, TypeDefinition lvalue, TypeDefinition* rvar, TypeDefinition rvalue, bool is_expr) {
	Type l_var_type = lvar.type;
	lvalue = is_undefined(lvalue.type) ? lvar : lvalue;
	Type l_type = lvalue.type;
	Type r_var_type = rvar ? rvar->type : rvalue.type;
	Type r_type = rvalue.type;

	if ((is_any(l_var_type) || is_any(r_var_type)
		|| is_any(l_type) || is_any(r_type)
		|| is_void(l_type) || is_void(r_type)) && op == "=") {
		return rvalue;
	}

	if ((is_void(l_type) || is_void(r_type))
		&& Token::is_equality_op(op)) {
		return TypeDefinition::get_basic(Type::T_BOOL);
	}

	if (is_any(l_type) || is_any(r_type)) {
		if (Token::is_relational_op(op)) {
			return TypeDefinition::get_basic(Type::T_BOOL);
		}
		return is_any(r_type) ?
			!rvar || is_any(r_var_type) ?
			is_any(l_type) ? lvar : lvalue
			: *rvar
			: rvalue;
	}

	switch (r_type) {
	case Type::T_BOOL: {
		if (!is_bool(l_type)
			|| op != "="
			&& op != "and"
			&& op != "or"
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_INT: {
		if (is_float(l_type) && is_any(l_var_type)
			|| is_int(l_type) && is_any(l_var_type)
			&& !Token::is_int_ex_op(op)) {
			if (!Token::is_float_op(op)
				&& !Token::is_relational_op(op)
				&& !Token::is_equality_op(op)) {
				ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
			}
		}
		else if (is_int(l_type)
			&& !Token::is_int_op(op)
			&& !Token::is_relational_op(op)
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}
		else if (!is_numeric(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_FLOAT: {
		if ((is_float(l_type) || is_int(l_type))
			&& !Token::is_float_op(op)
			&& !Token::is_relational_op(op)
			&& !Token::is_equality_op(op)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}
		else if (!is_numeric(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_CHAR: {
		if (is_string(l_type) && !Token::is_collection_op(op)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}
		else if (is_char(l_type)) {
			if (op != "=" && !Token::is_equality_op(op)) {
				ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
			}
		}
		else if (!is_text(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_STRING: {
		if ((!is_string(l_type)
			|| (!Token::is_collection_op(op)
				&& !Token::is_equality_op(op)))
			&& (is_expr && (!is_char(l_type)
				|| !Token::is_expression_collection_op(op)))) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}
		else if (!is_text(l_type) && !is_any(l_type) && !is_any(l_var_type)) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_ARRAY: {
		if (!TypeDefinition::match_type_array(lvalue, rvalue, evaluate_access_vector_ptr)
			|| (!Token::is_collection_op(op)
				&& !Token::is_equality_op(op))) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_STRUCT: {
		if (!is_struct(l_type) || (!Token::is_equality_op(op) && op != "=")) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	case Type::T_FUNCTION: {
		if (!is_function(l_type) || (!Token::is_equality_op(op) && op != "=")) {
			ExceptionHandler::throw_operation_type_err(op, l_type, r_type);
		}

		break;
	}
	default:
		throw std::runtime_error("cannot determine type of operation");

	}

	if (Token::is_equality_op(op) || Token::is_relational_op(op)) {
		return TypeDefinition::get_basic(Type::T_BOOL);
	}

	return is_float(lvalue.type) || is_string(lvalue.type) ? lvalue : rvalue;
}

void SemanticAnalyser::equals_value(const SemanticValue& lval, const SemanticValue& rval) {
	if (lval.use_ref && !rval.use_ref) {
		throw std::runtime_error("both values must be references");
	}
	if (!lval.use_ref && rval.use_ref) {
		throw std::runtime_error("both values must be unreferenced");
	}
}

void SemanticAnalyser::determine_array_type(ASTArrayConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

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
	set_curr_pos(astnode->row, astnode->col);

	auto aux_curr_type = current_expression.type;
	astnode->accept(this);

	if (is_any(current_expression.array_type) || is_undefined(current_expression.array_type) || is_void(current_expression.array_type)) {
		current_expression.array_type = current_expression.type;
	}
	if (!match_type(current_expression.array_type, current_expression.type)) {
		throw std::runtime_error("mismatched type in array definition");
	}
	current_expression.type = aux_curr_type;
}

SemanticValue* SemanticAnalyser::access_value(SemanticValue* value, const std::vector<Identifier>& identifier_vector, size_t i) {
	SemanticValue* next_value = value;

	auto access_vector = evaluate_access_vector(identifier_vector[i].access_vector);

	if (access_vector.size() > 0) {
		if (access_vector.size() == value->dim.size()) {
			next_value = new SemanticValue(next_value->array_type, Type::T_UNDEFINED,
				std::vector<ASTExprNode*>(), next_value->type_name, next_value->type_name_space,
				0, false, next_value->row, next_value->col);
		}
	}

	++i;

	if (i < identifier_vector.size()) {
		SemanticScope* curr_scope;
		try {
			curr_scope = get_inner_most_struct_definition_scope(get_namespace(next_value->type_name_space), next_value->type_name);
		}
		catch (...) {
			throw std::runtime_error("can't find struct");
		}
		auto type_struct = curr_scope->find_declared_structure_definition(next_value->type_name);

		if (type_struct.variables.find(identifier_vector[i].identifier) == type_struct.variables.end()) {
			ExceptionHandler::throw_struct_member_err(next_value->type_name, identifier_vector[i].identifier);
		}

		next_value = new SemanticValue(type_struct.variables[identifier_vector[i].identifier], 0, false, next_value->row, next_value->col);

		if (identifier_vector[i].access_vector.size() > 0 || i < identifier_vector.size()) {
			return access_value(next_value, identifier_vector, i);
		}
	}

	return next_value;
}

void SemanticAnalyser::check_is_struct_exists(parser::Type type, const std::string& nmspace, const std::string& type_name) {
	if (is_struct(type)) {
		try {
			get_inner_most_struct_definition_scope(get_namespace(nmspace), type_name);
		}
		catch (...) {
			throw std::runtime_error("struct '" + type_name + "' was not defined");
		}
	}
}

std::vector<unsigned int> SemanticAnalyser::evaluate_access_vector(const std::vector<ASTExprNode*>& expr_access_vector) {
	auto access_vector = std::vector<unsigned int>();
	for (const auto& expr : expr_access_vector) {
		unsigned int val = 0;
		if (expr) {
			expr->accept(this);
			val = expr->hash(this);
			if (!is_int(current_expression.type)) {
				throw std::runtime_error("array index access must be a integer value");
			}
		}
		access_vector.push_back(val);
	}
	return access_vector;
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

bool SemanticAnalyser::returns(ASTNode* astnode) {
	if (dynamic_cast<ASTReturnNode*>(astnode)) {
		return true;
	}

	if (auto block = dynamic_cast<ASTBlockNode*>(astnode)) {
		for (const auto& blk_stmt : block->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	if (auto ifstmt = dynamic_cast<ASTIfNode*>(astnode)) {
		auto ifreturn = returns(ifstmt->if_block);
		auto elifreturn = true;
		auto elsereturn = true;
		for (const auto& elif : ifstmt->else_ifs) {
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

	if (const auto switchstmt = dynamic_cast<ASTSwitchNode*>(astnode)) {
		for (const auto& blk_stmt : switchstmt->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	if (const auto forstmt = dynamic_cast<ASTForNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (const auto forstmt = dynamic_cast<ASTForEachNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (const auto forstmt = dynamic_cast<ASTForEachNode*>(astnode)) {
		return returns(forstmt->block);
	}

	if (const auto whilestmt = dynamic_cast<ASTWhileNode*>(astnode)) {
		return returns(whilestmt->block);
	}

	return false;
}

long long SemanticAnalyser::hash(ASTExprNode* astnode) {
	astnode->accept(this);
	return 0;
}

long long SemanticAnalyser::hash(ASTLiteralNode<cp_bool>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long SemanticAnalyser::hash(ASTLiteralNode<cp_int>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long SemanticAnalyser::hash(ASTLiteralNode<cp_float>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long SemanticAnalyser::hash(ASTLiteralNode<cp_char>* astnode) {
	return static_cast<long long>(astnode->val);
}

long long SemanticAnalyser::hash(ASTLiteralNode<cp_string>* astnode) {
	return axe::StringUtils::hashcode(astnode->val);
}

long long SemanticAnalyser::hash(ASTIdentifierNode* astnode) {
	SemanticScope* curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_variable_scope(nmspace, astnode->identifier_vector[0].identifier);
	}
	catch (...) {
		throw std::runtime_error("identifier '" + astnode->identifier_vector[0].identifier +
			"' was not declared");
	}

	auto declared_variable = curr_scope->find_declared_variable(astnode->identifier_vector[0].identifier);
	auto variable_expression = declared_variable->value;

	return variable_expression->hash;
}

void SemanticAnalyser::register_built_in_functions() {
	auto signature = std::vector<visitor::TypeDefinition>();
	auto parameters = std::vector<visitor::VariableDefinition>();

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_basic_function("print", Type::T_VOID, signature, parameters);

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_basic_function("println", Type::T_VOID, signature, parameters);


	signature.clear();
	parameters.clear();
	scopes[default_namespace].back()->declare_basic_function("read", Type::T_STRING, signature, parameters);
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("args", Type::T_ANY, new ASTNullNode(0, 0), true));
	scopes[default_namespace].back()->declare_basic_function("read", Type::T_STRING, signature, parameters);


	signature.clear();
	parameters.clear();
	scopes[default_namespace].back()->declare_basic_function("readch", Type::T_CHAR, signature, parameters);


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_array(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_array("arr", Type::T_ANY));
	scopes[default_namespace].back()->declare_basic_function("len", Type::T_INT, signature, parameters);

	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_STRING));
	parameters.push_back(VariableDefinition::get_basic("str", Type::T_STRING));
	scopes[default_namespace].back()->declare_basic_function("len", Type::T_INT, signature, parameters);


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	signature.push_back(TypeDefinition::get_basic(Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("lval", Type::T_ANY));
	parameters.push_back(VariableDefinition::get_basic("rval", Type::T_ANY));
	scopes[default_namespace].back()->declare_basic_function("equals", Type::T_BOOL, signature, parameters);


	signature.clear();
	parameters.clear();
	signature.push_back(TypeDefinition::get_basic(Type::T_STRING));
	parameters.push_back(VariableDefinition::get_basic("cmd", Type::T_STRING));
	scopes[default_namespace].back()->declare_basic_function("system", Type::T_VOID, signature, parameters);
}

const std::string& SemanticAnalyser::get_namespace(const std::string& nmspace) const {
	return get_namespace(current_program, nmspace);
}

const std::string& SemanticAnalyser::get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace) const {
	return nmspace.empty() ? (program->alias.empty() ? default_namespace : program->alias) : nmspace;
}

void SemanticAnalyser::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string SemanticAnalyser::msg_header() {
	return "(SERR) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
