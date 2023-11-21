#include <utility>
#include <iostream>

#include "semantic_analysis.h"


using namespace visitor;


bool SemanticScope::alreadyDeclared(std::string identifier)
{
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool SemanticScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature)
{
	auto funcs = functionSymbolTable.equal_range(identifier);

	// If key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0)
		return false;

	// Check signature for each function in multimap
	for (auto i = funcs.first; i != funcs.second; i++)
		if (std::get<1>(i->second) == signature)
			return true;

	// Function with matching signature not found
	return false;
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, unsigned int lineNumber)
{
	variableSymbolTable[identifier] = std::make_pair(type, lineNumber);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::vector<parser::TYPE> signature, unsigned int lineNumber)
{
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(type, signature, lineNumber)));
}

parser::TYPE SemanticScope::type(std::string identifier)
{
	if (alreadyDeclared(identifier))
		return variableSymbolTable[identifier].first;

	throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");
}

parser::TYPE SemanticScope::type(std::string identifier, std::vector<parser::TYPE> signature)
{
	auto funcs = functionSymbolTable.equal_range(identifier);

	// If key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0)
		throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");

	// Check signature for each
	for (auto i = funcs.first; i != funcs.second; i++)
		if (std::get<1>(i->second) == signature)
			return std::get<0>(i->second);

	// Function with matching signature not found
	throw std::runtime_error("Something went wrong when determining the type of '" + identifier + "'.");
}

unsigned int SemanticScope::declaration_line(std::string identifier)
{
	if (alreadyDeclared(identifier))
		return variableSymbolTable[std::move(identifier)].second;

	throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");
}

unsigned int SemanticScope::declaration_line(std::string identifier, std::vector<parser::TYPE> signature)
{
	auto funcs = functionSymbolTable.equal_range(identifier);

	// If key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0)
		throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");

	// Check signature for each
	for (auto i = funcs.first; i != funcs.second; i++)
		if (std::get<1>(i->second) == signature)
			return std::get<2>(i->second);

	// Function with matching signature not found
	throw std::runtime_error("Something went wrong when determining the line number of '" + identifier + "'.");
}


std::vector<std::pair<std::string, std::string>> SemanticScope::function_list()
{
	std::vector<std::pair<std::string, std::string>> list;

	for (auto func = functionSymbolTable.begin(), last = functionSymbolTable.end(); func != last; func = functionSymbolTable.upper_bound(func->first))
	{
		std::string func_name = func->first + "(";
		bool has_params = false;
		for (auto param : std::get<1>(func->second))
		{
			has_params = true;
			func_name += type_str(param) + ", ";
		}
		func_name.pop_back();   // remove last whitespace
		func_name.pop_back();   // remove last comma
		func_name += ")";

		list.emplace_back(std::make_pair(func_name, type_str(std::get<0>(func->second))));
	}

	return std::move(list);
}


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0])
{
	// Add global scope
	scopes.push_back(global_scope);
};

SemanticAnalyser::~SemanticAnalyser() = default;


void SemanticAnalyser::start() {
	visit(currentProgram);
}

void SemanticAnalyser::visit(parser::ASTProgramNode* prog)
{
	// For each statement, accept
	for (auto& statement : prog->statements)
		statement->accept(this);
}

void SemanticAnalyser::visit(parser::ASTUsingNode* usg)
{
	for (auto program : programs) {
		if (usg->library == program->name) {
			auto prev_program = currentProgram;
			currentProgram = program;
			start();
			currentProgram = prev_program;
		}
	}
}

