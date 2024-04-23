#include <utility>
#include <iostream>

#include "semantic_scope.hpp"
#include "util.hpp"


using namespace visitor;


SemanticScope::SemanticScope() {}

parser::StructureDefinition_t SemanticScope::findDeclaredStructureDefinition(std::string identifier) {
	return structureSymbolTable[identifier];
}

parser::VariableDefinition_t SemanticScope::findDeclaredVariable(std::string identifier) {
	return variableSymbolTable[identifier];
}

parser::FunctionDefinition_t SemanticScope::findDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong when determining the type of '" + identifier + "'.");
	}

	// check signature for each function in functionSymbolTable
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& funcSig = i->second.signature;
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return i->second;
	}

	throw std::runtime_error("SSERR: definition of '" + identifier + "' function not found");
}

bool SemanticScope::alreadyDeclaredStructureDefinition(std::string identifier) {
	return structureSymbolTable.find(identifier) != structureSymbolTable.end();
}

bool SemanticScope::alreadyDeclaredVariable(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool SemanticScope::alreadyDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
	try {
		findDeclaredFunction(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

void SemanticScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t strDef(name, variables, row, col);
	structureSymbolTable[name] = strDef;
}

void SemanticScope::declareVariable(std::string identifier, parser::TYPE type, std::string typeName, parser::TYPE arrayType, std::vector<parser::ASTExprNode*> dim, bool isConst, bool hasValue, unsigned int row, unsigned int col, bool isParameter) {
	parser::VariableDefinition_t var(identifier, type, typeName, arrayType, dim, isConst, hasValue, row, col, isParameter);
	variableSymbolTable[identifier] = var;
}

void SemanticScope::declareFunction(std::string identifier, parser::TYPE type, std::string typeName, std::vector<parser::TYPE> signature, bool isAny, unsigned int row, unsigned int col) {
	parser::FunctionDefinition_t fun(identifier, type, typeName, signature, isAny, row, col);
	functionSymbolTable.insert(std::make_pair(identifier, fun));
}

void SemanticScope::assignVariable(std::string identifier, bool hasValue) {
	auto var = variableSymbolTable[identifier];
	var.hasValue = hasValue;
	variableSymbolTable[identifier] = var;
}

void SemanticScope::changeVariableTypeName(std::string identifier, std::string typeName) {
	auto var = variableSymbolTable[identifier];
	var.typeName = typeName;
	variableSymbolTable[identifier] = var;
}

void SemanticScope::changeVariableType(std::string identifier, parser::TYPE type) {
	auto var = variableSymbolTable[identifier];
	var.type = type;
	variableSymbolTable[identifier] = var;
}
