#include <utility>
#include <iostream>

#include "semantic_analysis.h"
#include "util.h"


using namespace visitor;


SemanticAnalyser::SemanticAnalyser(SemanticScope* global_scope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]) {
	// add global scope
	scopes.push_back(global_scope);
	currentExpressionType = parser::TYPE::T_ND;
	currentExpressionArrayType = parser::TYPE::T_ND;
	isFunctionDefinitionContext = false;
};

SemanticAnalyser::~SemanticAnalyser() = default;

void SemanticAnalyser::start() {
	visit(currentProgram);
}

void SemanticAnalyser::visit(parser::ASTProgramNode* astnode) {
	// for each statement, accept
	for (auto& statement : astnode->statements) {
		statement->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTUsingNode* astnode) {
	for (auto program : programs) {
		if (astnode->library == program->name) {
			auto prevProgram = currentProgram;
			currentProgram = program;
			start();
			currentProgram = prevProgram;
		}
	}
}

void SemanticAnalyser::visit(parser::ASTDeclarationNode* astnode) {
	// current scope is the scope at the back
	SemanticScope* currentScope = scopes.back();

	// if variable already declared, throw error
	if (currentScope->alreadyDeclaredVariable(astnode->identifier)) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "variable '" + astnode->identifier + "' already declared");
	}

	// visit the expression to update current type
	if (astnode->expr) {
		astnode->expr->accept(this);
	}
	else {
		currentExpressionType = parser::TYPE::T_NULL;
		currentExpressionArrayType = parser::TYPE::T_ND;
		currentExpressionTypeName = "";
	}

	if (astnode->type != parser::TYPE::T_ARRAY && currentExpressionType == parser::TYPE::T_ARRAY) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "trying to assign array expression to '" + astnode->identifier + "' " + parser::typeStr(astnode->type) + " variable");
	}

	// allow mismatched type in the case of declaration of int to float and char to string
	if (astnode->type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT
		|| astnode->type == parser::TYPE::T_STRING && currentExpressionType == parser::TYPE::T_CHAR) {
		currentScope->declare(astnode->identifier, astnode->type, astnode->typeName, astnode->arrayType, astnode->dim, astnode->arrayType == parser::TYPE::T_ANY, astnode->isConst, astnode->row, astnode->col);
	}
	// handle equal types and special types (any, struct and array)
	else if (astnode->type == currentExpressionType || astnode->type == parser::TYPE::T_ANY || astnode->type == parser::TYPE::T_STRUCT || astnode->type == parser::TYPE::T_ARRAY) { // types match
		// handle struct
		if (astnode->type == parser::TYPE::T_STRUCT || currentExpressionType == parser::TYPE::T_STRUCT) {
			if (currentExpressionType == parser::TYPE::T_STRUCT && astnode->type != parser::TYPE::T_STRUCT && astnode->type != parser::TYPE::T_ANY) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "trying to assign struct '" + astnode->identifier + "' (" + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + "), expected " + parser::typeStr(astnode->type) + (astnode->typeName.empty() ? "" : " (" + astnode->typeName + ")") + "");
			}

			parser::ASTLiteralNode<cp_struct>* strExpr = nullptr;

			// has expression to declare
			if (astnode->expr) {
				if (currentExpressionType != parser::TYPE::T_STRUCT && currentExpressionType != parser::TYPE::T_NULL) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "found " + parser::typeStr(currentExpressionType) + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + " in definition of '" + astnode->identifier + "', expected " + parser::typeStr(astnode->type) + (astnode->typeName.empty() ? "" : " (" + astnode->typeName + ")") + "");
				}
				strExpr = static_cast<parser::ASTLiteralNode<cp_struct>*>(astnode->expr);
			}

			auto strType = astnode->type;
			auto strTypeName = astnode->typeName;
			// check is is any var
			if (astnode->type == parser::TYPE::T_ANY) {
				strType = currentExpressionType;
				strTypeName = currentExpressionTypeName;
			}

			currentScope->declare(astnode->identifier, strType, strTypeName, astnode->arrayType, astnode->dim, astnode->type == parser::TYPE::T_ANY, astnode->isConst, astnode->row, astnode->col);
			if (strExpr && !isFunctionDefinitionContext) {
				declareStructureDefinitionVariables(astnode->identifier, currentExpressionTypeName, strExpr->val, strExpr);
			}
			declareStructureDefinitionFirstLevelVariables(astnode->identifier, strTypeName);
		}
		else if (astnode->type == parser::TYPE::T_ARRAY) {
			if (astnode->expr) {
				if (currentExpressionType != parser::TYPE::T_ARRAY && currentExpressionType != parser::TYPE::T_NULL) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected array expression assigning '" + astnode->identifier + "'");
				}
				auto val = dynamic_cast<parser::ASTLiteralNode<cp_array>*>(astnode->expr)->val;

				auto exprDim = calcArrayDimSize(std::any_cast<cp_array>(val));

				if (astnode->dim.size() != exprDim.size()) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid expression dimension assigning '" + astnode->identifier + "' array");
				}

				for (size_t dc = 0; dc < astnode->dim.size(); ++dc) {
					if (astnode->dim.at(dc) != -1 && astnode->dim.at(dc) != exprDim.at(dc)) {
						throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid expression size assigning '" + astnode->identifier + "' array");
					}
				}
			}
			else {

			}

			currentScope->declare(astnode->identifier, astnode->type, astnode->typeName, currentExpressionArrayType, astnode->dim, astnode->arrayType == parser::TYPE::T_ANY, astnode->isConst, astnode->row, astnode->col);
		}
		else {
			currentScope->declare(astnode->identifier, currentExpressionType, astnode->typeName, astnode->arrayType, astnode->dim, astnode->type == parser::TYPE::T_ANY, astnode->isConst, astnode->row, astnode->col);
		}
	}
	else if (currentExpressionType == parser::TYPE::T_NULL) {
		currentScope->declare(astnode->identifier, astnode->type, astnode->typeName, astnode->arrayType, astnode->dim, astnode->arrayType == parser::TYPE::T_ANY, astnode->isConst, astnode->row, astnode->col);
	}
	else { // types don't match
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "found " + parser::typeStr(currentExpressionType) + " in definition of '" + astnode->identifier + "', expected " + parser::typeStr(astnode->type) + ".");
	}
}