void SemanticAnalyser::visit(parser::ASTDeclarationNode* decl)
{
	// Current scope is the scope at the back
	SemanticScope* current_scope = scopes.back();

	// If variable already declared, throw error
	if (current_scope->alreadyDeclared(decl->identifier))
		throw std::runtime_error("Variable redeclaration on line " + std::to_string(decl->lineNumber) + ". '" +
			decl->identifier + "' was already declared in this scope on line " +
			std::to_string(current_scope->declaration_line(decl->identifier)) + ".");

	// Visit the expression to update current type
	decl->expr->accept(this);

	// allow mismatched type in the case of declaration of int to real
	if (decl->type == parser::TYPE::FLOAT && currentExpressionType == parser::TYPE::INT)
		current_scope->declare(decl->identifier, parser::TYPE::FLOAT, decl->lineNumber);
	else if (decl->type == currentExpressionType) // types match
		current_scope->declare(decl->identifier, decl->type, decl->lineNumber);
	else // types don't match
		throw std::runtime_error("Found " + type_str(currentExpressionType) + " on line " +
			std::to_string(decl->lineNumber) + " in definition of '" +
			decl->identifier + "', expected " + type_str(decl->type) + ".");
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* assign)
{
	// Determine the inner-most scope in which the value is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(assign->identifier); i--)
		if (i <= 0)
			throw std::runtime_error("Identifier '" + assign->identifier + "' being reassigned on line " +
				std::to_string(assign->lineNumber) + " was never declared " +
				((scopes.size() == 1) ? "globally." : "in this scope."));


	// Get the type of the originally declared variable
	parser::TYPE type = scopes[i]->type(assign->identifier);

	// Visit the expression to update current type
	assign->expr->accept(this);

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::TYPE::FLOAT && currentExpressionType == parser::TYPE::INT) {}

	// otherwise throw error
	else if (currentExpressionType != type)
		throw std::runtime_error("Mismatched type for '" + assign->identifier + "' on line " +
			std::to_string(assign->lineNumber) + ". Expected " + type_str(type) +
			", found " + type_str(currentExpressionType) + ".");
}

void SemanticAnalyser::visit(parser::ASTPrintNode* print)
{
	// Update current expression
	print->expr->accept(this);
}

void SemanticAnalyser::visit(parser::ASTReadNode* read)
{

}

void SemanticAnalyser::visit(parser::ASTFunctionCallNode* func)
{
	// Determine the signature of the function
	std::vector<parser::TYPE> signature;

	// For each parameter,
	for (auto param : func->parameters)
	{
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);
	}

	// Make sure the function exists in some scope i
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); i--)
		if (i <= 0)
		{
			std::string func_name = func->identifier + "(";
			bool has_params = false;
			for (auto param : signature)
			{
				has_params = true;
				func_name += type_str(param) + ", ";
			}
			func_name.pop_back();   // remove last whitespace
			func_name.pop_back();   // remove last comma
			func_name += ")";
			throw std::runtime_error("Function '" + func_name + "' appearing on line " +
				std::to_string(func->lineNumber) + " was never declared " +
				((scopes.size() == 1) ? "globally." : "in this scope."));
		}

	// Set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTReturnNode* ret)
{
	// Update current expression
	ret->expr->accept(this);

	// If we are not global, check that we return current function return type
	if (!functions.empty() && currentExpressionType != functions.top())
		throw std::runtime_error("Invalid return type on line " + std::to_string(ret->lineNumber) +
			". Expected " + type_str(functions.top()) + ", found " +
			type_str(currentExpressionType) + ".");
}

void SemanticAnalyser::visit(parser::ASTBlockNode* block)
{
	// Create new scope
	scopes.push_back(new SemanticScope());

	// Check whether this is a function block by seeing if we have any current function
	// parameters. If we do, then add them to the current scope.
	for (auto param : currentFunctionParameters)
		scopes.back()->declare(param.first, param.second, block->lineNumber);

	// Clear the global function parameters vector
	currentFunctionParameters.clear();

	// Visit each statement in the block
	for (auto& stmt : block->statements)
		stmt->accept(this);

	// Close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTIfNode* ifnode)
{
	// Set current type to while expression
	ifnode->condition->accept(this);

	// Make sure it is boolean
	if (currentExpressionType != parser::TYPE::BOOL)
		throw std::runtime_error("Invalid if-condition on line " + std::to_string(ifnode->lineNumber)
			+ ", expected boolean expression.");

	// Check the if block
	ifnode->ifBlock->accept(this);

	// If there is an else block, check it too
	if (ifnode->elseBlock)
		ifnode->elseBlock->accept(this);

}

void SemanticAnalyser::visit(parser::ASTWhileNode* whilenode)
{
	// Set current type to while expression
	whilenode->condition->accept(this);

	// Make sure it is boolean
	if (currentExpressionType != parser::TYPE::BOOL)
		throw std::runtime_error("Invalid while-condition on line " + std::to_string(whilenode->lineNumber)
			+ ", expected boolean expression.");

	// Check the while block
	whilenode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* func) {
	// First check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->alreadyDeclared(func->identifier, func->signature)) {
			// Determine line number of error and the corresponding function signature
			int line = scope->declaration_line(func->identifier, func->signature);
			std::string signature = "(";
			bool has_params = false;
			for (auto param : func->signature) {
				has_params = true;
				signature += type_str(param) + ", ";
			}
			signature.pop_back();   // remove last whitespace
			signature.pop_back();   // remove last comma
			signature += ")";


			throw std::runtime_error("Error on line " + std::to_string(func->lineNumber) +
				". Function " + func->identifier + signature +
				" already defined on line " + std::to_string(line) + ".");
		}
	}

	// Add function to symbol table
	scopes.back()->declare(func->identifier, func->type, func->signature, func->lineNumber);

	// Push current function type onto function stack
	functions.push(func->type);

	// Empty and update current function parameters vector
	currentFunctionParameters.clear();
	currentFunctionParameters = func->parameters;

	// Check semantics of function block by visiting nodes
	func->block->accept(this);

	// Check that the function body returns
	if (!returns(func->block) && func->type != parser::TYPE::VOID)
		throw std::runtime_error("Function " + func->identifier + " defined on line " +
			std::to_string(func->lineNumber) + " is not guaranteed to " +
			"return a value.");

	// End the current function
	functions.pop();
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<__int64_t>*)
{
	currentExpressionType = parser::TYPE::INT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<long double>*)
{
	currentExpressionType = parser::TYPE::FLOAT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<bool>*)
{
	currentExpressionType = parser::TYPE::BOOL;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<std::string>*)
{
	currentExpressionType = parser::TYPE::STRING;
}

