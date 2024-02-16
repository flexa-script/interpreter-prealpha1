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

	throw std::runtime_error("IERR: can't found '" + identifier + "'");
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

Value_t* InterpreterScope::declareNull(std::string identifier, parser::TYPE type) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(type);
	}
	value->setNull();
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declareNullStruct(std::string identifier, parser::TYPE type, std::string typeName) {
	Value_t* value = declareNull(identifier, type);
	value->str.first = typeName;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_bool boolValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_BOOL);
	}
	value->set(boolValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_int intValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_INT);
	}
	value->set(intValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_float realValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_FLOAT);
	}
	value->set(realValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_char charValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_CHAR);
	}
	value->set(charValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_string stringValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_STRING);
	}
	value->set(stringValue);
	variableSymbolTable[identifier] = value;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_array arrValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_ARRAY);
	}
	value->set(arrValue);
	variableSymbolTable[identifier] = value;
	return value;
}


Value_t* InterpreterScope::declare(std::string identifier, cp_struct strValue) {
	Value_t* value;
	if (alreadyDeclaredVariable(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t(parser::TYPE::T_STRUCT);
	}
	value->set(strValue);

	auto currTypeName = value->str.first;

	if (currTypeName.empty()) {
		currTypeName = strValue.first;
	}

	variableSymbolTable[identifier] = value;
	return value;
}

void InterpreterScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structureSymbolTable.push_back(type);
}

void InterpreterScope::declare(std::string identifier, std::vector<parser::TYPE> signature, std::vector<std::string> variableNames, parser::ASTBlockNode* block) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(signature, variableNames, block)));
}

std::string InterpreterScope::typenameof(std::string identifier) {
	Value_t* value = valueof(identifier);
	return value->str.first;
}

parser::TYPE InterpreterScope::typeof(std::string identifier) {
	Value_t* value = valueof(identifier);
	return value->currentType;
}

Value_t* InterpreterScope::valueof(std::string identifier) {
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

	throw std::runtime_error("IERR: something went wrong when determining the typename of '" + identifier + "' variable");
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

	for (auto const& var : variableSymbolTable) {
		switch (var.second->actualType) {
		case parser::TYPE::T_BOOL:
			list.emplace_back(std::make_tuple(var.first, "bool", (var.second->b) ? "true" : "false"));
			break;
		case parser::TYPE::T_INT:
			list.emplace_back(std::make_tuple(var.first, "int", std::to_string(var.second->i)));
			break;
		case parser::TYPE::T_FLOAT:
			list.emplace_back(std::make_tuple(var.first, "float", std::to_string(var.second->f)));
			break;
		case parser::TYPE::T_CHAR:
			list.emplace_back(std::make_tuple(var.first, "char", std::to_string(var.second->c)));
			break;
		case parser::TYPE::T_STRING:
			list.emplace_back(std::make_tuple(var.first, "string", var.second->s));
			break;
		}
	}

	return list;
}