std::vector<int> SemanticAnalyser::calcArrayDimSize(cp_array value) {
	auto dim = std::vector<int>();
	Value_t* val = value.at(0);

	dim.push_back(value.size());
	if (val->currentType == parser::TYPE::T_ARRAY) {
		auto dim2 = calcArrayDimSize(val->arr);
		dim.insert(dim.end(), dim2.begin(), dim2.end());
	}

	return dim;
}

void SemanticAnalyser::declareStructureDefinitionFirstLevelVariables(std::string identifier, std::string typeName) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureDefinition(typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(typeName);

	for (auto varTypeStruct : typeStruct.variables) {
		auto currentIdentifier = identifier + '.' + varTypeStruct.identifier;
		scopes.back()->declare(currentIdentifier, varTypeStruct.type, varTypeStruct.typeName, varTypeStruct.arrayType, varTypeStruct.dim, varTypeStruct.isAny, varTypeStruct.isConst, varTypeStruct.row, varTypeStruct.col);
	}
}

void SemanticAnalyser::declareStructureDefinitionVariables(std::string identifier, std::string typeName, cp_struct str, parser::ASTLiteralNode<cp_struct>* expr) {
	cp_struct_values values = str.second;
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureDefinition(typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(typeName);

	for (auto const& strValue : values) {
		auto currentIdentifier = identifier + '.' + strValue.first;

		bool found = false;
		parser::VariableDefinition_t* varTypeStruct = nullptr;
		for (size_t i = 0; i < typeStruct.variables.size(); ++i) {
			varTypeStruct = &typeStruct.variables[i];
			if (varTypeStruct->identifier == strValue.first) {
				found = true;
				break;
			}
		}
		if (!found) throw std::runtime_error(msgHeader(expr->row, expr->col) + "'" + strValue.first + "' is not a member of '" + typeName + "'");

		if (strValue.second->actualType != parser::TYPE::T_NULL && strValue.second->actualType != varTypeStruct->type && !varTypeStruct->isAny) {
			throw std::runtime_error(msgHeader(expr->row, expr->col) + "invalid type " + parser::typeStr(strValue.second->actualType) + " trying to assign '" + currentIdentifier + "'");
		}

		if (strValue.second->actualType == parser::TYPE::T_STRUCT) {
			scopes.back()->declare(currentIdentifier, strValue.second->actualType, strValue.second->str.first, parser::TYPE::T_ND, varTypeStruct->dim, false, false, expr->row, expr->col);
			declareStructureDefinitionVariables(currentIdentifier, strValue.second->str.first, strValue.second->str, expr);
		}
		else {
			scopes.back()->declare(currentIdentifier, varTypeStruct->type, varTypeStruct->typeName, varTypeStruct->arrayType, varTypeStruct->dim, false, false, expr->row, expr->col);
		}
	}
}

parser::VariableDefinition_t SemanticAnalyser::findDeclaredVariable(std::string identifier) {
	int i;
	for (i = scopes.size() - 1; i >= 0 && !scopes[i]->alreadyDeclaredVariable(identifier); --i) {
		if (i <= 0) {
			i = -1;
			break;
		}
	}
	if (i >= 0) {
		for (auto variable : scopes.at(i)->getVariableSymbolTable()) {
			if (variable.identifier == identifier) {
				return variable;
			}
		}
	}

	if (axe::contains(identifier, ".")) {
		std::list<std::string> identifiers = axe::splitList(identifier, '.');
		parser::StructureDefinition_t strDef;
		std::string typeName = "";

		for (i = scopes.size() - 1; i >= 0 && !scopes[i]->alreadyDeclaredVariable(identifiers.front()); --i) {
			if (i <= 0) {
				i = -1;
				break;
			}
		}
		if (i >= 0) {
			for (auto variable : scopes.at(i)->getVariableSymbolTable()) {
				if (variable.identifier == identifiers.front()) {
					typeName = variable.typeName;
					break;
				}
			}
		}

		size_t i;
		for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureDefinition(typeName); --i);
		strDef = scopes[i]->findDeclaredStructureDefinition(typeName);

		identifiers.pop_front();

		while (identifiers.size() > 1) {
			typeName = "";

			for (auto variable : strDef.variables) {
				if (variable.identifier == identifiers.front()) {
					typeName = variable.typeName;
					break;
				}
			}

			size_t i;
			for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureDefinition(typeName); --i);
			strDef = scopes[i]->findDeclaredStructureDefinition(typeName);

			identifiers.pop_front();
		}

		for (size_t vari = 0; vari < strDef.variables.size(); ++vari) {
			if (strDef.variables.at(vari).identifier == identifiers.front()) {
				return strDef.variables.at(vari);
			}
		}
	}

	throw std::runtime_error("SERR: error: '" + identifier + "' variable not found");
}