void SemanticAnalyser::visit(parser::ASTBinaryExprNode* bin)
{
	// Operator
	std::string op = bin->op;

	// Visit left node first
	bin->left->accept(this);
	parser::TYPE l_type = currentExpressionType;

	// Then right node
	bin->right->accept(this);
	parser::TYPE r_type = currentExpressionType;

	// These only work for int/float
	if (op == "*" || op == "/" || op == "-" || op == "%")
	{
		if ((l_type != parser::TYPE::INT && l_type != parser::TYPE::FLOAT) || (r_type != parser::TYPE::INT && r_type != parser::TYPE::FLOAT))
			throw std::runtime_error("Expected numerical operands for '" + op +
				"' operator on line " + std::to_string(bin->lineNumber) + ".");

		// If both int, then expression is int, otherwise float
		currentExpressionType = (l_type == parser::TYPE::INT && r_type == parser::TYPE::INT) ? parser::TYPE::INT : parser::TYPE::FLOAT;
	}
	else if (op == "+")
	{
		// + works for all types except bool
		if (l_type == parser::TYPE::BOOL || r_type == parser::TYPE::BOOL)
			throw std::runtime_error("Invalid operand for '+' operator, expected numerical or string"
				" operand on line " + std::to_string(bin->lineNumber) + ".");

		if (l_type == parser::TYPE::STRING && r_type == parser::TYPE::STRING) // If both string, no error
			currentExpressionType = parser::TYPE::STRING;
		else if (l_type == parser::TYPE::STRING || r_type == parser::TYPE::STRING) // only one is string, error
			throw std::runtime_error("Mismatched operands for '+' operator, found " + type_str(l_type) +
				" on the left, but " + type_str(r_type) + " on the right (line " +
				std::to_string(bin->lineNumber) + ").");
		else // real/int possibilities remain. If both int, then result is int, otherwise result is real
			currentExpressionType = (l_type == parser::TYPE::INT && r_type == parser::TYPE::INT) ?
			parser::TYPE::INT : parser::TYPE::FLOAT;
	}
	else if (op == "and" || op == "or")
	{
		// and/or only work for bool
		if (l_type == parser::TYPE::BOOL && r_type == parser::TYPE::BOOL)
			currentExpressionType = parser::TYPE::BOOL;
		else throw std::runtime_error("Expected two boolean-type operands for '" + op + "' operator " +
			"on line " + std::to_string(bin->lineNumber) + ".");
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=")
	{
		// rel-ops only work for numeric types
		if ((l_type != parser::TYPE::FLOAT && l_type != parser::TYPE::INT) ||
			(r_type != parser::TYPE::FLOAT && r_type != parser::TYPE::INT))
			throw std::runtime_error("Expected two numerical operands for '" + op + "' operator " +
				"on line " + std::to_string(bin->lineNumber) + ".");
		currentExpressionType = parser::TYPE::BOOL;
	}
	else if (op == "==" || op == "!=")
	{
		// == and != only work for like types
		if (l_type != r_type && (l_type != parser::TYPE::FLOAT || r_type != parser::TYPE::INT) &&
			(l_type != parser::TYPE::INT || r_type != parser::TYPE::FLOAT))
			throw std::runtime_error("Expected arguments of the same type '" + op + "' operator " +
				"on line " + std::to_string(bin->lineNumber) + ".");
		currentExpressionType = parser::TYPE::BOOL;
	}
	else
	{
		throw std::runtime_error("Unhandled semantic error in binary operator.");
	}
}

void SemanticAnalyser::visit(parser::ASTIdentifierNode* id)
{
	// Determine the inner-most scope in which the value is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(id->identifier); i--)
		if (i <= 0)
			throw std::runtime_error("Identifier '" + id->identifier + "' appearing on line " +
				std::to_string(id->lineNumber) + " in '" + currentProgram->name + "' was never declared " +
				((scopes.size() == 1) ? "globally." : "in this scope."));

	// Update current expression type
	currentExpressionType = scopes[i]->type(id->identifier);
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* un)
{
	// Determine expression type
	un->expr->accept(this);

	// Handle different cases
	switch (currentExpressionType)
	{
	case parser::TYPE::INT:
	case parser::TYPE::FLOAT:
		if (un->unaryOp != "+" && un->unaryOp != "-")
			throw std::runtime_error("Operator '" + un->unaryOp + "' in front of numerical " +
				"expression on line " + std::to_string(un->lineNumber) + ".");
		break;
	case parser::TYPE::BOOL:
		if (un->unaryOp != "not")
			throw std::runtime_error("Operator '" + un->unaryOp + "' in front of boolean " +
				"expression on line " + std::to_string(un->lineNumber) + ".");
		break;
	default:
		throw std::runtime_error("Incompatible unary operator '" + un->unaryOp + "' in front of " +
			"expression on line " + std::to_string(un->lineNumber) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTExprFunctionCallNode* func)
{
	// Determine the signature of the function
	std::vector<parser::TYPE> signature;

	// For each parameter,
	for (auto param : func->parameters)
	{
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);
	}

	// Make sure the function exists in some scope i
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(func->identifier, signature); i--) {
		if (i <= 0) {
			std::string func_name = func->identifier + "(";
			bool has_params = false;
			for (auto param : signature) {
				has_params = true;
				func_name += type_str(param) + ", ";
			}
			func_name.pop_back();   // remove last whitespace
			func_name.pop_back();   // remove last comma
			func_name += ")";
			throw std::runtime_error("Function '" + func_name + "' appearing on line " +
				std::to_string(func->lineNumber) + " was never declared " +
				((scopes.size() == 1) ? "globally." : "in this scope."));
		}
	}

	// Set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTFloatParseNode* float_parser)
{
	currentExpressionType = parser::TYPE::FLOAT;
}

void SemanticAnalyser::visit(parser::ASTIntParseNode* int_parser)
{
	currentExpressionType = parser::TYPE::INT;
}

void SemanticAnalyser::visit(parser::ASTStringParseNode* str_parser)
{
	currentExpressionType = parser::TYPE::STRING;
}

void SemanticAnalyser::visit(parser::ASTExprReadNode* read)
{
	if (currentExpressionType != parser::TYPE::STRING)
	{
		throw std::runtime_error("Function 'read()' appearing on line " +
			std::to_string(read->lineNumber) +
			" is trying to assing an invalid type.");
	}
}

std::string type_str(parser::TYPE t)
{
	switch (t)
	{
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

// Determines whether a statement definitely returns or not
bool SemanticAnalyser::returns(parser::ASTStatementNode* stmt)
{
	// Base case: if the statement is a return statement, then it definitely returns
	if (dynamic_cast<parser::ASTReturnNode*>(stmt))
		return true;

	// For a block, if at least one statement returns, then the block returns
	if (auto block = dynamic_cast<parser::ASTBlockNode*>(stmt))
		for (auto& blk_stmt : block->statements)
			if (returns(blk_stmt))
				return true;

	// An if-(else) block returns only if both the if and the else statement return.
	if (auto ifstmt = dynamic_cast<parser::ASTIfNode*>(stmt))
		if (ifstmt->elseBlock)
			return (returns(ifstmt->ifBlock) && returns(ifstmt->elseBlock));

	// A while block returns if its block returns
	if (auto whilestmt = dynamic_cast<parser::ASTWhileNode*>(stmt))
		return returns(whilestmt->block);

	// Other statements do not return
	else return false;
}
