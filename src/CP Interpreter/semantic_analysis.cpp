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
		currentExpressionArrayType = parser::TYPE::T_NULL;
		currentExpressionTypeName = "";
	}

	bool hasValue = currentExpressionType != parser::TYPE::T_NULL;

	// similar types
	if (astnode->type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT
		|| astnode->type == parser::TYPE::T_STRING && currentExpressionType == parser::TYPE::T_CHAR) {
		currentScope->declareVariable(astnode->identifier, astnode->type, astnode->typeName, astnode->arrayType, astnode->dim, astnode->isConst, hasValue, astnode->row, astnode->col, false);
	}
	// equal types and special types (any, struct and array)
	else if (astnode->type == currentExpressionType || astnode->type == parser::TYPE::T_ANY || astnode->type == parser::TYPE::T_STRUCT || astnode->type == parser::TYPE::T_ARRAY) { // types match
		// handle struct
		if (astnode->type == parser::TYPE::T_STRUCT || currentExpressionType == parser::TYPE::T_STRUCT) {
			if (currentExpressionType == parser::TYPE::T_STRUCT && astnode->type != parser::TYPE::T_STRUCT && astnode->type != parser::TYPE::T_ANY) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + parser::typeStr(astnode->type) + ")' with type '" + currentExpressionTypeName + "'");
			}
			if (currentExpressionType == parser::TYPE::T_STRUCT && astnode->type == parser::TYPE::T_STRUCT && astnode->typeName != currentExpressionTypeName) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + astnode->typeName + ")' with type '" + currentExpressionTypeName + "'");
			}

			parser::ASTStructConstructorNode* strExpr = nullptr;

			// has expression to declare
			if (astnode->expr) {
				if (currentExpressionType != parser::TYPE::T_STRUCT && currentExpressionType != parser::TYPE::T_NULL) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "found " + parser::typeStr(currentExpressionType) + (currentExpressionTypeName.empty() ? "" : " (" + currentExpressionTypeName + ")") + " in definition of '" + astnode->identifier + "', expected " + parser::typeStr(astnode->type) + (astnode->typeName.empty() ? "" : " (" + astnode->typeName + ")") + "");
				}
				// TODO: need more tests
				if (currentExpressionType == parser::TYPE::T_STRUCT) {
					strExpr = static_cast<parser::ASTStructConstructorNode*>(astnode->expr);
				}
			}

			auto strType = astnode->type;
			auto strTypeName = astnode->typeName;
			// check if is any var
			if (astnode->type == parser::TYPE::T_ANY) {
				strType = currentExpressionType;
				strTypeName = currentExpressionTypeName;
			}

			currentScope->declareVariable(astnode->identifier, strType, strTypeName, astnode->arrayType, astnode->dim, astnode->isConst, hasValue, astnode->row, astnode->col, false);
			if (strExpr && !isFunctionDefinitionContext) {
				declareStructureDefinitionVariables(astnode->identifier, strExpr);
			}
			declareStructureDefinitionFirstLevelVariables(astnode->identifier, strTypeName);
		}
		else if (astnode->type == parser::TYPE::T_ARRAY || currentExpressionType == parser::TYPE::T_ARRAY) {
			if (astnode->type != parser::TYPE::T_ANY && astnode->type != parser::TYPE::T_ARRAY && currentExpressionType == parser::TYPE::T_ARRAY) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "cannot initialize '" + astnode->identifier + "(" + parser::typeStr(astnode->type) + ")' with type '" + parser::typeStr(currentExpressionType) + "'");
			}

			if (astnode->expr) {
				if (currentExpressionType != parser::TYPE::T_ARRAY && currentExpressionType != parser::TYPE::T_NULL) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "expected array expression assigning '" + astnode->identifier + "'");
				}

				// TODO: need more tests
				parser::ASTArrayConstructorNode* arr = dynamic_cast<parser::ASTArrayConstructorNode*>(astnode->expr);
				if (arr) {
					determineArrayType(arr);

					auto exprDim = calculateArrayDimSize(arr);

					if (astnode->dim.size() != exprDim.size()) {
						throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier + "'");
					}

					for (size_t dc = 0; dc < astnode->dim.size(); ++dc) {
						if (astnode->dim.at(dc) != -1 && astnode->dim.at(dc) != exprDim.at(dc)) {
							throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid array size assigning '" + astnode->identifier + "'");
						}
					}
				}

			}

			currentExpressionArrayType = currentExpressionArrayType == parser::TYPE::T_NULL ? parser::TYPE::T_ANY : currentExpressionArrayType;

			currentScope->declareVariable(astnode->identifier, astnode->type, astnode->typeName, currentExpressionArrayType, astnode->dim, astnode->isConst, hasValue, astnode->row, astnode->col, false);
		}
		else if (astnode->type == currentExpressionType || astnode->type == parser::TYPE::T_ANY) {
			currentScope->declareVariable(astnode->identifier, currentExpressionType, astnode->typeName, astnode->arrayType, astnode->dim, astnode->isConst, hasValue, astnode->row, astnode->col, false);
		}
		else {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "found " + parser::typeStr(currentExpressionType) + " defining '" + astnode->identifier + "', expected " + parser::typeStr(astnode->type) + "");
		}
	}
	else if (currentExpressionType == parser::TYPE::T_NULL) {
		currentScope->declareVariable(astnode->identifier, astnode->type, astnode->typeName, astnode->arrayType, astnode->dim, astnode->isConst, hasValue, astnode->row, astnode->col, false);
	}
	else { // types don't match
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "found " + parser::typeStr(currentExpressionType) + " in definition of '" + astnode->identifier + "', expected " + parser::typeStr(astnode->type) + "");
	}
}