std::string SemanticAnalyser::findTypeName(std::string identifier) {
	return findDeclaredVariable(identifier).typeName;
}

parser::TYPE SemanticAnalyser::findType(std::string identifier) {
	return findDeclaredVariable(identifier).type;
}

bool SemanticAnalyser::findAnyVar(std::string identifier) {
	return findDeclaredVariable(identifier).isAny;
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* astnode) {
	std::string actualIdentifier = astnode->identifier;
	if (astnode->identifierVector.size() > 1) {
		actualIdentifier = axe::join(astnode->identifierVector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredVariable(isFunctionDefinitionContext ? astnode->identifier : actualIdentifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "identifier '" + actualIdentifier + "' being reassigned was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	if (scopes[i]->isConst(actualIdentifier)) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + actualIdentifier + "' constant being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
	}

	parser::TYPE type;
	parser::TYPE arrtype;
	bool isAnyVariable;

	// get the type of the originally declared variable
	if (isFunctionDefinitionContext) {
		type = findType(actualIdentifier);
		isAnyVariable = findAnyVar(actualIdentifier);
	}
	else {
		type = scopes[i]->type(actualIdentifier);
		isAnyVariable = scopes[i]->findAnyVar(actualIdentifier);
	}

	// visit the expression to update current type
	astnode->expr->accept(this);

	if (type == parser::TYPE::T_ARRAY) {
		if (currentExpressionType == parser::TYPE::T_ARRAY) {
			parser::VariableDefinition_t var = scopes[i]->var(actualIdentifier);
			parser::ASTLiteralNode<cp_array>* strExpr = static_cast<parser::ASTLiteralNode<cp_array>*>(astnode->expr);
			try {
				cp_array arr = strExpr->val;
				std::vector<int> newDim = std::vector<int>();
				while (arr.size() > 0) {
					newDim.push_back(arr.size());
					if (arr[0]->actualType == parser::TYPE::T_ARRAY) {
						arr = arr[0]->arr;
					}
					else {
						break;
					}
				}
				if (var.dim.size() != newDim.size()) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid dimension trying assign '" + actualIdentifier + "' array");
				}
				for (size_t i = 0; i < var.dim.size(); ++i) {
					if (var.dim[i] != -1 && var.dim[i] != newDim[i]) {
						throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid size trying assign '" + actualIdentifier + "' array");
					}
				}
			}
			catch (...) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid assignment of '" + actualIdentifier + "' array");
			}
			//throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + actualIdentifier + "' array being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
		arrtype = scopes[i]->arrayType(actualIdentifier);

		if (astnode->accessVector.size() == 0 && arrtype != parser::TYPE::T_ANY && arrtype != currentExpressionArrayType) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "'. expected '" + parser::typeStr(arrtype) + "' array, found '" + parser::typeStr(currentExpressionArrayType) + "' array");
		}
	}

	if (astnode->accessVector.size() > 0) {
		type = arrtype;
	}

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT ||
		type == parser::TYPE::T_INT && currentExpressionType == parser::TYPE::T_FLOAT) {
	}
	else if (isAnyVariable) {
		scopes[i]->changeVarType(actualIdentifier, currentExpressionType);
		scopes[i]->changeVarTypeName(actualIdentifier, currentExpressionTypeName);
	}
	else if (type == parser::TYPE::T_STRUCT && (currentExpressionType == parser::TYPE::T_STRUCT || currentExpressionType == parser::TYPE::T_NULL)) {
		std::string typeName;
		if (isFunctionDefinitionContext) {
			typeName = findTypeName(actualIdentifier);
		}
		else {
			typeName = scopes[i]->typeName(actualIdentifier);
		}
		if (currentExpressionTypeName != typeName && currentExpressionType != parser::TYPE::T_NULL) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "' struct. Expected '" + typeName + "', found '" + currentExpressionTypeName + "'");
		}

		try {
			parser::ASTLiteralNode<cp_struct>* strExpr = static_cast<parser::ASTLiteralNode<cp_struct>*>(astnode->expr);

			if (typeid(strExpr->val) == typeid(cp_array)) {
				declareStructureDefinitionVariables(actualIdentifier, currentExpressionTypeName, strExpr->val, strExpr);
			}
			else {
				declareStructureDefinitionFirstLevelVariables(actualIdentifier, typeName);
			}
		}
		catch (...) {
			declareStructureDefinitionFirstLevelVariables(actualIdentifier, typeName);
		}
	}
	// otherwise throw error
	else if (currentExpressionType != type) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "'. Expected " + parser::typeStr(type) + ", found " + parser::typeStr(currentExpressionType) + "");
	}
}

