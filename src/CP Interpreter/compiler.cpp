#include <iostream>
#include <utility>

#include "compiler.hpp"
#include "exception_handler.hpp"
#include "graphics.hpp"
#include "token.hpp"
#include "builtin.hpp"

#include "vendor/axeutils.hpp"
#include "vendor/axeuuid.hpp"

using namespace visitor;
using namespace parser;
using namespace lexer;

Compiler::Compiler(ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs)
	: Visitor(programs, main_program, main_program ? main_program->name : default_namespace) {
	auto builtin = std::unique_ptr<modules::Builtin>(new modules::Builtin());
	//builtin->register_functions(this);
};

void Compiler::start() {
	visit(current_program);
}

void Compiler::visit(ASTProgramNode* astnode) {
	for (const auto& statement : astnode->statements) {
		try {
			statement->accept(this);
		}
		catch (std::exception ex) {
			throw std::runtime_error(ex.what());
		}
	}
}

void Compiler::visit(ASTUsingNode* astnode) {
	std::string libname = axe::StringUtils::join(astnode->library, ".");

	if (built_in_libs.find(libname) != built_in_libs.end()) {
		//built_in_libs.find(libname)->second->register_functions(this);
	}

	auto program = programs[libname];

	// add lib to current program
	current_program->libs.push_back(libname);

	// if can't parsed yet
	if (!axe::StringUtils::contains(parsed_libs, libname)) {
		parsed_libs.push_back(libname);
		auto prev_program = current_program;
		current_program = program;

		start();
		current_program = prev_program;
	}
}

void Compiler::visit(ASTNamespaceManagerNode* astnode) {}

void Compiler::visit(ASTEnumNode* astnode) {
	for (size_t i = 0; i < astnode->identifiers.size(); ++i) {
		add_instruction(OpCode::OP_PUSH_INT, byteopnd8(i));
		add_instruction(OpCode::OP_SET_VAR_TYPE, byteopnd8(Type::T_INT));
		add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifiers[i]));
	}
}

void Compiler::visit(ASTDeclarationNode* astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_SET_VAR_ARRAY_DIM, byteopnd8(astnode->dim.size()));
	for (auto& s : astnode->dim) {
		s->accept(this);
		add_instruction(OpCode::OP_SET_VAR_ARRAY_SIZE, byteopnd_n);
	}
	add_instruction(OpCode::OP_SET_VAR_TYPE, byteopnd8(astnode->type));

	// todo parse {1} array build

	add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifier));
}

void Compiler::visit(ASTUnpackedDeclarationNode* astnode) {
	for (const auto& declaration : astnode->declarations) {
		declaration->accept(this);
	}
}

void Compiler::visit(ASTAssignmentNode* astnode) {
	astnode->expr->accept(this);
	add_instruction(OpCode::OP_STORE_VAR, byteopnd_s(astnode->identifier));
}

void Compiler::visit(ASTReturnNode* astnode) {
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		//add_instruction(OpCode::OP_PUSH_UNDEFINED, byteopnd_n);
	}

	add_instruction(OpCode::OP_RETURN, byteopnd_n);
}

void Compiler::visit(ASTFunctionCallNode* astnode) {
	for (const auto& param : astnode->parameters) {
		param->accept(this);
	}

	add_instruction(OpCode::OP_CALL, byteopnd_s(astnode->identifier));

}

void Compiler::visit(ASTFunctionDefinitionNode* astnode) {
	if (astnode->block) {
		const auto& nmspace = get_namespace(astnode->type_name_space);

		add_instruction(OpCode::OP_FUN_START, byteopnd_s(astnode->identifier));

		for (auto& param : astnode->parameters) {
			if (param.default_value) {
				auto param_dcl = std::make_unique<ASTDeclarationNode>(param.identifier, param.type, param.array_type,
					param.dim, param.type_name, param.type_name_space, param.default_value, false,
					astnode->row, astnode->col);
				param_dcl->accept(this);
			}
		}

		add_instruction(OpCode::OP_FUN_PARAM_END, byteopnd_n);

		astnode->block->accept(this);

		add_instruction(OpCode::OP_FUN_END, byteopnd_n);
	}
}

void Compiler::visit(ASTFunctionExpression* astnode) {
	astnode->fun->identifier = axe::UUID::generate();
	astnode->fun->accept(this);
	add_instruction(OpCode::OP_PUSH_FUNCTION, byteopnd_s(astnode->fun->identifier));
}

void Compiler::visit(ASTBlockNode* astnode) {
	for (const auto& stmt : astnode->statements) {
		stmt->accept(this);
	}
}

void Compiler::visit(ASTExitNode* astnode) {
	astnode->exit_code->accept(this);
	add_instruction(OpCode::OP_HALT, byteopnd_n);
}

