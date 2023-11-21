#include <iostream>
#include <cmath>

#include "interpreter.h"


using namespace visitor;


InterpreterScope::InterpreterScope(std::string name) : name(name) {}


InterpreterScope::InterpreterScope() {
	name = "";
}

std::string InterpreterScope::getName() {
	return name;
}

bool InterpreterScope::alreadyDeclared(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool InterpreterScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// If key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		return false;
	}

	// Check signature for each function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<0>(i->second) == signature) {
			return true;
		}
	}

	// Function with matching signature not found
	return false;
}

void InterpreterScope::declare(std::string identifier, __int64_t intValue) {
	value_t value;
	value.i = intValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::INT, value);
}

void InterpreterScope::declare(std::string identifier, long double realValue) {
	value_t value;
	value.f = realValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::FLOAT, value);
}

void InterpreterScope::declare(std::string identifier, bool boolValue) {
	value_t value;
	value.b = boolValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::BOOL, value);
}

void InterpreterScope::declare(std::string identifier, std::string stringValue) {
	value_t value;
	value.s = stringValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::STRING, value);
}

void InterpreterScope::declare(std::string identifier, std::vector<parser::TYPE> signature, std::vector<std::string> variableNames, parser::ASTBlockNode* block) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(signature, variableNames, block)));
}

parser::TYPE InterpreterScope::typeof(std::string identifier) {
	return variableSymbolTable[identifier].first;
}

value_t InterpreterScope::valueof(std::string identifier) {
	return variableSymbolTable[identifier].second;
}

std::vector<std::string> InterpreterScope::variablenamesof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// Match given signature to function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<0>(i->second) == signature) {
			return std::get<1>(i->second);
		}
	}

}

parser::ASTBlockNode* InterpreterScope::blockof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// Match given signature to function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		if (std::get<0>(i->second) == signature) {
			return std::get<2>(i->second);
		}
	}

	return nullptr;
}

std::vector<std::tuple<std::string, std::string, std::string>> InterpreterScope::variableList() {
	std::vector<std::tuple<std::string, std::string, std::string>> list;

	for (auto const& var : variableSymbolTable) {
		switch (var.second.first) {
		case parser::TYPE::INT:
			list.emplace_back(std::make_tuple(var.first, "int", std::to_string(var.second.second.i)));
			break;
		case parser::TYPE::FLOAT:
			list.emplace_back(std::make_tuple(var.first, "float", std::to_string(var.second.second.f)));
			break;
		case parser::TYPE::BOOL:
			list.emplace_back(std::make_tuple(var.first, "bool", (var.second.second.b) ? "true" : "false"));
			break;
		case parser::TYPE::STRING:
			list.emplace_back(std::make_tuple(var.first, "string", var.second.second.s));
			break;
		}
	}

	return list;
}


Interpreter::Interpreter() {
	// add global scope
	scopes.push_back(new InterpreterScope());
	currentExpressionType = parser::TYPE::VOID;
	currentProgram = nullptr;
}

Interpreter::Interpreter(InterpreterScope* globalScope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]) {
	// add global scope
	scopes.push_back(globalScope);
	currentExpressionType = parser::TYPE::VOID;
}

Interpreter::~Interpreter() = default;

void Interpreter::start() {
	visit(currentProgram);
}

void visitor::Interpreter::visit(parser::ASTProgramNode* prog) {
	// for each statement, accept
	for (auto& statement : prog->statements) {
		statement->accept(this);
	}
}