void SemanticAnalyser::visit(parser::ASTPrintNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
}

void SemanticAnalyser::visit(parser::ASTReturnNode* ret) {
	// update current expression
	ret->expr->accept(this);

	// if we are not global, check that we return current function return type
	if (!functions.empty() && currentExpressionType != functions.top()) {
		throw std::runtime_error(msgHeader(ret->row, ret->col) + "invalid return type. Expected " + parser::typeStr(functions.top()) + ", found " + parser::typeStr(currentExpressionType) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTBlockNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

	// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
	for (auto param : currentFunctionParameters) {
		if (param.type == parser::TYPE::T_STRUCT) {
			scopes.back()->declare(param.identifier, param.type, param.typeName, param.arrayType, param.dim, param.isAny, param.isConst, param.row, param.col);
		}
		else {
			scopes.back()->declare(param.identifier, param.type, param.typeName, param.arrayType, param.dim, param.isAny, param.isConst, param.row, param.col);
		}
	}

	// clear the global function parameters vector
	currentFunctionParameters.clear();

	// visit each statement in the block
	for (auto& stmt : astnode->statements) {
		stmt->accept(this);
	}

	// close scope
	scopes.pop_back();
}

void SemanticAnalyser::visit(parser::ASTIfNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// make sure it is boolean
	//if (currentExpressionType != parser::TYPE::T_BOOL && currentExpressionType != parser::TYPE::T_STRUCT) {
	//	throw std::runtime_error(msgHeader(ifNode->row, ifNode->col) + "invalid if-condition, expected boolean expression.");
	//}

	// check the if block
	astnode->ifBlock->accept(this);

	// if there is an else block, check it too
	if (astnode->elseBlock) {
		astnode->elseBlock->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTWhileNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// make sure it is boolean
	//if (currentExpressionType != parser::TYPE::T_BOOL && currentExpressionType != parser::TYPE::T_STRUCT)
	//	throw std::runtime_error(msgHeader(whileNode->row, whileNode->col) + "invalid while-condition, expected boolean expression.");

	// check the while block
	astnode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* astnode) {
	isFunctionDefinitionContext = true;

	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->alreadyDeclaredFunction(astnode->identifier, astnode->signature)) {
			// determine line number of error and the corresponding function signature
			int line = scope->declarationLine(astnode->identifier, astnode->signature);
			std::string signature = "(";
			bool has_params = false;
			for (auto param : astnode->signature) {
				has_params = true;
				signature += parser::typeStr(param) + ", ";
			}
			if (astnode->signature.size() > 0) {
				signature.pop_back();   // remove last whitespace
				signature.pop_back();   // remove last comma
			}
			signature += ")";


			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "error: function " + astnode->identifier + signature + " already defined.");
		}
	}

	// add function to symbol table
	scopes.back()->declare(astnode->identifier, astnode->type, astnode->typeName, astnode->signature, astnode->type == parser::TYPE::T_ANY, astnode->row, astnode->row);

	// push current function type into function stack
	functions.push(astnode->type);

	// empty and update current function parameters vector
	currentFunctionParameters.clear();
	currentFunctionParameters = astnode->parameters;

	// check semantics of function block by visiting nodes
	astnode->block->accept(this);

	// check that the function body returns
	if (!returns(astnode->block) && astnode->type != parser::TYPE::T_VOID) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "defined function " + astnode->identifier + " is not guaranteed to return a value.");
	}

	// end the current function
	functions.pop();

	isFunctionDefinitionContext = false;
}