void Compiler::visit(ASTContinueNode* astnode) {
	add_instruction(OpCode::OP_CONTINUE, byteopnd_n);
}

void Compiler::visit(ASTBreakNode* astnode) {
	add_instruction(OpCode::OP_BREAK, byteopnd_n);
}

void Compiler::visit(ASTSwitchNode* astnode) {
	astnode->condition->accept(this);

	for (size_t i = 0; i < astnode->statements.size(); ++i) {
		for (const auto& [key, value] : astnode->parsed_case_blocks) {
			if (i == value || i == astnode->default_block) {
				add_instruction(OpCode::OP_JUMP_IF_FALSE_OR_NEXT, nullptr);
			}
		}
		add_instruction(OpCode::OP_POP, byteopnd_n);
		astnode->statements[i]->accept(this);
		replace_last_operand(byteopnd8(pointer));
	}
}

void Compiler::visit(ASTElseIfNode* astnode) {
	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

	astnode->block->accept(this);

	replace_last_operand(byteopnd8(pointer));
}

void Compiler::visit(ASTIfNode* astnode) {
	astnode->condition->accept(this);

	add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

	astnode->if_block->accept(this);

	replace_last_operand(byteopnd8(pointer));

	for (const auto& elif : astnode->else_ifs) {
		elif->accept(this);
	}

	if (astnode->else_block) {
		add_instruction(OpCode::OP_JUMP_IF_FALSE, nullptr);

		astnode->else_block->accept(this);

		replace_last_operand(byteopnd8(pointer));
	}
}

void Compiler::visit(ASTForNode* astnode) {

	//deviation_stack.push(pointer);

	if (astnode->dci[0]) {
		astnode->dci[0]->accept(this);
	}
	if (astnode->dci[1]) {
		astnode->dci[1]->accept(this);
	}
	if (astnode->dci[2]) {
		astnode->dci[2]->accept(this);
	}
	astnode->block->accept(this);
}

