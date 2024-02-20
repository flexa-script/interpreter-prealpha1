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

bool InterpreterScope::alreadyDeclaredStructureDefinition(std::string identifier) {
	for (auto variable : structureSymbolTable) {
		if (variable.identifier == identifier) {
			return true;
		}
	}
	return false;
}

parser::StructureDefinition_t InterpreterScope::findDeclaredStructureDefinition(std::string identifier) {
	for (auto variable : structureSymbolTable) {
		if (variable.identifier == identifier) {
			return variable;
		}
	}

	throw std::runtime_error("ISERR: can't found '" + identifier + "'");
}

bool InterpreterScope::alreadyDeclaredVariable(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool InterpreterScope::alreadyDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = std::vector<std::pair<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>>>();

	for (auto fun : functionSymbolTable) {
		if (fun.first == identifier) {
			funcs.push_back(fun);
		}
	}

	// if key is not present in functionSymbolTable
	if (funcs.empty()) {
		return false;
	}

	// check signature for each function in multimap
	for (auto fun : funcs) {
		auto funcSig = std::get<0>(fun.second);
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
			}
		}
		if (found) return true;
	}

	return false;
}

Value_t* InterpreterScope::declareNull(std::string identifier, parser::TYPE type, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(type);
		variableSymbolTable[identifier] = value;
	}
	value->setNull();
	return value;
}

Value_t* InterpreterScope::declareNullStruct(std::string identifier, parser::TYPE type, std::string typeName, std::vector<unsigned int> accessVector) {
	Value_t* value = declareNull(identifier, type, accessVector);
	value->str.first = typeName;
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_bool boolValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_BOOL);
		variableSymbolTable[identifier] = value;
	}
	value->set(boolValue);
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_int intValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_INT);
		variableSymbolTable[identifier] = value;
	}
	value->set(intValue);
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_float realValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_FLOAT);
		variableSymbolTable[identifier] = value;
	}
	value->set(realValue);
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_char charValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_CHAR);
		variableSymbolTable[identifier] = value;
	}
	value->set(charValue);
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_string stringValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_STRING);
		variableSymbolTable[identifier] = value;
	}
	value->set(stringValue);
	return value;
}

Value_t* InterpreterScope::declareVariable(std::string identifier, cp_array arrValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_ARRAY);
		variableSymbolTable[identifier] = value;
	}
	value->set(arrValue);
	return value;
}


Value_t* InterpreterScope::declareVariable(std::string identifier, cp_struct strValue, std::vector<unsigned int> accessVector) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier, accessVector);
	}
	else {
		value = new Value_t(parser::TYPE::T_STRUCT);
		variableSymbolTable[identifier] = value;
	}
	value->set(strValue);

	auto currTypeName = value->str.first;

	if (currTypeName.empty()) {
		currTypeName = strValue.first;
	}

	return value;
}

void InterpreterScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structureSymbolTable.push_back(type);
}

void InterpreterScope::declareVariable(std::string identifier, std::vector<parser::TYPE> signature, std::vector<std::string> variableNames, parser::ASTBlockNode* block) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(signature, variableNames, block)));
}

std::string InterpreterScope::typenameof(std::string identifier, std::vector<unsigned int> accessVector) {
	Value_t* value = valueof(identifier, accessVector);
	return value->str.first;
}

parser::TYPE InterpreterScope::typeof(std::string identifier, std::vector<unsigned int> accessVector) {
	Value_t* value = valueof(identifier, accessVector);
	return value->currentType;
}

Value_t* InterpreterScope::accessvalueofarray(Value_t* arr, std::vector<unsigned int> accessVector) {
	cp_array* currentVal = &arr->arr;
	size_t s = 0;

	for (s = 0; s < accessVector.size() - 1; ++s) {
		if (accessVector.at(s) >= currentVal->size()) {
			throw std::runtime_error("ISERR: tryed to access a invalid position in a string");
		}
		if (currentVal->at(accessVector.at(s))->currentType != parser::TYPE::T_ARRAY) {
			hasStringAccess = true;
			break;
		}
		currentVal = &currentVal->at(accessVector.at(s))->arr;
	}

	if (accessVector.at(s) >= currentVal->size()) {
		throw std::runtime_error("ISERR: tryed to access a invalid position in a string");
	}

	return currentVal->at(accessVector.at(s));
}

Value_t* InterpreterScope::valueof(std::string identifier, std::vector<unsigned int> accessVector) {
	Value_t* value;

	if (axe::contains(identifier, ".")) {
		auto identifierVector = axe::split(identifier, '.');
		value = variableSymbolTable[identifierVector[0]];

		for (size_t i = 1; i < identifierVector.size(); ++i) {
			for (size_t j = 0; j < value->str.second.size(); ++j) {
				if (identifierVector[i] == value->str.second[j].first) {
					value = value->str.second[j].second;
					break;
				}
			}
		}
	}
	else {
		value = variableSymbolTable[identifier];
	}

	if (accessVector.size() > 0 && value->actualType == parser::TYPE::T_ARRAY) {
		hasStringAccess = false;
		return accessvalueofarray(value, accessVector);
	}
	else {
		hasStringAccess = true;
	}

	return value;
}

std::vector<std::string> InterpreterScope::variablenamesof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = std::vector<std::pair<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>>>();

	for (auto fun : functionSymbolTable) {
		if (fun.first == identifier) {
			funcs.push_back(fun);
		}
	}

	// check signature for each function in multimap
	for (auto const& fun : funcs) {
		auto const& funcSig = std::get<0>(fun.second);
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				found = false;
			}
		}
		if (found) return std::get<1>(fun.second);
	}

	throw std::runtime_error("ISERR: something went wrong when determining the typename of '" + identifier + "' variable");
}

parser::ASTBlockNode* InterpreterScope::blockof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = std::vector<std::pair<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>>>();

	for (auto const& fun : functionSymbolTable) {
		if (fun.first == identifier) {
			funcs.push_back(fun);
		}
	}

	// check signature for each function in multimap
	for (auto const& fun : funcs) {
		auto const& funcSig = std::get<0>(fun.second);
		auto found = true;
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				//throw std::runtime_error("there was an error determining '" + identifier + "' function statements");
				found = false;
			}
		}
		if (found) return std::get<2>(fun.second);
	}

	return nullptr;
}

std::vector<std::tuple<std::string, std::string, std::string>> InterpreterScope::variableList() {
	std::vector<std::tuple<std::string, std::string, std::string>> list;

	for (auto const& findDeclaredVariable : variableSymbolTable) {
		switch (findDeclaredVariable.second->actualType) {
		case parser::TYPE::T_BOOL:
			list.emplace_back(std::make_tuple(findDeclaredVariable.first, "bool", (findDeclaredVariable.second->b) ? "true" : "false"));
			break;
		case parser::TYPE::T_INT:
			list.emplace_back(std::make_tuple(findDeclaredVariable.first, "int", std::to_string(findDeclaredVariable.second->i)));
			break;
		case parser::TYPE::T_FLOAT:
			list.emplace_back(std::make_tuple(findDeclaredVariable.first, "float", std::to_string(findDeclaredVariable.second->f)));
			break;
		case parser::TYPE::T_CHAR:
			list.emplace_back(std::make_tuple(findDeclaredVariable.first, "char", std::to_string(findDeclaredVariable.second->c)));
			break;
		case parser::TYPE::T_STRING:
			list.emplace_back(std::make_tuple(findDeclaredVariable.first, "string", findDeclaredVariable.second->s));
			break;
		}
	}

	return list;
}