void SemanticAnalyser::visit(parser::ASTStructDefinitionNode* astnode) {
	// first check that all enclosing scopes have not already defined the struct
	for (auto& scope : scopes) {
		if (scope->alreadyDeclaredStructureDefinition(astnode->identifier)) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "error: struct " + astnode->identifier + " already defined.");
		}
	}

	scopes.back()->declareStructureDefinition(astnode->identifier, astnode->variables, astnode->row, astnode->col);
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_bool>*) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_BOOL;
	currentExpressionTypeName = "";
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_int>*) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_INT;
	currentExpressionTypeName = "";
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_float>*) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_FLOAT;
	currentExpressionTypeName = "";
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_char>*) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_CHAR;
	currentExpressionTypeName = "";
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_string>*) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionTypeName = "";
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_array>* astnode) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_ARRAY;
	currentExpressionTypeName = "";
	determineArrayType(astnode->val, astnode->col, astnode->row);
}

void SemanticAnalyser::determineArrayType(cp_array arr, unsigned int row, unsigned int col) {
	for (int i = 0; i < arr.size(); ++i) {
		Value_t* val = arr.at(i);
		if (val->currentType == parser::TYPE::T_ARRAY) {
			determineArrayType(val->arr, row, col);
		}
		else {
			checkArrayType(val->currentType, row, col);
			if (currentExpressionArrayType == parser::TYPE::T_STRUCT) {
				currentExpressionTypeName = val->str.first;
			}
		}
	}
}