void Compiler::visit(ASTForEachNode* astnode) {
	astnode->itdecl->accept(this);

	astnode->collection->accept(this);

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->itdecl)) {
		const auto& decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		auto& decl_key_ptr = decl_key.first;
		decl_key_ptr->value = std::make_shared<SemanticValue>(Type::T_STRING, astnode->row, astnode->col);

		back_scope = scopes[nmspace].back();
		const auto& decl_val = back_scope->find_declared_variable(idnode->declarations[1]->identifier);
		auto& decl_val_ptr = decl_val.first;
		decl_val_ptr->value = std::make_shared<SemanticValue>(Type::T_ANY, astnode->row, astnode->col);
	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->itdecl)) {
		const auto& declared_variable = back_scope->find_declared_variable(idnode->identifier);
		auto& declared_variable_ptr = declared_variable.first;

		if (is_struct(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_STRUCT;
			declared_variable_ptr->value->type_name = "Pair";
			declared_variable_ptr->value->type_name_space = "cp";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (is_string(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_CHAR;
			declared_variable_ptr->value->type_name = "";
			declared_variable_ptr->value->type_name_space = "";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (is_any(col_type.type)) {
			declared_variable_ptr->value->type = Type::T_ANY;
			declared_variable_ptr->value->type_name = "";
			declared_variable_ptr->value->type_name_space = "";
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
		}
		else if (col_type.dim.size() > 1) {
			if (!is_any(declared_variable_ptr->type)) {
				declared_variable_ptr->value->type = declared_variable_ptr->type;
				declared_variable_ptr->value->array_type = declared_variable_ptr->array_type;
				declared_variable_ptr->value->type_name = declared_variable_ptr->type_name;
				declared_variable_ptr->value->type_name_space = declared_variable_ptr->type_name_space;
			}
			else {
				declared_variable_ptr->value->type = col_type.type;
				declared_variable_ptr->value->array_type = current_expression.array_type;
				if (!current_expression.type_name.empty()) {
					declared_variable_ptr->value->type_name = current_expression.type_name;
					declared_variable_ptr->value->type_name_space = current_expression.type_name_space;
				}
			}
		}
		else {
			declared_variable_ptr->value->type = col_type.array_type;
			declared_variable_ptr->value->array_type = Type::T_UNDEFINED;
			if (!current_expression.type_name.empty()) {
				declared_variable_ptr->value->type_name = current_expression.type_name;
				declared_variable_ptr->value->type_name_space = current_expression.type_name_space;
			}
		}

	}

	astnode->block->accept(this);
}

void Compiler::visit(ASTTryCatchNode* astnode) {
	astnode->try_block->accept(this);

	astnode->decl->accept(this);

	if (const auto idnode = dynamic_cast<ASTUnpackedDeclarationNode*>(astnode->decl)) {
		std::shared_ptr<CompilerScope> back_scope = scopes[nmspace].back();
		const auto& decl_key = back_scope->find_declared_variable(idnode->declarations[0]->identifier);
		auto& decl_key_ptr = decl_key.first;
		decl_key_ptr->value = std::make_shared<SemanticValue>(Type::T_STRING, astnode->row, astnode->col);

	}
	else if (const auto idnode = dynamic_cast<ASTDeclarationNode*>(astnode->decl)) {
		auto& declared_variable = current_expression.ref;
		declared_variable->value->type = Type::T_STRUCT;
		declared_variable->value->type_name = "Exception";
		declared_variable->value->type_name_space = "cp";
	}
	else if (!dynamic_cast<ASTReticencesNode*>(astnode->decl)) {
		throw std::runtime_error("expected declaration");
	}

	astnode->catch_block->accept(this);
}

void Compiler::visit(parser::ASTThrowNode* astnode) {
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

void Compiler::visit(ASTReticencesNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
}

void Compiler::visit(ASTWhileNode* astnode) {
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

void Compiler::visit(ASTDoWhileNode* astnode) {
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

void Compiler::visit(ASTStructDefinitionNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	const auto& nmspace = get_namespace();

	try {
		std::shared_ptr<CompilerScope> curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->identifier);
		throw std::runtime_error("struct '" + astnode->identifier +
			"' already defined");
	}
	catch (...) {}

	scopes[nmspace].back()->declare_structure_definition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void Compiler::visit(ASTLiteralNode<cp_bool>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_BOOL;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_int>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_INT;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_float>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_FLOAT;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_char>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_CHAR;
	current_expression.is_const = true;
}

void Compiler::visit(ASTLiteralNode<cp_string>* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
	current_expression.is_const = true;
}

void Compiler::visit(ASTArrayConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto is_const = true;
	cp_int arr_size = 0;

	if (current_expression_array_dim.size() == 0) {
		current_expression_array_type = TypeDefinition();
		current_expression_array_dim_max = 0;
		is_max = false;
	}

	++current_expression_array_dim_max;
	if (!is_max) {
		current_expression_array_dim.push_back(new ASTLiteralNode<cp_int>(0, astnode->row, astnode->col));
	}

	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
		if (!current_expression.is_const) {
			is_const = false;
		}

		if (is_undefined(current_expression.type)) {
			throw std::runtime_error("undefined expression");
		}

		if (is_undefined(current_expression_array_type.type) || is_array(current_expression_array_type.type)) {
			current_expression_array_type = current_expression;
		}
		else {
			if (!match_type(current_expression_array_type.type, current_expression.type)
				&& !is_any(current_expression.type) && !is_void(current_expression.type)
				&& !is_array(current_expression.type)) {
				current_expression_array_type = TypeDefinition::get_basic(Type::T_ANY);
			}
		}

		++arr_size;
	}

	if (!is_max) {
		((ASTLiteralNode<cp_int>*)current_expression_array_dim.back())->val = arr_size;
	}

	is_max = true;

	current_expression = SemanticValue();
	current_expression.type = Type::T_ARRAY;
	current_expression.array_type = current_expression_array_type.type;
	current_expression.type_name = current_expression_array_type.type_name;
	current_expression.type_name_space = current_expression_array_type.type_name_space;
	current_expression.is_const = is_const;
	--current_expression_array_dim_max;
	size_t stay = current_expression_array_dim.size() - current_expression_array_dim_max;
	std::vector<ASTExprNode*> current_expression_array_dim_aux;
	size_t curr_dim_i = current_expression_array_dim.size() - 1;
	for (size_t i = 0; i < stay; ++i) {
		current_expression_array_dim_aux.emplace(current_expression_array_dim_aux.begin(), current_expression_array_dim.at(curr_dim_i));
		--curr_dim_i;
	}
	current_expression.dim = current_expression_array_dim_aux;

	if (current_expression_array_dim_max == 0) {
		if (is_undefined(current_expression.array_type)) {
			current_expression.array_type = Type::T_ANY;
		}
		current_expression_array_dim.clear();
	}
}

void Compiler::visit(ASTStructConstructorNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);
	auto is_const = true;

	std::shared_ptr<CompilerScope> curr_scope;
	try {
		const auto& nmspace = get_namespace(astnode->nmspace);
		curr_scope = get_inner_most_struct_definition_scope(nmspace, astnode->type_name);
	}
	catch (...) {
		throw std::runtime_error("struct '" + astnode->type_name +
			"' was not declared");
	}
	auto type_struct = curr_scope->find_declared_structure_definition(astnode->type_name);
	auto& type_struct_ptr = type_struct.first;

	for (const auto& expr : astnode->values) {
		if (type_struct_ptr.variables.find(expr.first) == type_struct_ptr.variables.end()) {
			ExceptionHandler::throw_struct_member_err(astnode->nmspace, astnode->type_name, expr.first);
		}
		const auto& var_type_struct = type_struct_ptr.variables[expr.first];
		expr.second->accept(this);
		if (!current_expression.is_const) {
			is_const = false;
		}

		if (!TypeDefinition::is_any_or_match_type(var_type_struct,
			current_expression, evaluate_access_vector_ptr)) {
			ExceptionHandler::throw_struct_type_err(astnode->nmspace, astnode->type_name, var_type_struct, evaluate_access_vector_ptr);
		}
	}

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRUCT;
	current_expression.type_name = astnode->type_name;
	current_expression.type_name_space = astnode->nmspace;
	current_expression.is_const = is_const;
}

void Compiler::visit(ASTIdentifierNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	std::shared_ptr<CompilerScope> curr_scope;
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
					astnode->identifier, nullptr);
				current_expression.type = Type::T_FUNCTION;

				return;
			}
			catch (...) {
				throw std::runtime_error("identifier '" + astnode->identifier +
					"' was not declared");
			}
		}
	}

	const auto& declared_variable = curr_scope->find_declared_variable(astnode->identifier);
	const auto& declared_variable_ptr = declared_variable.first;
	auto variable_expr = access_value(declared_variable_ptr->value, astnode->identifier_vector);
	variable_expr->reset_ref();

	if (is_undefined(variable_expr->type)) {
		throw std::runtime_error("variable '" + astnode->identifier + "' is undefined");
	}

	current_expression = *variable_expr;
	current_expression.reset_ref();
	current_expression.is_sub = declared_variable_ptr->value != variable_expr;

}

