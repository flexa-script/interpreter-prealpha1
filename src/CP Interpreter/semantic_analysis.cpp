#include <utility>
#include <iostream>

#include "semantic_analysis.h"
#include "util.h"


using namespace visitor;


parser::StructureDefinition_t SemanticScope::findDeclaredStructureDefinition(std::string identifier) {
	for (auto structure : structures) {
		if (structure.identifier == identifier) {
			return structure;
		}
	}
	throw std::runtime_error("can't found '" + identifier + "' type.");
}

bool SemanticScope::alreadyDeclaredStructureType(std::string identifier) {
	for (auto variable : structures) {
		if (variable.identifier == identifier) {
			return true;
		}
	}
	return false;
}

bool SemanticScope::alreadyDeclared(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return true;
		}
	}
	return false;
}

void SemanticScope::changeVarTypeName(std::string identifier, std::string typeName) {
	std::vector<parser::VariableDefinition_t> newVariableSymbolTable;
	for (size_t i = 0; i < variableSymbolTable.size(); ++i) {
		parser::VariableDefinition_t variable = variableSymbolTable[i];
		if (variable.identifier == identifier) {
			variable.typeName = typeName;
		}
		newVariableSymbolTable.push_back(variable);
	}
	variableSymbolTable = newVariableSymbolTable;
}

void SemanticScope::changeVarType(std::string identifier, parser::TYPE type) {
	std::vector<parser::VariableDefinition_t> newVariableSymbolTable;
	for (size_t i = 0; i < variableSymbolTable.size(); ++i) {
		parser::VariableDefinition_t variable = variableSymbolTable[i];
		if (variable.identifier == identifier) {
			variable.type = type;
		}
		newVariableSymbolTable.push_back(variable);
	}
	variableSymbolTable = newVariableSymbolTable;
}

bool SemanticScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature) {
	std::vector<parser::FunctionDefinition_t> funcs = std::vector<parser::FunctionDefinition_t>();

	for (auto fun : functionSymbolTable) {
		if (fun.identifier == identifier) {
			funcs.push_back(fun);
		}
	}

	// if key is not present in functionSymbolTable
	if (funcs.empty()) {
		return false;
	}

	// check signature for each function in functionSymbolTable
	for (auto fun : funcs) {
		auto funcSig = fun.signature;
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return true;
	}

	return false;
}

bool SemanticScope::isAnyVar(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.isAny;
		}
	}
	return false;
}

bool SemanticScope::isConst(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.isConst;
		}
	}
	return false;
}

void SemanticScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structures.push_back(type);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::string typeName, parser::TYPE arrayType, bool isAny, bool isConst, unsigned int row, unsigned int col) {
	parser::VariableDefinition_t var(identifier, type, typeName, arrayType, isAny, isConst, row, col);
	variableSymbolTable.push_back(var);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::string typeName, std::vector<parser::TYPE> signature, bool isAny, unsigned int row, unsigned int col) {
	parser::FunctionDefinition_t fun(identifier, type, typeName, signature, isAny, row, col);
	functionSymbolTable.push_back(fun);
	//functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(type, signature, row)));
}

parser::TYPE SemanticScope::arrayType(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.arrayType;
		}
	}

	throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' array");
}

std::string SemanticScope::typeName(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.typeName;
		}
	}

	throw std::runtime_error("something went wrong when determining the typename of '" + identifier + "' variable");
}

parser::TYPE SemanticScope::type(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.type;
		}
	}

	throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' variable");
}

std::string SemanticScope::typeName(std::string identifier, std::vector<parser::TYPE> signature) {
	std::vector<parser::FunctionDefinition_t> funcs = std::vector<parser::FunctionDefinition_t>();

	for (auto fun : functionSymbolTable) {
		if (fun.identifier == identifier) {
			funcs.push_back(fun);
		}
	}

	// if key is not present in functionSymbolTable
	if (funcs.empty()) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' function");
	}

	// check signature for each function in functionSymbolTable
	for (auto fun : funcs) {
		auto funcSig = fun.signature;
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return fun.typeName;
	}

	throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' function");
}