void SemanticAnalyser::checkArrayType(parser::TYPE type, unsigned int row, unsigned int col) {
	if (currentExpressionArrayType == parser::TYPE::T_ND || currentExpressionArrayType == parser::TYPE::T_ANY || currentExpressionArrayType == parser::TYPE::T_NULL) {
		currentExpressionArrayType = type;
		return;
	}
	if (currentExpressionArrayType != type) {
		throw std::runtime_error(msgHeader(row, col) + "invalid " + typeStr(type) + " type value encontered on " + parser::typeStr(currentExpressionArrayType) + " array");
	}
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_struct>* astnode) {
	currentExpressionArrayType = parser::TYPE::T_ND;
	currentExpressionType = parser::TYPE::T_STRUCT;
	currentExpressionTypeName = astnode->val.first;
}

void SemanticAnalyser::visit(parser::ASTBinaryExprNode* astnode) {
	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	parser::TYPE l_type = currentExpressionType;

	// then right node
	astnode->right->accept(this);
	parser::TYPE r_type = currentExpressionType;

	// these only work for int/float
	if (op == "*" || op == "/" || op == "-" || op == "%") {
		if ((l_type != parser::TYPE::T_INT && l_type != parser::TYPE::T_FLOAT) || (r_type != parser::TYPE::T_INT && r_type != parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected numerical operands for '" + op + "' operator.");
		}

		if (op == "%" && (l_type == parser::TYPE::T_FLOAT || r_type == parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid operands to binary expression ('" + parser::typeStr(l_type) + " and '" + parser::typeStr(r_type) + "').");
		}

		// if both int, then expression is int, otherwise float
		currentExpressionType = (l_type == parser::TYPE::T_INT && r_type == parser::TYPE::T_INT) ? parser::TYPE::T_INT : parser::TYPE::T_FLOAT;
	}
	else if (op == "+") {
		// + works for all types except bool
		if (l_type == parser::TYPE::T_BOOL || r_type == parser::TYPE::T_BOOL) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid operand for '+' operator, expected numerical or string operand.");
		}
		if ((l_type == parser::TYPE::T_STRING || l_type == parser::TYPE::T_CHAR) && (r_type == parser::TYPE::T_STRING || r_type == parser::TYPE::T_CHAR)) { // If both string, no error
			currentExpressionType = (l_type == parser::TYPE::T_CHAR && r_type == parser::TYPE::T_CHAR) ? parser::TYPE::T_CHAR : parser::TYPE::T_STRING;
		}
		else if ((l_type == parser::TYPE::T_STRING || l_type == parser::TYPE::T_CHAR) || (r_type == parser::TYPE::T_STRING || r_type == parser::TYPE::T_CHAR)) { // only one is string, error
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched operands for '+' operator, found " + parser::typeStr(l_type) + " on the left, but " + parser::typeStr(r_type) + " on the right.");
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
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected two boolean-type operands for '" + op + "' operator.");
		}
	}
	else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
		// rel-ops only work for numeric types
		if ((l_type != parser::TYPE::T_FLOAT && l_type != parser::TYPE::T_INT) || (r_type != parser::TYPE::T_FLOAT && r_type != parser::TYPE::T_INT)) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected two numerical operands for '" + op + "' operator.");
		}
		currentExpressionType = parser::TYPE::T_BOOL;
	}
	else if (op == "==" || op == "!=") {
		// == and != only work for like types
		if (l_type != r_type && (l_type != parser::TYPE::T_FLOAT || r_type != parser::TYPE::T_INT) && (l_type != parser::TYPE::T_INT || r_type != parser::TYPE::T_FLOAT)) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected arguments of the same type '" + op + "' operator.");
		}
		currentExpressionType = parser::TYPE::T_BOOL;
	}
	else {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "unhandled semantic error in binary operator.");
	}
}