void visitor::Interpreter::visit(parser::ASTUsingNode* usg) {
	for (auto program : programs) {
		if (usg->library == program->name) {
			auto prev_program = currentProgram;
			currentProgram = program;
			start();
			currentProgram = prev_program;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTDeclarationNode* decl) {
	// Visit expression to update current value/type
	decl->expr->accept(this);

	// Declare variable, depending on type
	switch (decl->type) {
	case parser::TYPE::INT:
		scopes.back()->declare(decl->identifier, currentExpressionValue.i);
		break;
	case parser::TYPE::FLOAT:
		if (currentExpressionType == parser::TYPE::INT)
			scopes.back()->declare(decl->identifier, (long double)currentExpressionValue.i);
		else
			scopes.back()->declare(decl->identifier, currentExpressionValue.f);
		break;
	case parser::TYPE::BOOL:
		scopes.back()->declare(decl->identifier, currentExpressionValue.b);
		break;
	case parser::TYPE::STRING:
		scopes.back()->declare(decl->identifier, currentExpressionValue.s);
		break;
	}
}

void visitor::Interpreter::visit(parser::ASTAssignmentNode* assign) {
	// Determine innermost scope in which variable is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(assign->identifier); i--);

	// Visit expression node to update current value/type
	assign->expr->accept(this);

	// Redeclare variable, depending on type
	switch (scopes[i]->typeof(assign->identifier)) {
	case parser::TYPE::INT:
		scopes[i]->declare(assign->identifier, currentExpressionValue.i);
		break;
	case parser::TYPE::FLOAT:
		if (currentExpressionType == parser::TYPE::INT)
			scopes[i]->declare(assign->identifier, (long double)currentExpressionValue.i);
		else
			scopes[i]->declare(assign->identifier, currentExpressionValue.f);
		break;
	case parser::TYPE::BOOL:
		scopes[i]->declare(assign->identifier, currentExpressionValue.b);
		break;
	case parser::TYPE::STRING:
		scopes[i]->declare(assign->identifier, currentExpressionValue.s);
		break;
	}
}

void visitor::Interpreter::visit(parser::ASTPrintNode* print) {
	// visit expression node to update current value/type
	print->expr->accept(this);

	// print, depending on type
	switch (currentExpressionType) {
	case parser::TYPE::INT:
		std::cout << currentExpressionValue.i;
		break;
	case parser::TYPE::FLOAT:
		std::cout << currentExpressionValue.f;
		break;
	case parser::TYPE::BOOL:
		std::cout << ((currentExpressionValue.b) ? "true" : "false");
		break;
	case parser::TYPE::STRING:
		std::cout << currentExpressionValue.s;
		break;
	}
}

void visitor::Interpreter::visit(parser::ASTReadNode* read) {
	std::string line;
	std::getline(std::cin, line);
}

void visitor::Interpreter::visit(parser::ASTFunctionCallNode* func) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;
	std::vector<std::pair<parser::TYPE, value_t>> currentFunctionArguments;

	// for each parameter,
	for (auto param : func->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);

		// add the current expr to the local vector of function arguments, to be
		// used in the creation of the function scope
		currentFunctionArguments.emplace_back(currentExpressionType, currentExpressionValue);
	}

	// update the global vector current_function_arguments
	for (auto arg : currentFunctionArguments) {
		this->currentFunctionArguments.push_back(arg);
	}

	// determine in which scope the function is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); --i);

	// populate the global vector of function parameter names, to be used in creation of
	// function scope
	currentFunctionParameters = scopes[i]->variablenamesof(func->identifier, signature);

	currentFunctionName = func->identifier;

	// visit the corresponding function block
	scopes[i]->blockof(func->identifier, signature)->accept(this);
}

void visitor::Interpreter::visit(parser::ASTReturnNode* ret) {
	// update current expression
	ret->expr->accept(this);
	for (long i = scopes.size() - 1; i >= 0; --i) {
		if (!scopes[i]->getName().empty()) {
			returnFromFunctionName = scopes[i]->getName();
			returnFromFunction = true;
			break;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTBlockNode* block) {
	// create new scope
	scopes.push_back(new InterpreterScope(currentFunctionName));

	// check whether this is a function block by seeing if we have any current function
	// parameters. If we do, then add them to the current scope.
	for (unsigned int i = 0; i < currentFunctionArguments.size(); i++) {
		switch (currentFunctionArguments[i].first) {
		case parser::TYPE::INT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second.i);
			break;
		case parser::TYPE::FLOAT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second.f);
			break;
		case parser::TYPE::BOOL:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second.b);
			break;
		case parser::TYPE::STRING:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second.s);
			break;
		}
	}

	// clear the global function parameter/argument vectors
	currentFunctionParameters.clear();
	currentFunctionArguments.clear();
	currentFunctionName = "";

	// visit each statement in the block
	for (auto& stmt : block->statements) {
		stmt->accept(this);
		if (returnFromFunction) {
			if (!returnFromFunctionName.empty() && returnFromFunctionName == scopes.back()->getName()) {
				returnFromFunctionName = "";
				returnFromFunction = false;
			}
			break;
		}
	}

	// Close scope
	scopes.pop_back();
}

void visitor::Interpreter::visit(parser::ASTIfNode* ifNode) {
	// Evaluate if condition
	ifNode->condition->accept(this);

	// Execute appropriate blocks
	if (currentExpressionValue.b) {
		ifNode->ifBlock->accept(this);
	}
	else {
		if (ifNode->elseBlock) {
			ifNode->elseBlock->accept(this);
		}
	}

}

