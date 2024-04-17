#include "interpreter_scope.h"

#include <iostream>
#include <cmath>

#include "interpreter.h"
#include "util.h"


using namespace visitor;


InterpreterScope::InterpreterScope(std::string name) : name(name) {}

InterpreterScope::InterpreterScope() {
	name = "";
}

std::string InterpreterScope::getName() {
	return name;
}

bool InterpreterScope::alreadyDeclaredVariable(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool InterpreterScope::alreadyDeclaredStructureDefinition(std::string identifier) {
	return structureSymbolTable.find(identifier) != structureSymbolTable.end();
}

bool InterpreterScope::alreadyDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
	try {
		findDeclaredFunction(identifier, signature);
		return true;
	}
	catch (...) {
		return false;
	}
}

Value_t* InterpreterScope::declareNullVariable(std::string identifier, parser::TYPE type) {
	Value_t* value = new Value_t(type);
	value->setNull();
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareNullStructVariable(std::string identifier, std::string typeName) {
	Value_t* value = declareNullVariable(identifier, parser::TYPE::T_STRUCT);
	value->str.first = typeName;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_bool boolValue) {
	Value_t* value = new Value_t(parser::TYPE::T_BOOL);
	value->set(boolValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_int intValue) {
	Value_t* value = new Value_t(parser::TYPE::T_INT);
	value->set(intValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_float floatValue) {
	Value_t* value = new Value_t(parser::TYPE::T_FLOAT);
	value->set(floatValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_char charValue) {
	Value_t* value = new Value_t(parser::TYPE::T_CHAR);
	value->set(charValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_string stringValue) {
	Value_t* value = new Value_t(parser::TYPE::T_STRING);
	value->set(stringValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_array arrValue) {
	Value_t* value = new Value_t(parser::TYPE::T_ARRAY);
	value->set(arrValue);
	variableSymbolTable[identifier] = value;
	return value;
}


Value_t* InterpreterScope::declareVariable(std::string identifier, cp_struct strValue) {
	Value_t* value = new Value_t(parser::TYPE::T_STRUCT);
	value->set(strValue);
	variableSymbolTable[identifier] = value;
	return value;
}

void InterpreterScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structureSymbolTable[name] = (type);
}

void InterpreterScope::declareFunction(std::string identifier, std::vector<parser::TYPE> signature, std::vector<std::string> variableNames, parser::ASTBlockNode* block) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(signature, variableNames, block)));
}

parser::StructureDefinition_t InterpreterScope::findDeclaredStructureDefinition(std::string identifier) {
	return structureSymbolTable[identifier];
}

Value_t* InterpreterScope::accessValueOfArray(Value_t* arr, std::vector<unsigned int> accessVector) {
	cp_array* currentVal = &arr->arr;
	size_t s = 0;

	for (s = 0; s < accessVector.size() - 1; ++s) {
		if (accessVector.at(s) >= currentVal->size()) {
			throw std::runtime_error("ISERR: tryed to access a invalid position");
		}
		if (currentVal->at(accessVector.at(s))->currType != parser::TYPE::T_ARRAY) {
			hasStringAccess = true;
			break;
		}
		currentVal = &currentVal->at(accessVector.at(s))->arr;
	}

	if (accessVector.at(s) >= currentVal->size()) {
		throw std::runtime_error("ISERR: tryed to access a invalid position");
	}

	return currentVal->at(accessVector.at(s));
}

Value_t* InterpreterScope::accessValueOfStructure(Value_t* value, std::vector<std::string> identifierVector) {
	Value_t* strValue = value;

	for (size_t i = 1; i < identifierVector.size(); ++i) {
		for (size_t j = 0; j < strValue->str.second.size(); ++j) {
			if (identifierVector[i] == strValue->str.second[j].first) {
				strValue = strValue->str.second[j].second;
				break;
			}
		}
	}

	return strValue;
}

Value_t* InterpreterScope::accessValue(std::vector<std::string> identifierVector, std::vector<unsigned int> accessVector) {
	Value_t* value = variableSymbolTable[identifierVector[0]];

	if (identifierVector.size() > 1) {
		value = accessValueOfStructure(value, identifierVector);
	}

	if (accessVector.size() > 0 && value->currType == parser::TYPE::T_ARRAY) {
		hasStringAccess = false;
		value = accessValueOfArray(value, accessVector);
	}
	else {
		hasStringAccess = true;
	}

	return value;
}

std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*> InterpreterScope::findDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		throw std::runtime_error("something went wrong searching '" + identifier + "'");
	}

	// check signature for each function in multimap
	for (auto& i = funcs.first; i != funcs.second; ++i) {
		auto& funcSig = std::get<0>(i->second);
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
				break;
			}
		}
		if (found) return i->second;
	}

	throw std::runtime_error("something went wrong searching '" + identifier + "'");
}