parser::TYPE SemanticScope::type(std::string identifier, std::vector<parser::TYPE> signature) {
	std::vector<parser::FunctionDefinition_t> funcs = std::vector<parser::FunctionDefinition_t>();

	for (auto fun : functionSymbolTable) {
		if (fun.identifier == identifier) {
			funcs.push_back(fun);
		}
	}

	// if key is not present in functionSymbolTable
	if (funcs.empty()) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' function");
	}

	// check signature for each function in functionSymbolTable
	for (auto fun : funcs) {
		auto funcSig = fun.signature;
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return fun.type;
	}

	throw std::runtime_error("something went wrong when determining the type of '" + identifier + "' function");
}

unsigned int SemanticScope::declarationLine(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.row;
		}
	}

	throw std::runtime_error("something went wrong when determining the line number of '" + identifier + "'");
}

unsigned int SemanticScope::declarationLine(std::string identifier, std::vector<parser::TYPE> signature) {
	std::vector<parser::FunctionDefinition_t> funcs = std::vector<parser::FunctionDefinition_t>();

	for (auto fun : functionSymbolTable) {
		if (fun.identifier == identifier) {
			funcs.push_back(fun);
		}
	}

	// if key is not present in functionSymbolTable
	if (funcs.empty()) {
		throw std::runtime_error("something went wrong when determining the line number of '" + identifier + "'");
	}

	// check signature for each function in functionSymbolTable
	for (auto fun : funcs) {
		auto funcSig = fun.signature;
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return fun.row;
	}

	// function with matching signature not found
	throw std::runtime_error("something went wrong when determining the line number of '" + identifier + "'");
}


std::vector<std::pair<std::string, std::string>> SemanticScope::functionList() {
	std::vector<std::pair<std::string, std::string>> list;

	// check signature for each function in functionSymbolTable
	for (auto fun : functionSymbolTable) {
		std::string func_name = fun.identifier + "(";
		bool has_params = false;
		for (auto param : fun.signature) {
			has_params = true;
			func_name += typeStr(param) + ", ";
		}
		func_name.pop_back();   // remove last whitespace
		func_name.pop_back();   // remove last comma
		func_name += ")";

		list.emplace_back(std::make_pair(func_name, typeStr(fun.type)));
	}

	return list;
}


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]) {
	// add global scope
	scopes.push_back(global_scope);
	currentExpressionType = parser::TYPE::T_ND;
	currentExpressionIsArray = false;
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
		throw std::runtime_error(msgHeader(decl->row, decl->col) + "variable '" + decl->identifier + "' already declared");
	}

	// visit the expression to update current type
	if (decl->expr) {
		decl->expr->accept(this);
	}

	// allow mismatched type in the case of declaration of int to float and char to string
	if (decl->type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT
		|| decl->type == parser::TYPE::T_STRING && currentExpressionType == parser::TYPE::T_CHAR) {
		currentScope->declare(decl->identifier, decl->type, "", decl->arrayType, false, decl->isConst, decl->row, decl->col);
	}
	// handle equal types and special types (any, struct and array)
	else if (decl->type == currentExpressionType || decl->type == parser::TYPE::T_ANY || decl->type == parser::TYPE::T_STRUCT || decl->type == parser::TYPE::T_ARRAY) { // types match
		// handle struct
		if (decl->type == parser::TYPE::T_STRUCT || currentExpressionType == parser::TYPE::T_STRUCT) {
			if (currentExpressionType == parser::TYPE::T_STRUCT && decl->type != parser::TYPE::T_ANY) {
				throw std::runtime_error(msgHeader(decl->row, decl->col) + "trying to assign struct '" + decl->identifier + "' (" + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + "), expected " + typeStr(decl->type) + (decl->typeName.empty() ? "" : " (" + decl->typeName + ")") + "");
			}

			parser::ASTLiteralNode<cp_struct>* strExpr = nullptr;

			// has expression to declare
			if (decl->expr) {
				if (currentExpressionType != parser::TYPE::T_STRUCT) {
					throw std::runtime_error(msgHeader(decl->row, decl->col) + "found " + typeStr(currentExpressionType) + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + " in definition of '" + decl->identifier + "', expected " + typeStr(decl->type) + (decl->typeName.empty() ? "" : " (" + decl->typeName + ")") + "");
				}
				//std::cout << typeid(parser::ASTLiteralNode<cp_struct>*).name() << std::endl << std::endl;
				//std::cout << typeid(decl->expr).name() << std::endl << std::endl;
				//if (typeid(parser::ASTLiteralNode<cp_struct>*) != typeid(decl->expr)) {
				//	throw std::runtime_error(msgHeader(decl->row, decl->col) + "found " + typeStr(currentExpressionType) + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + " in definition of '" + decl->identifier + "', expected " + typeStr(decl->type) + (decl->typeName.empty() ? "" : " (" + decl->typeName + ")") + "");
				//}
				strExpr = static_cast<parser::ASTLiteralNode<cp_struct>*>(decl->expr);
			}

			auto strType = decl->type;
			auto strTypeName = decl->typeName;
			// check is is any var
			if (decl->type == parser::TYPE::T_ANY) {
				strType = currentExpressionType;
				strTypeName = currentExpressionTypeName;
			}

			currentScope->declare(decl->identifier, strType, strTypeName, decl->arrayType, decl->type == parser::TYPE::T_ANY, decl->isConst, decl->row, decl->col);
			if (strExpr) {
				declareStructureTypeVariables(decl->identifier, currentExpressionTypeName, strExpr->val, strExpr);
			}

		}
		else if (decl->type == parser::TYPE::T_ARRAY) {
			currentScope->declare(decl->identifier, decl->type, decl->typeName, decl->arrayType, decl->arrayType == parser::TYPE::T_ANY, decl->isConst, decl->row, decl->col);
		}
		else {
			currentScope->declare(decl->identifier, currentExpressionType, decl->typeName, decl->arrayType, decl->type == parser::TYPE::T_ANY, decl->isConst, decl->row, decl->col);
		}
	}
	else { // types don't match
		throw std::runtime_error(msgHeader(decl->row, decl->col) + "found " + typeStr(currentExpressionType) + " in definition of '" + decl->identifier + "', expected " + typeStr(decl->type) + ".");
	}
}