void visitor::Interpreter::visit(parser::ASTWhileNode* whileNode) {
	// evaluate while condition
	whileNode->condition->accept(this);

	while (currentExpressionValue.b) {
		// execute block
		whileNode->block->accept(this);

		// re-evaluate while condition
		whileNode->condition->accept(this);
	}
}

void visitor::Interpreter::visit(parser::ASTFunctionDefinitionNode* func) {
	// add function to symbol table
	scopes.back()->declare(func->identifier, func->signature, func->variableNames, func->block);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<__int64_t>* lit) {
	value_t v;
	v.i = lit->val;
	currentExpressionType = parser::TYPE::INT;
	currentExpressionValue = std::move(v);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<long double>* lit) {
	value_t v;
	v.f = lit->val;
	currentExpressionType = parser::TYPE::FLOAT;
	currentExpressionValue = std::move(v);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<bool>* lit) {
	value_t v;
	v.b = lit->val;
	currentExpressionType = parser::TYPE::BOOL;
	currentExpressionValue = std::move(v);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<std::string>* lit) {
	value_t v;
	v.s = lit->val;
	currentExpressionType = parser::TYPE::STRING;
	currentExpressionValue = std::move(v);
}

void visitor::Interpreter::visit(parser::ASTBinaryExprNode* bin) {
	// operator
	std::string op = bin->op;

	// visit left node first
	bin->left->accept(this);
	parser::TYPE l_type = currentExpressionType;
	value_t l_value = currentExpressionValue;

	// then right node
	bin->right->accept(this);
	parser::TYPE r_type = currentExpressionType;
	value_t r_value = currentExpressionValue;

	// expression struct
	value_t v;

	// arithmetic operators for now
	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		// two ints
		if (l_type == parser::TYPE::INT && r_type == parser::TYPE::INT) {
			currentExpressionType = parser::TYPE::INT;
			if (op == "+") {
				v.i = l_value.i + r_value.i;
			}
			else if (op == "-") {
				v.i = l_value.i - r_value.i;
			}
			else if (op == "*") {
				v.i = l_value.i * r_value.i;
			}
			else if (op == "/") {
				if (r_value.i == 0) {
					throw std::runtime_error("Division by zero encountered on line " + std::to_string(bin->lineNumber) + ".");
				}
				v.i = l_value.i / r_value.i;
			}
			else if (op == "%") {
				v.i = l_value.i % r_value.i;
			}
		}
		else if (l_type == parser::TYPE::FLOAT || r_type == parser::TYPE::FLOAT) { // at least one real
			currentExpressionType = parser::TYPE::FLOAT;
			long double l = l_value.f, r = r_value.f;
			if (l_type == parser::TYPE::INT) {
				l = l_value.i;
			}
			if (r_type == parser::TYPE::INT) {
				r = r_value.i;
			}
			if (op == "+") {
				v.f = l + r;
			}
			else if (op == "-") {
				v.f = l - r;
			}
			else if (op == "*") {
				v.f = l * r;
			}
			else if (op == "/") {
				if (r == 0) {
					throw std::runtime_error("Division by zero encountered on line " + std::to_string(bin->lineNumber) + ".");
				}
				v.f = l / r;
			}
			else if (op == "%") {
				v.f = l * r;
			}
		}
		else { // remaining case is for strings
			currentExpressionType = parser::TYPE::STRING;
			v.s = l_value.s + r_value.s;
		}
	}
	else if (op == "and" || op == "or") { // now bool
		currentExpressionType = parser::TYPE::BOOL;
		if (op == "and") {
			v.b = l_value.b && r_value.b;
		}
		else if (op == "or") {
			v.b = l_value.b || r_value.b;
		}
	}
	else { // now Comparator Operators
		currentExpressionType = parser::TYPE::BOOL;
		if (l_type == parser::TYPE::BOOL) {
			v.b = (op == "==") ? l_value.b == r_value.b : l_value.b != r_value.b;
		}
		else if (l_type == parser::TYPE::STRING) {
			v.b = (op == "==") ? l_value.s == r_value.s : l_value.s != r_value.s;
		}
		else {
			long double l = l_value.f, r = r_value.f;

			if (l_type == parser::TYPE::INT) {
				l = l_value.i;
			}
			if (r_type == parser::TYPE::INT) {
				r = r_value.i;
			}
			if (op == "==") {
				v.b = l == r;
			}
			else if (op == "!=") {
				v.b = l != r;
			}
			else if (op == "<") {
				v.b = l < r;
			}
			else if (op == ">") {
				v.b = l > r;
			}
			else if (op == ">=") {
				v.b = l >= r;
			}
			else if (op == "<=") {
				v.b = l <= r;
			}
		}
	}

	// update current expression
	currentExpressionValue = v;
}