void SemanticAnalyser::visit(parser::ASTAssignmentNode* astnode) {
	std::string actualIdentifier = astnode->identifier;
	if (astnode->identifierVector.size() > 1) {
		actualIdentifier = axe::join(astnode->identifierVector, ".");
	}

	// determine the inner-most scope in which the value is declared
	int i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredVariable(isFunctionDefinitionContext ? astnode->identifier : actualIdentifier); i--) {
		if (i <= 0) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "identifier '" + actualIdentifier + "' being reassigned was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	// get base variable
	parser::VariableDefinition_t declaredVariable = scopes[i]->findDeclaredVariable(astnode->identifier);
	parser::TYPE type;

	// check if the base variable is not null
	if (!declaredVariable.hasValue && !declaredVariable.isParameter) {
		// struct
		if (astnode->identifierVector.size() > 1) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "trying assign '" + actualIdentifier + "' but '" + astnode->identifier + "' is null");
		}
		// array
		if (astnode->accessVector.size() > 0) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "trying assign '" + actualIdentifier + "' array position but it's null");
		}
	}

	// visit the expression to update current type
	astnode->expr->accept(this);

	// evaluate array access vector
	evaluateAccessVector(astnode->accessVector);
	
	// get the type of the originally declared variable
	if (declaredVariable.isParameter) {
		declaredVariable = findDeclaredVariableRecursively(actualIdentifier);
	}
	// assign if it has or not a value
	else {
		scopes[i]->assignVariable(actualIdentifier, currentExpressionType != parser::TYPE::T_NULL);
		declaredVariable = scopes[i]->findDeclaredVariable(actualIdentifier);
	}

	if (declaredVariable.isConst) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + actualIdentifier + "' constant being reassigned " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
	}

	type = declaredVariable.type;

	if (type == parser::TYPE::T_ARRAY) {
		if (currentExpressionType == parser::TYPE::T_ARRAY) {

			try {
				parser::ASTArrayConstructorNode* arrExpr = dynamic_cast<parser::ASTArrayConstructorNode*>(astnode->expr);
				std::vector<int> exprDim;

				if (arrExpr) {
					determineArrayType(arrExpr);
					exprDim = calculateArrayDimSize(arrExpr);
				}
				else {
					parser::ASTIdentifierNode* idExpr = dynamic_cast<parser::ASTIdentifierNode*>(astnode->expr);
					auto exprVariable = findDeclaredVariableRecursively(idExpr->identifier);
					exprDim = exprVariable.dim;
					currentExpressionArrayType = exprVariable.arrayType;
				}

				if (declaredVariable.dim.size() != exprDim.size()) {
					throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid array dimension assigning '" + astnode->identifier + "'");
				}

				for (size_t dc = 0; dc < declaredVariable.dim.size(); ++dc) {
					if (declaredVariable.dim[i] != -1 && declaredVariable.dim[i] != exprDim.at(dc)) {
						throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid array size assigning '" + astnode->identifier + "'");
					}
				}
			}
			catch (...) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid assignment of '" + actualIdentifier + "' array");
			}
		}

		if (astnode->accessVector.size() == 0 && declaredVariable.arrayType != parser::TYPE::T_ANY && declaredVariable.arrayType != currentExpressionArrayType) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "', expected '" + parser::typeStr(declaredVariable.arrayType) + "' array, found '" + parser::typeStr(currentExpressionArrayType) + "' array");
		}
	}

	if (astnode->accessVector.size() > 0) {
		type = declaredVariable.arrayType;
	}

	// allow mismatched type in the case of declaration of int to real
	if (type == parser::TYPE::T_FLOAT && currentExpressionType == parser::TYPE::T_INT ||
		type == parser::TYPE::T_INT && currentExpressionType == parser::TYPE::T_FLOAT) {
	}
	else if (type == parser::TYPE::T_ANY) {
		scopes[i]->changeVariableType(actualIdentifier, currentExpressionType);
		scopes[i]->changeVariableTypeName(actualIdentifier, currentExpressionTypeName);
	}
	else if (type == parser::TYPE::T_STRUCT && (currentExpressionType == parser::TYPE::T_STRUCT || currentExpressionType == parser::TYPE::T_NULL)) {
		std::string typeName = declaredVariable.typeName;
		if (currentExpressionTypeName != typeName && currentExpressionType != parser::TYPE::T_NULL) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "' struct. Expected '" + typeName + "', found '" + currentExpressionTypeName + "'");
		}

		if (typeid(astnode->expr) == typeid(parser::ASTStructConstructorNode*)) {
			parser::ASTStructConstructorNode* strExpr = static_cast<parser::ASTStructConstructorNode*>(astnode->expr);

			if (strExpr) {
				declareStructureDefinitionVariables(actualIdentifier, strExpr);
			}
		}
		else {
			declareStructureDefinitionFirstLevelVariables(actualIdentifier, typeName);
		}

	}
	// otherwise throw error
	else if (currentExpressionType != type && type != parser::TYPE::T_ANY && type != parser::TYPE::T_NULL) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "mismatched type for '" + actualIdentifier + "'. Expected " + parser::typeStr(type) + ", found " + parser::typeStr(currentExpressionType) + "");
	}
}

