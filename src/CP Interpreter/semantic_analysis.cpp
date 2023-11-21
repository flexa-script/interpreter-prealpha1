#include <utility>
#include <iostream>

#include "semantic_analysis.h"


using namespace visitor;


bool SemanticScope::alreadyDeclared(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool SemanticScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		return false;
	}
	// check signature for each function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<1>(i->second) == signature) {
			return true;
		}
	}

	// function with matching signature not found
	return false;
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, unsigned int lineNumber) {
	variableSymbolTable[identifier] = std::make_pair(type, lineNumber);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::vector<parser::TYPE> signature, unsigned int lineNumber) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(type, signature, lineNumber)));
}

parser::TYPE SemanticScope::type(std::string identifier) {
	if (alreadyDeclared(identifier)) {
		return variableSymbolTable[identifier].first;
	}

	throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");
}

parser::TYPE SemanticScope::type(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");
	}

	// check signature for each
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<1>(i->second) == signature) {
			return std::get<0>(i->second);
		}
	}

	// function with matching signature not found
	throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");
}

unsigned int SemanticScope::declaration_line(std::string identifier) {
	if (alreadyDeclared(identifier)) {
		return variableSymbolTable[std::move(identifier)].second;
	}

	throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");
}

unsigned int SemanticScope::declaration_line(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");
	}

	// check signature for each
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<1>(i->second) == signature) {
			return std::get<2>(i->second);
		}
	}

	// function with matching signature not found
	throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");
}


std::vector<std::pair<std::string, std::string>> SemanticScope::function_list() {
	std::vector<std::pair<std::string, std::string>> list;

	for (auto func = functionSymbolTable.begin(), last = functionSymbolTable.end(); func != last; func = functionSymbolTable.upper_bound(func->first)) {
		std::string func_name = func->first + "(";
		bool has_params = false;
		for (auto param : std::get<1>(func->second)) {
			has_params = true;
			func_name += typeStr(param) + ", ";
		}
		func_name.pop_back();   // remove last whitespace
		func_name.pop_back();   // remove last comma
		func_name += ")";

		list.emplace_back(std::make_pair(func_name, typeStr(std::get<0>(func->second))));
	}

	return list;
}


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]) {
	// add global scope
	scopes.push_back(global_scope);
};

SemanticAnalyser::~SemanticAnalyser() = default;


void SemanticAnalyser::start() {
	visit(currentProgram);
}