void SemanticAnalyser::declareStructureTypeVariables(std::string identifier, std::string typeName, cp_struct str, parser::ASTLiteralNode<cp_struct>* expr) {
	cp_struct_values values = str.second;
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureType(typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(typeName);
	//auto typeStruct = findDeclaredStructureType(typeName);

	for (auto const& strValue : values) {
		auto currentIdentifier = identifier + '.' + strValue.first;

		bool found = false;
		for (auto varTypeStruct : typeStruct.variables) {
			if (varTypeStruct.identifier == strValue.first) {
				found = true;
			}
		}
		if (!found) throw std::runtime_error(msgHeader(expr->row, expr->col) + "'" + strValue.first + "' is not a member of '" + typeName + "'");

		switch (strValue.second->actualType) {
		case parser::TYPE::T_STRUCT:
			scopes.back()->declare(currentIdentifier, strValue.second->actualType, strValue.second->str.first, parser::TYPE::T_ND, false, false, expr->row, expr->col);
			declareStructureTypeVariables(currentIdentifier, strValue.second->str.first, strValue.second->str, expr);
		default:
			scopes.back()->declare(currentIdentifier, strValue.second->actualType, "", parser::TYPE::T_ND, false, false, expr->row, expr->col);
			break;
		}
	}
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* assign) {
	std::string actualIdentifier = assign->identifier;
	if (assign->identifierVector.size() > 1) {
		actualIdentifier = axe::join(assign->identifierVector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(actualIdentifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msgHeader(assign->row, assign->col) + "identifier '" + actualIdentifier + "' being reassigned was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	if (scopes[i]->isConst(actualIdentifier)) {
		throw std::runtime_error(msgHeader(assign->row, assign->col) + "'" + actualIdentifier + "' constant being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
	}

	// get the type of the originally declared variable
	parser::TYPE type = scopes[i]->type(actualIdentifier);

	auto isAnyVariable = scopes[i]->isAnyVar(actualIdentifier);

	// visit the expression to update current type
	assign->expr->accept(this);

	if (type == parser::TYPE::T_ARRAY) {
		if (currentExpressionIsArray) {
			throw std::runtime_error(msgHeader(assign->row, assign->col) + "'" + actualIdentifier + "' array being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
		type = scopes[i]->arrayType(actualIdentifier);
	}

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT ||
		type == parser::TYPE::T_INT && currentExpressionType == parser::TYPE::T_FLOAT) {
	}
	else if (isAnyVariable) {
		scopes[i]->changeVarType(actualIdentifier, currentExpressionType);
		scopes[i]->changeVarTypeName(actualIdentifier, currentExpressionTypeName);
	}
	else if (type == parser::TYPE::T_STRUCT && currentExpressionType == parser::TYPE::T_STRUCT) {
		auto typeName = scopes[i]->typeName(actualIdentifier);
		if (currentExpressionTypeName != typeName) {
			throw std::runtime_error(msgHeader(assign->row, assign->col) + "mismatched type for '" + actualIdentifier + "' struct. Expected '" + typeName + "', found '" + currentExpressionTypeName + "'");
		}
	}
	// otherwise throw error
	else if (currentExpressionType != type) {
		throw std::runtime_error(msgHeader(assign->row, assign->col) + "mismatched type for '" + actualIdentifier + "'. Expected " + typeStr(type) + ", found " + typeStr(currentExpressionType) + "");
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
	size_t i;
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
			throw std::runtime_error(msgHeader(func->row, func->col) + "function '" + funcName + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, signature);
	currentExpressionTypeName = scopes[i]->typeName(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTReturnNode* ret) {
	// update current expression
	ret->expr->accept(this);

	// if we are not global, check that we return current function return type
	if (!functions.empty() && currentExpressionType != functions.top()) {
		throw std::runtime_error(msgHeader(ret->row, ret->col) + "invalid return type. Expected " + typeStr(functions.top()) + ", found " + typeStr(currentExpressionType) + ".");
	}
}

//cp_struct visitor::Interpreter::redeclareStructureTypeVariables(std::string identifier, cp_struct str) {
//	cp_struct rstr;
//	rstr.first = str.first;
//	rstr.second = cp_struct_values();

//	for (size_t i = 0; i < str.second.size(); ++i) {
//		cp_struct_value variable = str.second.at(i);
//		auto currentIdentifier = identifier + '.' + variable.first;
//		if (variable.second->currentType == parser::TYPE::T_STRUCT) {
//			cp_struct subStr = redeclareStructureTypeVariables(currentIdentifier, variable.second->str);
//			Value_t* val = scopes.back()->declare(currentIdentifier, subStr, scopes);
//			rstr.second.push_back(cp_struct_value(variable.first, val));
//		}
//		else {
//			Value_t* val;
//			switch (variable.second->currentType) {
//			case parser::TYPE::T_BOOL:
//				val = scopes.back()->declare(currentIdentifier, variable.second->b);
//				break;
//			case parser::TYPE::T_INT:
//				val = scopes.back()->declare(currentIdentifier, variable.second->i);
//				break;
//			case parser::TYPE::T_FLOAT:
//				val = scopes.back()->declare(currentIdentifier, variable.second->f);
//				break;
//			case parser::TYPE::T_CHAR:
//				val = scopes.back()->declare(currentIdentifier, variable.second->c);
//				break;
//			case parser::TYPE::T_STRING:
//				val = scopes.back()->declare(currentIdentifier, variable.second->s);
//				break;
//			case parser::TYPE::T_ANY:
//				val = scopes.back()->declare(currentIdentifier, variable.second->a);
//				break;
//			case parser::TYPE::T_ARRAY:
//				val = scopes.back()->declare(currentIdentifier, variable.second->arr);
//				break;
//			}
//			rstr.second.push_back(cp_struct_value(variable.first, val));
//		}
//	}
//	return rstr;
//}

void SemanticAnalyser::visit(parser::ASTBlockNode* block) {
	// create new scope
	scopes.push_back(new SemanticScope());

	// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
	for (auto param : currentFunctionParameters) {
		if (param.type == parser::TYPE::T_STRUCT) {
			scopes.back()->declare(param.identifier, param.type, param.typeName, param.arrayType, param.isAny, param.isConst, param.row, param.col);
			//redeclareStructureTypeVariables(param.identifier, param.typeName);
		}
		else {
			scopes.back()->declare(param.identifier, param.type, param.typeName, param.arrayType, param.isAny, param.isConst, param.row, param.col);
		}
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
	if (currentExpressionType != parser::TYPE::T_BOOL) {
		throw std::runtime_error(msgHeader(ifNode->row, ifNode->col) + "invalid if-condition, expected boolean expression.");
	}

	// check the if block
	ifNode->ifBlock->accept(this);

	// if there is an else block, check it too
	if (ifNode->elseBlock) {
		ifNode->elseBlock->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTWhileNode* whileNode) {
	// set current type to while expression
	whileNode->condition->accept(this);

	// make sure it is boolean
	if (currentExpressionType != parser::TYPE::T_BOOL)
		throw std::runtime_error(msgHeader(whileNode->row, whileNode->col) + "invalid while-condition, expected boolean expression.");

	// check the while block
	whileNode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* func) {
	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->alreadyDeclared(func->identifier, func->signature)) {
			// determine line number of error and the corresponding function signature
			int line = scope->declarationLine(func->identifier, func->signature);
			std::string signature = "(";
			bool has_params = false;
			for (auto param : func->signature) {
				has_params = true;
				signature += typeStr(param) + ", ";
			}
			signature.pop_back();   // remove last whitespace
			signature.pop_back();   // remove last comma
			signature += ")";


			throw std::runtime_error(msgHeader(func->row, func->col) + "error: function " + func->identifier + signature + " already defined.");
		}
	}

	// add function to symbol table
	scopes.back()->declare(func->identifier, func->type, func->typeName, func->signature, func->type == parser::TYPE::T_ANY, func->row, func->row);

	// push current function type into function stack
	functions.push(func->type);

	// empty and update current function parameters vector
	currentFunctionParameters.clear();
	currentFunctionParameters = func->parameters;

	// check semantics of function block by visiting nodes
	func->block->accept(this);

	// check that the function body returns
	if (!returns(func->block) && func->type != parser::TYPE::T_VOID) {
		throw std::runtime_error(msgHeader(func->row, func->col) + "defined function " + func->identifier + " is not guaranteed to return a value.");
	}

	// end the current function
	functions.pop();
}

void SemanticAnalyser::visit(parser::ASTStructDefinitionNode* structDef) {
	// first check that all enclosing scopes have not already defined the struct
	for (auto& scope : scopes) {
		if (scope->alreadyDeclaredStructureType(structDef->identifier)) {
			throw std::runtime_error(msgHeader(structDef->row, structDef->col) + "error: struct " + structDef->identifier + " already defined.");
		}
	}

	scopes.back()->declareStructureDefinition(structDef->identifier, structDef->variables, structDef->row, structDef->col);
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_bool>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_BOOL;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_int>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_INT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_float>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_FLOAT;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_char>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_CHAR;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_string>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_STRING;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_any>*) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_ANY;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_array>* litArr) {
	currentExpressionIsArray = true;
	determineArrayType(litArr->val);
}

void SemanticAnalyser::determineArrayType(cp_array arr) {
	if (arr.size() > 0) {
		Value_t* val = arr.at(0);
		if (val->currentType == parser::TYPE::T_ARRAY) {
			determineArrayType(val->arr);
		}
		else {
			currentExpressionType = val->currentType;
			if (currentExpressionType == parser::TYPE::T_STRUCT) {
				currentExpressionTypeName = val->str.first;
			}
		}
	}
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_struct>* litStr) {
	currentExpressionIsArray = false;
	currentExpressionType = parser::TYPE::T_ARRAY;
	currentExpressionTypeName = litStr->val.first;
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
		if ((l_type != parser::TYPE::T_INT && l_type != parser::TYPE::T_FLOAT) || (r_type != parser::TYPE::T_INT && r_type != parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "expected numerical operands for '" + op + "' operator.");
		}

		if (op == "%" && (l_type == parser::TYPE::T_FLOAT || r_type == parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "invalid operands to binary expression ('" + typeStr(l_type) + " and '" + typeStr(r_type) + "').");
		}

		// if both int, then expression is int, otherwise float
		currentExpressionType = (l_type == parser::TYPE::T_INT && r_type == parser::TYPE::T_INT) ? parser::TYPE::T_INT : parser::TYPE::T_FLOAT;
	}
	else if (op == "+") {
		// + works for all types except bool
		if (l_type == parser::TYPE::T_BOOL || r_type == parser::TYPE::T_BOOL) {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "invalid operand for '+' operator, expected numerical or string operand.");
		}
		if ((l_type == parser::TYPE::T_STRING || l_type == parser::TYPE::T_CHAR) && (r_type == parser::TYPE::T_STRING || r_type == parser::TYPE::T_CHAR)) { // If both string, no error
			currentExpressionType = (l_type == parser::TYPE::T_CHAR && r_type == parser::TYPE::T_CHAR) ? parser::TYPE::T_CHAR : parser::TYPE::T_STRING;
		}
		else if ((l_type == parser::TYPE::T_STRING || l_type == parser::TYPE::T_CHAR) || (r_type == parser::TYPE::T_STRING || r_type == parser::TYPE::T_CHAR)) { // only one is string, error
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "mismatched operands for '+' operator, found " + typeStr(l_type) + " on the left, but " + typeStr(r_type) + " on the right.");
		}
		else { // real/int possibilities remain. If both int, then result is int, otherwise result is real
			currentExpressionType = (l_type == parser::TYPE::T_INT && r_type == parser::TYPE::T_INT) ? parser::TYPE::T_INT : parser::TYPE::T_FLOAT;
		}
	}
	else if (op == "and" || op == "or") {
		// and/or only work for bool
		if (l_type == parser::TYPE::T_BOOL && r_type == parser::TYPE::T_BOOL) {
			currentExpressionType = parser::TYPE::T_BOOL;
		}
		else {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "expected two boolean-type operands for '" + op + "' operator.");
		}
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
		// rel-ops only work for numeric types
		if ((l_type != parser::TYPE::T_FLOAT && l_type != parser::TYPE::T_INT) || (r_type != parser::TYPE::T_FLOAT && r_type != parser::TYPE::T_INT)) {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "expected two numerical operands for '" + op + "' operator.");
		}
		currentExpressionType = parser::TYPE::T_BOOL;
	}
	else if (op == "==" || op == "!=") {
		// == and != only work for like types
		if (l_type != r_type && (l_type != parser::TYPE::T_FLOAT || r_type != parser::TYPE::T_INT) && (l_type != parser::TYPE::T_INT || r_type != parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(bin->row, bin->col) + "expected arguments of the same type '" + op + "' operator.");
		}
		currentExpressionType = parser::TYPE::T_BOOL;
	}
	else {
		throw std::runtime_error(msgHeader(bin->row, bin->col) + "unhandled semantic error in binary operator.");
	}
}

void SemanticAnalyser::visit(parser::ASTIdentifierNode* id) {
	std::string actualIdentifier = id->identifier;
	if (id->identifierVector.size() > 1) {
		actualIdentifier = axe::join(id->identifierVector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(actualIdentifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msgHeader(id->row, id->col) + "identifier '" + actualIdentifier + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// update current expression type
	currentExpressionType = scopes[i]->type(actualIdentifier);
	currentExpressionTypeName = scopes[i]->typeName(actualIdentifier);
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* un) {
	// determine expression type
	un->expr->accept(this);

	// handle different cases
	switch (currentExpressionType) {
	case parser::TYPE::T_INT:
	case parser::TYPE::T_FLOAT:
		if (un->unaryOp != "+" && un->unaryOp != "-") {
			throw std::runtime_error(msgHeader(un->row, un->col) + "operator '" + un->unaryOp + "' in front of numerical expression.");
		}
		break;
	case parser::TYPE::T_BOOL:
		if (un->unaryOp != "not") {
			throw std::runtime_error(msgHeader(un->row, un->col) + "operator '" + un->unaryOp + "' in front of boolean expression.");
		}
		break;
	default:
		throw std::runtime_error(msgHeader(un->row, un->col) + "incompatible unary operator '" + un->unaryOp + "' in front of expression.");
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
	size_t i;
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
			throw std::runtime_error(msgHeader(func->row, func->col) + "function '" + func_name + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(func->identifier, signature);
	currentExpressionTypeName = scopes[i]->typeName(func->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTTypeParseNode* typeParser) {
	typeParser->expr->accept(this);
	currentExpressionType = typeParser->type;
}

void SemanticAnalyser::visit(parser::ASTExprReadNode* read) {
	if (currentExpressionType != parser::TYPE::T_STRING) {
		throw std::runtime_error(msgHeader(read->row, read->col) + "function 'read()' is trying to assing an invalid type.");
	}
}

void SemanticAnalyser::visit(parser::ASTThisNode* read) {

}

std::string typeStr(parser::TYPE t) {
	switch (t) {
	case parser::TYPE::T_VOID:
		return "void";
	case parser::TYPE::T_BOOL:
		return "bool";
	case parser::TYPE::T_INT:
		return "int";
	case parser::TYPE::T_FLOAT:
		return "real";
	case parser::TYPE::T_CHAR:
		return "char";
	case parser::TYPE::T_STRING:
		return "string";
	case parser::TYPE::T_ANY:
		return "any";
	case parser::TYPE::T_STRUCT:
		return "struct";
	case parser::TYPE::T_ARRAY:
		return "array";
	default:
		throw std::runtime_error("Invalid type encountered.");
	}
}

std::string SemanticAnalyser::msgHeader(unsigned int row, unsigned int col) {
	return currentProgram->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
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