void Compiler::visit(ASTBinaryExprNode* astnode) {
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

void Compiler::visit(ASTUnaryExprNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	if (astnode->unary_op == "ref" || astnode->unary_op == "unref") {
		if (astnode->unary_op == "ref") {
			current_expression.use_ref = true;
		}
		if (astnode->unary_op == "unref") {
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
}

void Compiler::visit(ASTTernaryNode* astnode) {
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

void Compiler::visit(ASTInNode* astnode) {
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

void Compiler::visit(ASTTypeParseNode* astnode) {
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

void Compiler::visit(ASTNullNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_VOID;
}

void Compiler::visit(ASTThisNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	current_expression = SemanticValue();
	current_expression.type = Type::T_STRING;
}

void Compiler::visit(ASTTypingNode* astnode) {
	set_curr_pos(astnode->row, astnode->col);

	astnode->expr->accept(this);

	if (is_undefined(current_expression.type)) {
		throw std::runtime_error("undefined expression");
	}

	current_expression = SemanticValue();
	if (astnode->image == "typeid" || astnode->image == "refid") {
		current_expression.type = Type::T_INT;
	}
	else if (astnode->image == "typeof") {
		current_expression.type = Type::T_STRING;
	}
	else {
		current_expression.type = Type::T_BOOL;
	}
}

void Compiler::add_instruction(OpCode opcode, uint8_t* operand) {
	bytecode_program.push_back(BytecodeInstruction{ opcode, operand });
	++pointer;
}

void Compiler::replace_last_operand(uint8_t* operand) {
	for (size_t i = bytecode_program.size() - 1; i > 0; --i) {
		if (!bytecode_program[i].operand) {
			bytecode_program[i].operand = operand;
			break;
		}
		if (i == 0) break;
	}
}

bool Compiler::namespace_exists(const std::string& nmspace) {}

long long Compiler::hash(ASTExprNode* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_bool>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_int>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_float>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_char>* astnode) { return 0; }

long long Compiler::hash(ASTLiteralNode<cp_string>* astnode) { return 0; }

long long Compiler::hash(ASTIdentifierNode* astnode) { return 0; }

const std::string& Compiler::get_namespace(const std::string& nmspace) const {
	return get_namespace(current_program, nmspace);
}

const std::string& Compiler::get_namespace(const parser::ASTProgramNode* program, const std::string& nmspace) const {
	return nmspace.empty() ? (
		program->alias.empty() ? default_namespace : program->alias
		) : nmspace;
}

void Compiler::set_curr_pos(unsigned int row, unsigned int col) {
	curr_row = row;
	curr_col = col;
}

std::string Compiler::msg_header() {
	return "(CMP) " + current_program->name + '[' + std::to_string(curr_row) + ':' + std::to_string(curr_col) + "]: ";
}