void SemanticAnalyser::visit(parser::ASTProgramNode* prog) {
	// for each statement, accept
	for (auto& statement : prog->statements) {
		statement->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTUsingNode* usg) {
	for (auto program : programs) {
		if (usg->library == program->name) {
			auto prevProgram = currentProgram;
			currentProgram = program;
			start();
			currentProgram = prevProgram;
		}
	}
}

void SemanticAnalyser::visit(parser::ASTDeclarationNode* decl) {
	// current scope is the scope at the back
	SemanticScope* currentScope = scopes.back();

	// if variable already declared, throw error
	if (currentScope->alreadyDeclared(decl->identifier)) {
		throw std::runtime_error("Variable redeclaration on line " + std::to_string(decl->lineNumber) + ". '" + decl->identifier + "' was already declared in this scope on line " + std::to_string(currentScope->declaration_line(decl->identifier)) + ".");
	}

	// visit the expression to update current type
	decl->expr->accept(this);

	// allow mismatched type in the case of declaration of int to real
	if (decl->type == parser::TYPE::FLOAT && currentExpressionType == parser::TYPE::INT) {
		currentScope->declare(decl->identifier, parser::TYPE::FLOAT, decl->lineNumber);
	}
	else if (decl->type == currentExpressionType) { // types match
		currentScope->declare(decl->identifier, decl->type, decl->lineNumber);
	}
	else { // types don't match
		throw std::runtime_error("Found " + typeStr(currentExpressionType) + " on line " + std::to_string(decl->lineNumber) + " in definition of '" + decl->identifier + "', expected " + typeStr(decl->type) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* assign) {
	// determine the inner-most scope in which the value is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(assign->identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error("Identifier '" + assign->identifier + "' being reassigned on line " + std::to_string(assign->lineNumber) + " was never declared " + ((scopes.size() == 1) ? "globally." : "in this scope."));
		}
	}

	// get the type of the originally declared variable
	parser::TYPE type = scopes[i]->type(assign->identifier);

	// visit the expression to update current type
	assign->expr->accept(this);

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::TYPE::FLOAT && currentExpressionType == parser::TYPE::INT) {}
	// otherwise throw error
	else if (currentExpressionType != type) {
		throw std::runtime_error("Mismatched type for '" + assign->identifier + "' on line " + std::to_string(assign->lineNumber) + ". Expected " + typeStr(type) + ", found " + typeStr(currentExpressionType) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTPrintNode* print) {
	// update current expression
	print->expr->accept(this);
}

void SemanticAnalyser::visit(parser::ASTReadNode* read) { }

void SemanticAnalyser::visit(parser::ASTFunctionCallNode* func) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;

	// for each parameter,
	for (auto param : func->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);
	}

	// make sure the function exists in some scope i
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); i--) {
		if (i <= 0) {
			std::string funcName = func->identifier + "(";
			bool hasParams = false;
			for (auto param : signature) {
				hasParams = true;
				funcName += typeStr(param) + ", ";
			}
			funcName.pop_back();   // remove last whitespace
			funcName.pop_back();   // remove last comma
			funcName += ")";
			throw std::runtime_error("Function '" + funcName + "' appearing on line " + std::to_string(func->lineNumber) + " was never declared " + ((scopes.size() == 1) ? "globally." : "in this scope."));
		}
	}

	// set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTReturnNode* ret) {
	// Update current expression
	ret->expr->accept(this);

	// If we are not global, check that we return current function return type
	if (!functions.empty() && currentExpressionType != functions.top()) {
		throw std::runtime_error("Invalid return type on line " + std::to_string(ret->lineNumber) + ". Expected " + typeStr(functions.top()) + ", found " + typeStr(currentExpressionType) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTBlockNode* block) {
	// create new scope
	scopes.push_back(new SemanticScope());

	// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
	for (auto param : currentFunctionParameters) {
		scopes.back()->declare(param.first, param.second, block->lineNumber);
	}

	// clear the global function parameters vector
	currentFunctionParameters.clear();

	// visit each statement in the block
	for (auto& stmt : block->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTIfNode* ifNode) {
	// set current type to while expression
	ifNode->condition->accept(this);

	// make sure it is boolean
	if (currentExpressionType != parser::TYPE::BOOL) {
		throw std::runtime_error("Invalid if-condition on line " + std::to_string(ifNode->lineNumber) + ", expected boolean expression.");
	}

	// check the if block
	ifNode->ifBlock->accept(this);

	// if there is an else block, check it too
	if (ifNode->elseBlock) {
		ifNode->elseBlock->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTWhileNode* whileNode) {
	// Set current type to while expression
	whileNode->condition->accept(this);

	// Make sure it is boolean
	if (currentExpressionType != parser::TYPE::BOOL)
		throw std::runtime_error("Invalid while-condition on line " + std::to_string(whileNode->lineNumber)
			+ ", expected boolean expression.");

	// Check the while block
	whileNode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* func) {
	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->alreadyDeclared(func->identifier, func->signature)) {
			// determine line number of error and the corresponding function signature
			int line = scope->declaration_line(func->identifier, func->signature);
			std::string signature = "(";
			bool has_params = false;
			for (auto param : func->signature) {
				has_params = true;
				signature += typeStr(param) + ", ";
			}
			signature.pop_back();   // remove last whitespace
			signature.pop_back();   // remove last comma
			signature += ")";


			throw std::runtime_error("Error on line " + std::to_string(func->lineNumber) + ". Function " + func->identifier + signature + " already defined on line " + std::to_string(line) + ".");
		}
	}

	// add function to symbol table
	scopes.back()->declare(func->identifier, func->type, func->signature, func->lineNumber);

	// push current function type onto function stack
	functions.push(func->type);

	// empty and update current function parameters vector
	currentFunctionParameters.clear();
	currentFunctionParameters = func->parameters;

	// check semantics of function block by visiting nodes
	func->block->accept(this);

	// check that the function body returns
	if (!returns(func->block) && func->type != parser::TYPE::VOID) {
		throw std::runtime_error("Function " + func->identifier + " defined on line " + std::to_string(func->lineNumber) + " is not guaranteed to " + "return a value.");
	}

	// end the current function
	functions.pop();
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<__int64_t>*) {
	currentExpressionType = parser::TYPE::INT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<long double>*) {
	currentExpressionType = parser::TYPE::FLOAT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<bool>*) {
	currentExpressionType = parser::TYPE::BOOL;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<std::string>*) {
	currentExpressionType = parser::TYPE::STRING;
}

void SemanticAnalyser::visit(parser::ASTBinaryExprNode* bin) {
	// operator
	std::string op = bin->op;

	// visit left node first
	bin->left->accept(this);
	parser::TYPE l_type = currentExpressionType;

	// then right node
	bin->right->accept(this);
	parser::TYPE r_type = currentExpressionType;

	// these only work for int/float
	if (op == "*" || op == "/" || op == "-" || op == "%") {
		if ((l_type != parser::TYPE::INT && l_type != parser::TYPE::FLOAT) || (r_type != parser::TYPE::INT && r_type != parser::TYPE::FLOAT)) {
			throw std::runtime_error("Expected numerical operands for '" + op + "' operator on line " + std::to_string(bin->lineNumber) + ".");
		}

		// if both int, then expression is int, otherwise float
		currentExpressionType = (l_type == parser::TYPE::INT && r_type == parser::TYPE::INT) ? parser::TYPE::INT : parser::TYPE::FLOAT;
	}
	else if (op == "+") {
		// + works for all types except bool
		if (l_type == parser::TYPE::BOOL || r_type == parser::TYPE::BOOL) {
			throw std::runtime_error("Invalid operand for '+' operator, expected numerical or string" " operand on line " + std::to_string(bin->lineNumber) + ".");
		}
		if (l_type == parser::TYPE::STRING && r_type == parser::TYPE::STRING) { // If both string, no error
			currentExpressionType = parser::TYPE::STRING;
		}
		else if (l_type == parser::TYPE::STRING || r_type == parser::TYPE::STRING) { // only one is string, error
			throw std::runtime_error("Mismatched operands for '+' operator, found " + typeStr(l_type) + " on the left, but " + typeStr(r_type) + " on the right (line " + std::to_string(bin->lineNumber) + ").");
		}
		else { // real/int possibilities remain. If both int, then result is int, otherwise result is real
			currentExpressionType = (l_type == parser::TYPE::INT && r_type == parser::TYPE::INT) ? parser::TYPE::INT : parser::TYPE::FLOAT;
		}
	}
	else if (op == "and" || op == "or") {
		// and/or only work for bool
		if (l_type == parser::TYPE::BOOL && r_type == parser::TYPE::BOOL) {
			currentExpressionType = parser::TYPE::BOOL;
		}
		else {
			throw std::runtime_error("Expected two boolean-type operands for '" + op + "' operator " + "on line " + std::to_string(bin->lineNumber) + ".");
		}
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
		// rel-ops only work for numeric types
		if ((l_type != parser::TYPE::FLOAT && l_type != parser::TYPE::INT) || (r_type != parser::TYPE::FLOAT && r_type != parser::TYPE::INT)) {
			throw std::runtime_error("Expected two numerical operands for '" + op + "' operator " + "on line " + std::to_string(bin->lineNumber) + ".");
		}
		currentExpressionType = parser::TYPE::BOOL;
	}
	else if (op == "==" || op == "!=") {
		// == and != only work for like types
		if (l_type != r_type && (l_type != parser::TYPE::FLOAT || r_type != parser::TYPE::INT) && (l_type != parser::TYPE::INT || r_type != parser::TYPE::FLOAT)) {
			throw std::runtime_error("Expected arguments of the same type '" + op + "' operator " + "on line " + std::to_string(bin->lineNumber) + ".");
		}
		currentExpressionType = parser::TYPE::BOOL;
	}
	else {
		throw std::runtime_error("Unhandled semantic error in binary operator.");
	}
}

void SemanticAnalyser::visit(parser::ASTIdentifierNode* id) {
	// determine the inner-most scope in which the value is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(id->identifier); i--) {
		if (i <= 0) {
			throw std::runtime_error("Identifier '" + id->identifier + "' appearing on line " + std::to_string(id->lineNumber) + " in '" + currentProgram->name + "' was never declared " + ((scopes.size() == 1) ? "globally." : "in this scope."));
		}
	}

	// update current expression type
	currentExpressionType = scopes[i]->type(id->identifier);
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* un) {
	// determine expression type
	un->expr->accept(this);

	// handle different cases
	switch (currentExpressionType) {
	case parser::TYPE::INT:
	case parser::TYPE::FLOAT:
		if (un->unaryOp != "+" && un->unaryOp != "-") {
			throw std::runtime_error("Operator '" + un->unaryOp + "' in front of numerical " + "expression on line " + std::to_string(un->lineNumber) + ".");
		}
		break;
	case parser::TYPE::BOOL:
		if (un->unaryOp != "not") {
			throw std::runtime_error("Operator '" + un->unaryOp + "' in front of boolean " + "expression on line " + std::to_string(un->lineNumber) + ".");
		}
		break;
	default:
		throw std::runtime_error("Incompatible unary operator '" + un->unaryOp + "' in front of " + "expression on line " + std::to_string(un->lineNumber) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTExprFunctionCallNode* func) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;

	// for each parameter,
	for (auto param : func->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);
	}

	// make sure the function exists in some scope i
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); i--) {
		if (i <= 0) {
			std::string func_name = func->identifier + "(";
			bool has_params = false;
			for (auto param : signature) {
				has_params = true;
				func_name += typeStr(param) + ", ";
			}
			func_name.pop_back();   // remove last whitespace
			func_name.pop_back();   // remove last comma
			func_name += ")";
			throw std::runtime_error("Function '" + func_name + "' appearing on line " + std::to_string(func->lineNumber) + " was never declared " + ((scopes.size() == 1) ? "globally." : "in this scope."));
		}
	}

	// set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTFloatParseNode* float_parser) {
	currentExpressionType = parser::TYPE::FLOAT;
}