void SemanticAnalyser::determineArrayType(parser::ASTArrayConstructorNode* astnode) {
	auto auxCurrType = currentExpressionType;
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);

		if (currentExpressionType == parser::TYPE::T_ARRAY) {
			determineArrayType(static_cast<parser::ASTArrayConstructorNode*>(astnode->values.at(i)));
		}
		else {
			checkArrayType(astnode->values.at(i), astnode->row, astnode->col);
		}
	}
	currentExpressionType = auxCurrType;
}

void SemanticAnalyser::checkArrayType(parser::ASTExprNode* astnode, unsigned int row, unsigned int col) {
	auto auxCurrType = currentExpressionType;
	astnode->accept(this);

	if (currentExpressionArrayType == parser::TYPE::T_ANY || currentExpressionArrayType == parser::TYPE::T_ND || currentExpressionArrayType == parser::TYPE::T_NULL || currentExpressionArrayType == parser::TYPE::T_VOID) {
		currentExpressionArrayType = currentExpressionType;
	}
	if (currentExpressionArrayType != currentExpressionType) {
		throw std::runtime_error(msgHeader(row, col) + "mismatched type in array definition ");
	}
	currentExpressionType = auxCurrType;
}

std::vector<int> SemanticAnalyser::calculateArrayDimSize(parser::ASTArrayConstructorNode* arr) {
	auto dim = std::vector<int>();

	dim.push_back(arr->values.size());

	parser::ASTArrayConstructorNode* subArr = dynamic_cast<parser::ASTArrayConstructorNode*>(arr->values.at(0));
	if (subArr) {
		auto dim2 = calculateArrayDimSize(subArr);
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
		scopes.back()->declareVariable(currentIdentifier, varTypeStruct.type, varTypeStruct.typeName, varTypeStruct.arrayType, varTypeStruct.dim, varTypeStruct.isConst, false, varTypeStruct.row, varTypeStruct.col, false);
	}
}

