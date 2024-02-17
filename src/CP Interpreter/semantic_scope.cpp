#include <utility>
#include <iostream>

#include "semantic_scope.h"
#include "util.h"


using namespace visitor;


SemanticScope::SemanticScope() {}

parser::StructureDefinition_t SemanticScope::findDeclaredStructureDefinition(std::string identifier) {
	for (auto structure : structureSymbolTable) {
		if (structure.identifier == identifier) {
			return structure;
		}
	}

	throw std::runtime_error("SERR: can't found '" + identifier + "' type.");
}

bool SemanticScope::alreadyDeclaredStructureDefinition(std::string identifier) {
	try {
		findDeclaredStructureDefinition(identifier);
		return true;
	}
	catch (...) {
		return false;
	}
	return false;
}


std::vector<parser::VariableDefinition_t> SemanticScope::getVariableSymbolTable() {
	return variableSymbolTable;
}

bool SemanticScope::alreadyDeclaredVariable(std::string identifier) {
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

bool SemanticScope::alreadyDeclaredFunction(std::string identifier, std::vector<parser::TYPE> signature) {
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
		if (funcSig.size() == 0 && signature.size() == 0) {
			return true;
		}
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

bool SemanticScope::findAnyVar(std::string identifier) {
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
	structureSymbolTable.push_back(type);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::string typeName, parser::TYPE arrayType, std::vector<int> dim, bool isAny, bool isConst, unsigned int row, unsigned int col) {
	parser::VariableDefinition_t var(identifier, type, typeName, arrayType, dim, isAny, isConst, row, col);
	variableSymbolTable.push_back(var);
}

void SemanticScope::declare(std::string identifier, parser::TYPE type, std::string typeName, std::vector<parser::TYPE> signature, bool isAny, unsigned int row, unsigned int col) {
	parser::FunctionDefinition_t fun(identifier, type, typeName, signature, isAny, row, col);
	functionSymbolTable.push_back(fun);
}

parser::TYPE SemanticScope::arrayType(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.arrayType;
		}
	}

	throw std::runtime_error("SERR: something went wrong when determining the type of '" + identifier + "' array");
}

parser::VariableDefinition_t SemanticScope::var(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable;
		}
	}
}

std::string SemanticScope::typeName(std::string identifier) {
	return var(identifier).typeName;
}

parser::TYPE SemanticScope::type(std::string identifier) {
	return var(identifier).type;
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
		throw std::runtime_error("SERR: something went wrong when determining the type of '" + identifier + "' function");
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

	throw std::runtime_error("SERR: something went wrong when determining the type of '" + identifier + "' function");
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
		throw std::runtime_error("SERR: something went wrong when determining the type of '" + identifier + "' function");
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

	throw std::runtime_error("SERR: something went wrong when determining the type of '" + identifier + "' function");
}

unsigned int SemanticScope::declarationLine(std::string identifier) {
	for (auto variable : variableSymbolTable) {
		if (variable.identifier == identifier) {
			return variable.row;
		}
	}

	throw std::runtime_error("SERR: something went wrong when determining the line number of '" + identifier + "'");
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
		throw std::runtime_error("SERR: something went wrong when determining the line number of '" + identifier + "'");
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
	throw std::runtime_error("SERR: something went wrong when determining the line number of '" + identifier + "'");
}


std::vector<std::pair<std::string, std::string>> SemanticScope::functionList() {
	std::vector<std::pair<std::string, std::string>> list;

	// check signature for each function in functionSymbolTable
	for (auto fun : functionSymbolTable) {
		std::string func_name = fun.identifier + "(";
		bool has_params = false;
		for (auto param : fun.signature) {
			has_params = true;
			func_name += parser::typeStr(param) + ", ";
		}
		func_name.pop_back();   // remove last whitespace
		func_name.pop_back();   // remove last comma
		func_name += ")";

		list.emplace_back(std::make_pair(func_name, parser::typeStr(fun.type)));
	}

	return list;
}