void SemanticAnalyser::visit(parser::ASTIdentifierNode* astnode) {
	std::string actualIdentifier = astnode->identifier;
	if (astnode->identifierVector.size() > 1) {
		actualIdentifier = axe::join(astnode->identifierVector, ".");
	}

	// determine the inner-most scope in which the value is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredVariable(isFunctionDefinitionContext ? astnode->identifier : actualIdentifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "identifier '" + actualIdentifier + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + "");
		}
	}


	// update current expression type
	if (isFunctionDefinitionContext) {
		currentExpressionType = findType(actualIdentifier);
		currentExpressionTypeName = findTypeName(actualIdentifier);
	}
	else {
		currentExpressionType = scopes[i]->type(actualIdentifier);
		currentExpressionTypeName = scopes[i]->typeName(actualIdentifier);
	}

	if (astnode->accessVector.size() > 0 && currentExpressionType != parser::TYPE::T_ARRAY && currentExpressionType != parser::TYPE::T_STRING) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + actualIdentifier + "' is not an array or string");
	}
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* astnode) {
	// determine expression type
	astnode->expr->accept(this);

	// handle different cases
	switch (currentExpressionType) {
	case parser::TYPE::T_INT:
	case parser::TYPE::T_FLOAT:
		if (astnode->unaryOp != "+" && astnode->unaryOp != "-") {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "operator '" + astnode->unaryOp + "' in front of numerical expression.");
		}
		break;
	case parser::TYPE::T_BOOL:
	case parser::TYPE::T_STRUCT:
		if (astnode->unaryOp != "not") {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "operator '" + astnode->unaryOp + "' in front of boolean expression.");
		}
		break;
	default:
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "incompatible unary operator '" + astnode->unaryOp + "' in front of expression.");
	}
}

void SemanticAnalyser::visit(parser::ASTFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;

	// for each parameter,
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);
	}

	// make sure the function exists in some scope i
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredFunction(astnode->identifier, signature); i--) {
		if (i <= 0) {
			std::string func_name = astnode->identifier + "(";
			bool has_params = false;
			for (auto param : signature) {
				has_params = true;
				func_name += parser::typeStr(param) + ", ";
			}
			func_name.pop_back();   // remove last whitespace
			func_name.pop_back();   // remove last comma
			func_name += ")";
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "function '" + func_name + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// set current expression type to the return value of the function
	currentExpressionType = scopes[i]->type(astnode->identifier, signature);
	currentExpressionTypeName = scopes[i]->typeName(astnode->identifier, std::move(signature));
}

void SemanticAnalyser::visit(parser::ASTTypeParseNode* astnode) {
	astnode->expr->accept(this);
	currentExpressionType = astnode->type;
}

void SemanticAnalyser::visit(parser::ASTReadNode* astnode) {
	if (currentExpressionType != parser::TYPE::T_STRING) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "function 'read()' is trying to assing an invalid type");
	}
}

void SemanticAnalyser::visit(parser::ASTTypeNode* astnode) {
	astnode->expr->accept(this);
	currentExpressionType = parser::TYPE::T_STRING;
}

void SemanticAnalyser::visit(parser::ASTLenNode* astnode) {
	astnode->expr->accept(this);

	if (currentExpressionType != parser::TYPE::T_ARRAY && currentExpressionType != parser::TYPE::T_STRING) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "can't read len of type " + parser::typeStr(currentExpressionType));
	}

	currentExpressionType = parser::TYPE::T_INT;
}

void SemanticAnalyser::visit(parser::ASTRoundNode* astnode) {
	astnode->expr->accept(this);

	if (currentExpressionType != parser::TYPE::T_FLOAT) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "can't round type " + parser::typeStr(currentExpressionType));
	}

	currentExpressionType = parser::TYPE::T_FLOAT;
}

void SemanticAnalyser::visit(parser::ASTNullNode* astnode) {
	currentExpressionType = parser::TYPE::T_NULL;
}

void SemanticAnalyser::visit(parser::ASTThisNode* astnode) {
	currentExpressionType = parser::TYPE::T_STRING;
}

std::string SemanticAnalyser::msgHeader(unsigned int row, unsigned int col) {
	return "(SERR) " + currentProgram->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
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
		if (ifstmt->elseBlock) {
			return (returns(ifstmt->ifBlock) && returns(ifstmt->elseBlock));
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