void SemanticAnalyser::declareStructureDefinitionVariables(std::string identifier, parser::ASTStructConstructorNode* expr) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureDefinition(expr->typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(expr->typeName);

	for (auto const& strValue : expr->values) {
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
		if (!found) throw std::runtime_error(msgHeader(expr->row, expr->col) + "'" + strValue.first + "' is not a member of '" + expr->typeName + "'");

		strValue.second->accept(this);
		bool hasValue = currentExpressionType != parser::TYPE::T_NULL;

		if (varTypeStruct->type != parser::TYPE::T_ANY && currentExpressionType != parser::TYPE::T_NULL && varTypeStruct->type != currentExpressionType) {
			throw std::runtime_error(msgHeader(expr->row, expr->col) + "invalid type " + parser::typeStr(varTypeStruct->type) + " trying to assign '" + currentIdentifier + "'");
		}

		if (varTypeStruct->type == parser::TYPE::T_STRUCT) {
			scopes.back()->declareVariable(currentIdentifier, varTypeStruct->type, varTypeStruct->identifier, parser::TYPE::T_ND, varTypeStruct->dim, false, hasValue, expr->row, expr->col, false);

			parser::ASTStructConstructorNode* strExpr = nullptr;
			
			if (typeid(parser::ASTStructConstructorNode*) == typeid(strValue.second)) {
				strExpr = static_cast<parser::ASTStructConstructorNode*>(strValue.second);
			}

			if (strExpr) {
				declareStructureDefinitionVariables(currentIdentifier, strExpr);
			}
		}
		else {
			scopes.back()->declareVariable(currentIdentifier, varTypeStruct->type, varTypeStruct->typeName, varTypeStruct->arrayType, varTypeStruct->dim, false, hasValue, expr->row, expr->col, false);
		}
	}
}

parser::VariableDefinition_t SemanticAnalyser::findDeclaredVariableRecursively(std::string identifier) {
	int i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredVariable(identifier); --i) {
		if (i <= 0) {
			i = -1;
			break;
		}
	}
	if (i >= 0) {
		return scopes[i]->findDeclaredVariable(identifier);
	}

	if (axe::contains(identifier, ".")) {
		std::list<std::string> identifiers = axe::splitList(identifier, '.');
		parser::StructureDefinition_t strDef;
		std::string typeName = "";

		for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredVariable(identifiers.front()); --i) {
			if (i <= 0) {
				i = -1;
				break;
			}
		}
		if (i >= 0) {
			typeName = scopes[i]->findDeclaredVariable(identifiers.front()).typeName;
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

	throw std::runtime_error(msgHeader(0, 0) + "error: '" + identifier + "' variable not found");
}

void SemanticAnalyser::evaluateAccessVector(std::vector<parser::ASTExprNode*> exprAcessVector) {
	for (auto expr : exprAcessVector) {
		expr->accept(this);
		if (currentExpressionType != parser::TYPE::T_INT) {
			throw std::runtime_error(msgHeader(0, 0) + "array index access must be a integer value");
		}
	}
}

void SemanticAnalyser::visit(parser::ASTPrintNode* astnode) {
	// update current expression
	astnode->expr->accept(this);
}

void SemanticAnalyser::visit(parser::ASTReturnNode* astnode) {
	// update current expression
	astnode->expr->accept(this);

	// if we are not global, check that we return current function return type
	if (!functions.empty() && currentExpressionType != functions.top()) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "invalid return type. Expected " + parser::typeStr(functions.top()) + ", found " + parser::typeStr(currentExpressionType) + ".");
	}
}