void SemanticAnalyser::visit(parser::ASTIntParseNode* int_parser) {
	currentExpressionType = parser::TYPE::INT;
}

void SemanticAnalyser::visit(parser::ASTStringParseNode* str_parser) {
	currentExpressionType = parser::TYPE::STRING;
}

void SemanticAnalyser::visit(parser::ASTExprReadNode* read) {
	if (currentExpressionType != parser::TYPE::STRING) {
		throw std::runtime_error("Function 'read()' appearing on line " + std::to_string(read->lineNumber) + " is trying to assing an invalid type.");
	}
}

std::string typeStr(parser::TYPE t) {
	switch (t) {
	case parser::TYPE::VOID:
		return "void";
	case parser::TYPE::INT:
		return "int";
	case parser::TYPE::FLOAT:
		return "real";
	case parser::TYPE::BOOL:
		return "bool";
	case parser::TYPE::STRING:
		return "string";
	default:
		throw std::runtime_error("Invalid type encountered.");
	}
}

// determines whether a statement definitely returns or not
bool SemanticAnalyser::returns(parser::ASTStatementNode* stmt) {
	// base case: if the statement is a return statement, then it definitely returns
	if (dynamic_cast<parser::ASTReturnNode*>(stmt)) {
		return true;
	}

	// for a block, if at least one statement returns, then the block returns
	if (auto block = dynamic_cast<parser::ASTBlockNode*>(stmt)) {
		for (auto& blk_stmt : block->statements) {
			if (returns(blk_stmt)) {
				return true;
			}
		}
	}

	// an if-(else) block returns only if both the if and the else statement return.
	if (auto ifstmt = dynamic_cast<parser::ASTIfNode*>(stmt)) {
		if (ifstmt->elseBlock) {
			return (returns(ifstmt->ifBlock) && returns(ifstmt->elseBlock));
		}
	}

	// a while block returns if its block returns
	if (auto whilestmt = dynamic_cast<parser::ASTWhileNode*>(stmt)) {
		return returns(whilestmt->block);
	}

	// other statements do not return
	else return false;
}