void visitor::Interpreter::visit(parser::ASTIdentifierNode* id) {
	// determine innermost scope in which variable is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(id->identifier); i--);

	// update current expression
	currentExpressionType = scopes[i]->typeof(id->identifier);
	currentExpressionValue = scopes[i]->valueof(id->identifier);
}

void visitor::Interpreter::visit(parser::ASTUnaryExprNode* un) {
	// update current expression
	un->expr->accept(this);
	switch (currentExpressionType) {
	case parser::TYPE::INT:
		if (un->unaryOp == "-")
			currentExpressionValue.i *= -1;
		break;
	case parser::TYPE::FLOAT:
		if (un->unaryOp == "-")
			currentExpressionValue.f *= -1;
		break;
	case parser::TYPE::BOOL:
		currentExpressionValue.b ^= 1;
	}
}

void visitor::Interpreter::visit(parser::ASTExprFunctionCallNode* func) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;
	std::vector<std::pair<parser::TYPE, value_t>> currentFunctionArguments;

	// for each parameter
	for (auto param : func->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);

		// add the current expr to the local vector of function arguments, to be
		// used in the creation of the function scope
		currentFunctionArguments.emplace_back(currentExpressionType, currentExpressionValue);
	}

	// update the global vector current_function_arguments
	for (auto arg : currentFunctionArguments) {
		this->currentFunctionArguments.push_back(arg);
	}

	// determine in which scope the function is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); i--);

	// populate the global vector of function parameter names, to be used in creation of function scope
	currentFunctionParameters = scopes[i]->variablenamesof(func->identifier, signature);

	currentFunctionName = func->identifier;

	// visit the corresponding function block
	scopes[i]->blockof(func->identifier, signature)->accept(this);
}

void visitor::Interpreter::visit(parser::ASTFloatParseNode* floatParser) {
	// visit expression node to update current value/type
	floatParser->expr->accept(this);

	// parse depending on type
	switch (currentExpressionType) {
	case parser::TYPE::INT:
		currentExpressionValue.f = static_cast<long double>(currentExpressionValue.i);
		break;
	case parser::TYPE::STRING:
		currentExpressionValue.f = std::stold(currentExpressionValue.s);
		break;
	}

	currentExpressionType = parser::TYPE::FLOAT;
}

void visitor::Interpreter::visit(parser::ASTIntParseNode* intParser)
{
	// visit expression node to update current value/type
	intParser->expr->accept(this);

	// parse depending on type
	switch (currentExpressionType) {
	case parser::TYPE::FLOAT:
		currentExpressionValue.i = static_cast<long long>(round(currentExpressionValue.f));
		break;
	case parser::TYPE::STRING:
		currentExpressionValue.i = std::stoll(currentExpressionValue.s);
		break;
	}

	currentExpressionType = parser::TYPE::INT;
}

void visitor::Interpreter::visit(parser::ASTStringParseNode* strParser)
{
	// visit expression node to update current value/type
	strParser->expr->accept(this);

	// parse depending on type
	switch (currentExpressionType) {
	case parser::TYPE::INT:
		currentExpressionValue.s = std::to_string(currentExpressionValue.i);
		break;
	case parser::TYPE::FLOAT:
		currentExpressionValue.s = std::to_string(currentExpressionValue.f);
		break;
	case parser::TYPE::BOOL:
		currentExpressionValue.s = currentExpressionValue.b ? "true" : "false";
		break;
	}

	currentExpressionType = parser::TYPE::STRING;
}

void visitor::Interpreter::visit(parser::ASTExprReadNode* read) {
	std::string line;
	std::getline(std::cin, line);

	currentExpressionValue.s = std::move(line);
}

std::pair<parser::TYPE, value_t> Interpreter::currentExpr() {
	return std::move(std::make_pair(currentExpressionType, currentExpressionValue));
};


std::string visitor::typeStr(parser::TYPE t) {
	switch (t) {
	case parser::TYPE::VOID:
		return "void";
	case parser::TYPE::INT:
		return "int";
	case parser::TYPE::FLOAT:
		return "float";
	case parser::TYPE::BOOL:
		return "bool";
	case parser::TYPE::STRING:
		return "string";
	default:
		throw std::runtime_error("Invalid type encountered.");
	}
}