void SemanticAnalyser::visit(parser::ASTBlockNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

	// check whether this is a function block by seeing if we have any current function parameters. If we do, then add them to the current scope.
	for (auto param : currentFunctionParameters) {
		scopes.back()->declareVariable(param.identifier, param.type, param.typeName, param.arrayType, param.dim, param.isConst, param.hasValue, param.row, param.col, true);
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

void SemanticAnalyser::visit(parser::ASTBreakNode* astnode) {

}

void SemanticAnalyser::visit(parser::ASTSwitchNode* astnode) {
	// create new scope
	scopes.push_back(new SemanticScope());

	astnode->parsedCaseBlocks = new std::map<int, unsigned int>();

	astnode->condition->accept(this);

	// visit each case expresion in the block
	for (auto& expr : *astnode->caseBlocks) {
		expr.first->accept(this);
		astnode->parsedCaseBlocks->emplace(expr.first->hash(), expr.second);
		if (!isConstant) {
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "error: expression is not an constant expression");
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
	astnode->ifBlock->accept(this);

	for (auto& elif : astnode->elseIf) {
		elif->accept(this);
	}

	// if there is an else block, check it too
	if (astnode->elseBlock) {
		astnode->elseBlock->accept(this);
	}
}

void SemanticAnalyser::visit(parser::ASTWhileNode* astnode) {
	// set current type to while expression
	astnode->condition->accept(this);

	// check the while block
	astnode->block->accept(this);
}

void SemanticAnalyser::visit(parser::ASTFunctionDefinitionNode* astnode) {
	isFunctionDefinitionContext = true;

	// first check that all enclosing scopes have not already defined the function
	for (auto& scope : scopes) {
		if (scope->alreadyDeclaredFunction(astnode->identifier, astnode->signature)) {
			std::string signature = "(";
			for (auto param : astnode->signature) {
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
	scopes.back()->declareFunction(astnode->identifier, astnode->type, astnode->typeName, astnode->signature, astnode->type == parser::TYPE::T_ANY, astnode->row, astnode->row);

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
	currentExpressionType = parser::TYPE::T_BOOL;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_int>*) {
	currentExpressionType = parser::TYPE::T_INT;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_float>*) {
	currentExpressionType = parser::TYPE::T_FLOAT;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_char>*) {
	currentExpressionType = parser::TYPE::T_CHAR;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTLiteralNode<cp_string>*) {
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTArrayConstructorNode* astnode) {
	for (size_t i = 0; i < astnode->values.size(); ++i) {
		astnode->values.at(i)->accept(this);
	}
	currentExpressionType = parser::TYPE::T_ARRAY;
	currentExpressionTypeName = "";
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTStructConstructorNode* astnode) {
	for (auto& expr : astnode->values) {
		expr.second->accept(this);
	}
	currentExpressionType = parser::TYPE::T_STRUCT;
	currentExpressionTypeName = astnode->typeName;
	isConstant = true;
}

void SemanticAnalyser::visit(parser::ASTBinaryExprNode* astnode) {
	isConstant = false;

	// operator
	std::string op = astnode->op;

	// visit left node first
	astnode->left->accept(this);
	parser::TYPE l_type = currentExpressionType;

	// then right node
	astnode->right->accept(this);
	parser::TYPE r_type = currentExpressionType;

	// these only work for int/float
	if (op == "-" || op == "/" || op == "*" || op == "%") {
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

	parser::VariableDefinition_t declaredVariable;

	// update current expression type
	if (isFunctionDefinitionContext) {
		declaredVariable = findDeclaredVariableRecursively(actualIdentifier);
	}
	else {
		declaredVariable = scopes[i]->findDeclaredVariable(actualIdentifier);
	}

	currentExpressionType = declaredVariable.type;
	currentExpressionTypeName = declaredVariable.typeName;
	isConstant = declaredVariable.isConst;

	if (astnode->accessVector.size() > 0 && currentExpressionType != parser::TYPE::T_ARRAY && currentExpressionType != parser::TYPE::T_STRING) {
		throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + actualIdentifier + "' is not an array or string");
	}
}

void SemanticAnalyser::visit(parser::ASTUnaryExprNode* astnode) {
	isConstant = false;

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
			for (auto param : signature) {
				func_name += parser::typeStr(param) + ", ";
			}
			if (signature.size() > 0) {
				func_name.pop_back();   // remove last whitespace
				func_name.pop_back();   // remove last comma
			}
			func_name += ")";
			throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "function '" + func_name + "' was never declared " + ((scopes.size() == 1) ? "globally" : "in this scope") + '.');
		}
	}

	auto declaredFunction = scopes[i]->findDeclaredFunction(astnode->identifier, signature);

	// set current expression type to the return value of the function
	currentExpressionType = declaredFunction.type;
	currentExpressionTypeName = declaredFunction.typeName;
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
