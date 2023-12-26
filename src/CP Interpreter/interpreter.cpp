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

bool InterpreterScope::alreadyDeclaredStructureType(std::string identifier) {
	for (auto variable : structures) {
		if (variable.identifier == identifier) {
			return true;
		}
	}
	return false;
}

bool InterpreterScope::alreadyDeclared(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool InterpreterScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// if key is not present in multimap
	if (std::distance(funcs.first, funcs.second) == 0) {
		return false;
	}

	// check signature for each function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		//if (std::get<0>(i->second) == signature) {
		//	return true;
		//}
		auto funcSig = std::get<0>(i->second);
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				return false;
			}
		}
	}

	return true;
}

Value_t* InterpreterScope::declare(std::string identifier, bool boolValue) {
	Value_t* value = new Value_t();
	value->b = boolValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_BOOL, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, __int64_t intValue) {
	Value_t* value = new Value_t();
	value->i = intValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_INT, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, long double realValue) {
	Value_t* value = new Value_t();
	value->f = realValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_FLOAT, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, char charValue) {
	Value_t* value = new Value_t();
	value->c = charValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_CHAR, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, std::string stringValue) {
	Value_t* value = new Value_t();
	value->s = stringValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_STRING, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, std::any anyValue) {
	Value_t* value = new Value_t();
	value->a = anyValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_ANY, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, std::string typeName, cp_struct strValue) {
	Value_t* value = new Value_t();
	value->str = strValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_STRUCT, value);
	typeNamesTable[identifier] = typeName;
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, std::vector<std::any>* arrValue) {
	Value_t* value = new Value_t();
	value->arr = arrValue;
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_ARRAY, value);
	return value;
}

void InterpreterScope::declareStructureType(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
	parser::StructureDefinition_t type(name, variables, row, col);
	structures.push_back(type);
}

void InterpreterScope::declare(std::string identifier, std::vector<parser::TYPE> signature, std::vector<std::string> variableNames, parser::ASTBlockNode* block) {
	functionSymbolTable.insert(std::make_pair(identifier, std::make_tuple(signature, variableNames, block)));
}

std::string InterpreterScope::typenameof(std::string identifier) {
	for (auto variable : typeNamesTable) {
		if (variable.first == identifier) {
			return variable.second;
		}
	}

	throw std::runtime_error("something went wrong when determining the typename of '" + identifier + "' variable");
}

parser::TYPE InterpreterScope::typeof(std::string identifier) {
	return variableSymbolTable[identifier].first;
}

Value_t* InterpreterScope::valueof(std::string identifier) {
	return variableSymbolTable[identifier].second;
}

std::vector<std::string> InterpreterScope::variablenamesof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// match given signature to function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		//if (std::get<0>(i->second) == signature) {
		//	return std::get<1>(i->second);
		//}
		auto funcSig = std::get<0>(i->second);
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				throw std::runtime_error("there was an error determining '" + identifier + "' function parameters");
			}
		}
		return std::get<1>(i->second);
	}

}

parser::ASTBlockNode* InterpreterScope::blockof(std::string identifier, std::vector<parser::TYPE> signature) {
	auto funcs = functionSymbolTable.equal_range(identifier);

	// match given signature to function in multimap
	for (auto i = funcs.first; i != funcs.second; i++) {
		//if (std::get<0>(i->second) == signature) {
		//	return std::get<2>(i->second);
		//}
		auto funcSig = std::get<0>(i->second);
		for (size_t it = 0; it < funcSig.size(); ++it) {
			if (funcSig.at(it) != signature.at(it) && funcSig.at(it) != parser::TYPE::T_ANY) {
				throw std::runtime_error("there was an error determining '" + identifier + "' function statements");
			}
		}
		return std::get<2>(i->second);
	}

	return nullptr;
}

std::vector<std::tuple<std::string, std::string, std::string>> InterpreterScope::variableList() {
	std::vector<std::tuple<std::string, std::string, std::string>> list;

	for (auto const& var : variableSymbolTable) {
		switch (var.second.first) {
		case parser::TYPE::T_BOOL:
			list.emplace_back(std::make_tuple(var.first, "bool", (var.second.second->b) ? "true" : "false"));
			break;
		case parser::TYPE::T_INT:
			list.emplace_back(std::make_tuple(var.first, "int", std::to_string(var.second.second->i)));
			break;
		case parser::TYPE::T_FLOAT:
			list.emplace_back(std::make_tuple(var.first, "float", std::to_string(var.second.second->f)));
			break;
		case parser::TYPE::T_CHAR:
			list.emplace_back(std::make_tuple(var.first, "char", std::to_string(var.second.second->c)));
			break;
		case parser::TYPE::T_STRING:
			list.emplace_back(std::make_tuple(var.first, "string", var.second.second->s));
			break;
		case parser::TYPE::T_ANY:
			if (var.second.second->a.type() == typeid(nullptr)) {
				list.emplace_back(std::make_tuple(var.first, "any", "null"));
			}
			else if (var.second.second->a.type() == typeid(bool)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::any_cast<bool>(var.second.second->a) ? "true" : "false"));
			}
			else if (var.second.second->a.type() == typeid(__int64_t)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<bool>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(long double)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<long double>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(char)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<char>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(std::string)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::any_cast<std::string>(var.second.second->a)));
			}

			break;
		}
	}

	return list;
}


Interpreter::Interpreter() {
	// add global scope
	scopes.push_back(new InterpreterScope());
	currentExpressionType = parser::TYPE::T_VOID;
	currentProgram = nullptr;
}

Interpreter::Interpreter(InterpreterScope* globalScope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]) {
	// add global scope
	scopes.push_back(globalScope);
	currentExpressionType = parser::TYPE::T_VOID;
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
	// visit expression to update current value/type
	if (decl->expr) {
		decl->expr->accept(this);
	}

	// declare variable, depending on type
	switch (decl->type) {
	case parser::TYPE::T_BOOL:
		scopes.back()->declare(decl->identifier, currentExpressionValue->b);
		break;
	case parser::TYPE::T_INT:
		scopes.back()->declare(decl->identifier, currentExpressionValue->i);
		break;
	case parser::TYPE::T_FLOAT:
		if (currentExpressionType == parser::TYPE::T_INT)
			scopes.back()->declare(decl->identifier, (long double)currentExpressionValue->i);
		else
			scopes.back()->declare(decl->identifier, currentExpressionValue->f);
		break;
	case parser::TYPE::T_CHAR:
		scopes.back()->declare(decl->identifier, currentExpressionValue->c);
		break;
	case parser::TYPE::T_STRING:
		if (currentExpressionType == parser::TYPE::T_CHAR)
			scopes.back()->declare(decl->identifier, currentExpressionValue->c);
		else
			scopes.back()->declare(decl->identifier, currentExpressionValue->s);
		break;
	case parser::TYPE::T_ANY:
		if (currentExpressionType == parser::TYPE::T_BOOL) {
			scopes.back()->declare(decl->identifier, currentExpressionValue->b);
		}
		else if (currentExpressionType == parser::TYPE::T_INT) {
			scopes.back()->declare(decl->identifier, currentExpressionValue->i);
		}
		else if (currentExpressionType == parser::TYPE::T_FLOAT) {
			scopes.back()->declare(decl->identifier, currentExpressionValue->f);
		}
		else if (currentExpressionType == parser::TYPE::T_CHAR) {
			scopes.back()->declare(decl->identifier, currentExpressionValue->c);
		}
		else if (currentExpressionType == parser::TYPE::T_STRING) {
			scopes.back()->declare(decl->identifier, currentExpressionValue->s);
		}
		else if (currentExpressionType == parser::TYPE::T_STRUCT) {
			cp_struct str;
			str.first = currentExpressionTypeName;
			str.second = new std::vector<std::pair<std::string, std::any>>();
			declareStructureTypeVariables(decl->identifier, currentExpressionTypeName, str);
			scopes.back()->declare(decl->identifier, currentExpressionTypeName, str);
		}
		break;
	case parser::TYPE::T_STRUCT: {
		cp_struct str;
		str.first = decl->typeName;
		str.second = new std::vector<std::pair<std::string, std::any>>();
		declareStructureTypeVariables(decl->identifier, decl->typeName, str);
		scopes.back()->declare(decl->identifier, decl->typeName, str);
		break;
	}
	case parser::TYPE::T_ARRAY:
		scopes.back()->declare(decl->identifier, currentExpressionValue->arr);
		break;
	}
}

void visitor::Interpreter::declareStructureTypeVariables(std::string identifier, std::string typeName, cp_struct &str) {
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureType(typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureType(typeName);
	//auto typeStruct = findDeclaredStructureType(typeName);
	for (auto varTypeStruct : typeStruct.variables) {
		auto currentIdentifier = identifier + '.' + varTypeStruct.identifier;
		//switch (varTypeStruct.type) {
		//case parser::TYPE::T_BOOL:
		//	scopes.back()->declare(currentIdentifier, (cp_bool)false);
		//	break;
		//case parser::TYPE::T_INT:
		//	scopes.back()->declare(currentIdentifier, (cp_int)0);
		//	break;
		//case parser::TYPE::T_FLOAT:
		//	scopes.back()->declare(currentIdentifier, ((cp_float)0));
		//	break;
		//case parser::TYPE::T_CHAR:
		//	scopes.back()->declare(currentIdentifier, (cp_char)'\0');
		//	break;
		//case parser::TYPE::T_STRING:
		//	scopes.back()->declare(currentIdentifier, cp_string());
		//	break;
		//case parser::TYPE::T_ANY:
		//	scopes.back()->declare(currentIdentifier, cp_any());
		//	break;
		//case parser::TYPE::T_ARRAY:
		//	scopes.back()->declare(currentIdentifier, cp_array());
		//	break;
		//case parser::TYPE::T_STRUCT:
		//	cp_struct str;
		//	str.first = varTypeStruct.typeName;
		//	scopes.back()->declare(currentIdentifier, str);
		//	declareStructureTypeVariables(currentIdentifier, varTypeStruct.typeName);
		//	break;
		//}
		if (varTypeStruct.type == parser::TYPE::T_STRUCT) {
			cp_struct subStr;
			subStr.first = varTypeStruct.typeName;
			subStr.second = new std::vector<std::pair<std::string, std::any>>();
			declareStructureTypeVariables(currentIdentifier, varTypeStruct.typeName, subStr);
			scopes.back()->declare(currentIdentifier, varTypeStruct.typeName, subStr);
			str.second->push_back(std::pair<std::string, std::any>(varTypeStruct.identifier, subStr));
		}
		else {
			Value_t* val;
			switch (varTypeStruct.type) {
			case parser::TYPE::T_BOOL:
				val = scopes.back()->declare(currentIdentifier, (cp_bool)false);
				break;
			case parser::TYPE::T_INT:
				val = scopes.back()->declare(currentIdentifier, (cp_int)0);
				break;
			case parser::TYPE::T_FLOAT:
				val = scopes.back()->declare(currentIdentifier, ((cp_float)0));
				break;
			case parser::TYPE::T_CHAR:
				val = scopes.back()->declare(currentIdentifier, (cp_char)'\0');
				break;
			case parser::TYPE::T_STRING:
				val = scopes.back()->declare(currentIdentifier, cp_string());
				break;
			case parser::TYPE::T_ANY:
				val = scopes.back()->declare(currentIdentifier, cp_any());
				break;
			case parser::TYPE::T_ARRAY:
				val = scopes.back()->declare(currentIdentifier, cp_array());
				break;
			}
			str.second->push_back(std::pair<std::string, std::any>(varTypeStruct.identifier, val));
		}
	}
}

parser::StructureDefinition_t InterpreterScope::findDeclaredStructureType(std::string identifier) {
	for (auto variable : structures) {
		if (variable.identifier == identifier) {
			return variable;
		}
	}
	throw std::runtime_error("Can't found '" + identifier + "'.");
}

void visitor::Interpreter::visit(parser::ASTAssignmentNode* assign) {
	std::string actualIdentifier = assign->identifier;
	if (assign->identifierVector.size() > 1) {
		actualIdentifier = axe::join(assign->identifierVector, ".");
	}

	// determine innermost scope in which variable is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(actualIdentifier); i--);

	// visit expression node to update current value/type
	assign->expr->accept(this);

	auto type = scopes[i]->typeof(actualIdentifier);
	if (type != parser::TYPE::T_ARRAY) {
		type = currentExpressionType;
	}

	// redeclare variable, depending on type
	switch (type) {
	case parser::TYPE::T_BOOL:
		scopes[i]->declare(actualIdentifier, currentExpressionValue->b);
		break;
	case parser::TYPE::T_INT:
		scopes[i]->declare(actualIdentifier, currentExpressionValue->i);
		break;
	case parser::TYPE::T_FLOAT:
		scopes[i]->declare(actualIdentifier, currentExpressionValue->f);
		break;
	case parser::TYPE::T_CHAR:
		scopes[i]->declare(actualIdentifier, currentExpressionValue->c);
		break;
	case parser::TYPE::T_STRING:
		scopes[i]->declare(actualIdentifier, currentExpressionValue->s);
		break;
	case parser::TYPE::T_STRUCT: {
		//if (axe::contains(actualIdentifier, ".")) {
		//	auto identifiers = axe::split(actualIdentifier, '.');
		//	auto str = scopes.back()->valueof(identifiers[0]);
		//	auto currentVal = str->str.second;
		//	size_t s = 0;
		//	for (s = 0; s < identifiers.size() - 1; ++s) {
		//		for (size_t vi = 0; vi < currentVal->size(); ++vi) {
		//			if (currentVal->at(vi).first == identifiers.at(i + 1)) {
		//				currentVal = std::any_cast<std::vector<std::pair<std::string, std::any>>*>(currentVal->at(vi).second);
		//			}
		//		}
		//	}
		//	std::any newVal;
		//	switch (currentExpressionType) {
		//	case parser::TYPE::T_BOOL:
		//		newVal = currentExpressionValue->b;
		//		break;
		//	case parser::TYPE::T_INT:
		//		newVal = currentExpressionValue->i;
		//		break;
		//	case parser::TYPE::T_FLOAT:
		//		newVal = currentExpressionValue->f;
		//		break;
		//	case parser::TYPE::T_CHAR:
		//		newVal = currentExpressionValue->c;
		//		break;
		//	case parser::TYPE::T_STRING:
		//		newVal = currentExpressionValue->s;
		//		break;
		//	}

		//	//currentVal->at() = newVal;


		//}
		scopes.back()->declare(actualIdentifier, currentExpressionValue->str.first, currentExpressionValue->str);
		//declareStructureTypeVariables(assign->identifier, currentExpressionValue.str.first);
		break;
	}
	case parser::TYPE::T_ARRAY:
		Value_t* val = scopes[i]->valueof(actualIdentifier);
		std::vector<std::any>* currentVal = val->arr;
		size_t s = 0;
		for (s = 0; s < assign->accessVector.size() - 1; ++s) {
			currentVal = std::any_cast<std::vector<std::any>*>(currentVal->at(assign->accessVector[s]));
		}
		std::any newVal;
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			newVal = currentExpressionValue->b;
			break;
		case parser::TYPE::T_INT:
			newVal = currentExpressionValue->i;
			break;
		case parser::TYPE::T_FLOAT:
			newVal = currentExpressionValue->f;
			break;
		case parser::TYPE::T_CHAR:
			newVal = currentExpressionValue->c;
			break;
		case parser::TYPE::T_STRING:
			newVal = currentExpressionValue->s;
			break;
		}

		currentVal->at(assign->accessVector[s]) = newVal;

		break;
	}
}

void printValue(std::any value) {
	if (value.type() == typeid(nullptr)) {
		std::cout << "null";
	}
	else if (value.type() == typeid(bool)) {
		std::cout << std::any_cast<bool>(value);
	}
	else if (value.type() == typeid(__int64_t)) {
		std::cout << std::any_cast<__int64_t>(value);
	}
	else if (value.type() == typeid(long double)) {
		std::cout << std::any_cast<long double>(value);
	}
	else if (value.type() == typeid(char)) {
		std::cout << std::any_cast<char>(value);
	}
	else if (value.type() == typeid(std::string)) {
		std::cout << std::any_cast<std::string>(value);
	}
	else {
		std::cout << "null";
	}
}

void printArray(std::vector<std::any>* value) {
	std::cout << '[';
	auto vec_value = std::any_cast<std::vector<std::any>*>(value);
	for (auto i = 0; i < vec_value->size(); ++i) {
		std::any val = vec_value->at(i);
		if (val.type() == typeid(std::vector<std::any>*)) {
			printArray(std::any_cast<std::vector<std::any>*>(val));
		}
		else {
			printValue(val);
		}
		if (i < vec_value->size() - 1) {
			std::cout << ',';
		}
	}
	std::cout << ']';
}

void visitor::Interpreter::visit(parser::ASTPrintNode* print) {
	// visit expression node to update current value/type
	print->expr->accept(this);

	// print, depending on type
	switch (currentExpressionType) {
	case parser::TYPE::T_BOOL:
		std::cout << ((currentExpressionValue->b) ? "true" : "false");
		break;
	case parser::TYPE::T_INT:
		std::cout << currentExpressionValue->i;
		break;
	case parser::TYPE::T_FLOAT:
		std::cout << currentExpressionValue->f;
		break;
	case parser::TYPE::T_CHAR:
		std::cout << currentExpressionValue->c;
		break;
	case parser::TYPE::T_STRING:
		std::cout << currentExpressionValue->s;
		break;
	case parser::TYPE::T_ANY:
		if (currentExpressionValue->a.type() == typeid(nullptr)) {
			std::cout << "null";
		}
		else if (currentExpressionValue->a.type() == typeid(bool)) {
			std::cout << std::any_cast<bool>(currentExpressionValue->a);
		}
		else if (currentExpressionValue->a.type() == typeid(__int64_t)) {
			std::cout << std::any_cast<int>(currentExpressionValue->a);
		}
		else if (currentExpressionValue->a.type() == typeid(long double)) {
			std::cout << std::any_cast<long double>(currentExpressionValue->a);
		}
		else if (currentExpressionValue->a.type() == typeid(char)) {
			std::cout << std::any_cast<char>(currentExpressionValue->a);
		}
		else if (currentExpressionValue->a.type() == typeid(std::string)) {
			std::cout << std::any_cast<std::string>(currentExpressionValue->a);
		}
		break;
	case parser::TYPE::T_STRUCT:
		std::cout << "struct " << currentExpressionValue->str.first << " { ... }";
		break;
	case parser::TYPE::T_ARRAY:
		printArray(currentExpressionValue->arr);
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
	std::vector<std::pair<parser::TYPE, Value_t*>> currentFunctionArguments;

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
		case parser::TYPE::T_BOOL:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->b);
			break;
		case parser::TYPE::T_INT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->i);
			break;
		case parser::TYPE::T_FLOAT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->f);
			break;
		case parser::TYPE::T_CHAR:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->c);
			break;
		case parser::TYPE::T_STRING:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->s);
			break;
		case parser::TYPE::T_ANY:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->a);
			break;
		case parser::TYPE::T_STRUCT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->str.first, currentFunctionArguments[i].second->str);
			//declareStructureTypeVariables(currentFunctionParameters[i], currentFunctionArguments[i].second.str.first);
			break;
		case parser::TYPE::T_ARRAY:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->arr);
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

	// close scope
	scopes.pop_back();
}

void visitor::Interpreter::visit(parser::ASTIfNode* ifNode) {
	// evaluate if condition
	ifNode->condition->accept(this);

	// execute appropriate blocks
	if (currentExpressionValue->b) {
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

	while (currentExpressionValue->b) {
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

void visitor::Interpreter::visit(parser::ASTStructDefinitionNode* structure) {
	// add function to symbol table
	scopes.back()->declareStructureType(structure->identifier, structure->variables, structure->row, structure->col);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<bool>* lit) {
	Value_t* value = new Value_t();
	value->b = lit->val;
	currentExpressionType = parser::TYPE::T_BOOL;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<__int64_t>* lit) {
	Value_t* value = new Value_t();
	value->i = lit->val;
	currentExpressionType = parser::TYPE::T_INT;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<long double>* lit) {
	Value_t* value = new Value_t();
	value->f = lit->val;
	currentExpressionType = parser::TYPE::T_FLOAT;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<char>* lit) {
	Value_t* value = new Value_t();
	value->c = lit->val;
	currentExpressionType = parser::TYPE::T_CHAR;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<std::string>* lit) {
	Value_t* value = new Value_t();
	value->s = lit->val;
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<std::any>* lit) {
	Value_t* value = new Value_t();
	value->a = lit->val;
	currentExpressionType = parser::TYPE::T_ANY;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<std::vector<std::any>*>* lit) {
	Value_t* value = new Value_t();
	value->arr = lit->val;
	determineArrayType(lit->val);
	currentExpressionValue = value;
}

void Interpreter::determineArrayType(std::vector<std::any>* arr) {
	if (arr->size() > 0) {
		std::any val = arr->at(0);
		if (val.type() == typeid(bool)) {
			currentExpressionType = parser::TYPE::T_BOOL;
		}
		else if (val.type() == typeid(__int64_t)) {
			currentExpressionType = parser::TYPE::T_INT;
		}
		else if (val.type() == typeid(long double)) {
			currentExpressionType = parser::TYPE::T_FLOAT;
		}
		else if (val.type() == typeid(char)) {
			currentExpressionType = parser::TYPE::T_CHAR;
		}
		else if (val.type() == typeid(std::string)) {
			currentExpressionType = parser::TYPE::T_STRING;
		}
		else if (val.type() == typeid(std::any)) {
			currentExpressionType = parser::TYPE::T_ANY;
		}
		else if (val.type() == typeid(std::vector<std::any>*)) {
			determineArrayType(any_cast<std::vector<std::any>*>(val));
		}
		else {
			currentExpressionType = parser::TYPE::T_ANY;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTBinaryExprNode* bin) {
	// operator
	std::string op = bin->op;

	// visit left node first
	bin->left->accept(this);
	parser::TYPE l_type = currentExpressionType;
	Value_t* l_value = currentExpressionValue;

	// then right node
	bin->right->accept(this);
	parser::TYPE r_type = currentExpressionType;
	Value_t* r_value = currentExpressionValue;

	// expression struct
	Value_t* value = new Value_t();

	// arithmetic operators for now
	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		// two ints
		if (l_type == parser::TYPE::T_INT && r_type == parser::TYPE::T_INT) {
			currentExpressionType = parser::TYPE::T_INT;
			if (op == "+") {
				value->i = l_value->i + r_value->i;
			}
			else if (op == "-") {
				value->i = l_value->i - r_value->i;
			}
			else if (op == "*") {
				value->i = l_value->i * r_value->i;
			}
			else if (op == "/") {
				if (r_value->i == 0) {
					throw std::runtime_error(msgHeader(bin->row, bin->col) + "division by zero encountered.");
				}
				value->i = l_value->i / r_value->i;
			}
			else if (op == "%") {
				value->i = l_value->i % r_value->i;
			}
		}
		else if (l_type == parser::TYPE::T_FLOAT || r_type == parser::TYPE::T_FLOAT) { // at least one real
			currentExpressionType = parser::TYPE::T_FLOAT;
			long double l = l_value->f, r = r_value->f;
			if (l_type == parser::TYPE::T_INT) {
				l = l_value->i;
			}
			if (r_type == parser::TYPE::T_INT) {
				r = r_value->i;
			}
			if (op == "+") {
				value->f = l + r;
			}
			else if (op == "-") {
				value->f = l - r;
			}
			else if (op == "*") {
				value->f = l * r;
			}
			else if (op == "/") {
				if (r == 0) {
					throw std::runtime_error(msgHeader(bin->row, bin->col) + "division by zero encountered.");
				}
				value->f = l / r;
			}
		}
		else if (l_type == parser::TYPE::T_CHAR && r_type == parser::TYPE::T_STRING) { // char and string
			currentExpressionType = parser::TYPE::T_STRING;
			value->s = l_value->c + r_value->s;
		}
		else if (l_type == parser::TYPE::T_STRING && r_type == parser::TYPE::T_CHAR) { // string and char
			currentExpressionType = parser::TYPE::T_STRING;
			value->s = l_value->s + r_value->c;
		}
		else { // remaining case is for strings
			currentExpressionType = parser::TYPE::T_STRING;
			value->s = l_value->s + r_value->s;
		}
	}
	else if (op == "and" || op == "or") { // now bool
		currentExpressionType = parser::TYPE::T_BOOL;
		if (op == "and") {
			value->b = l_value->b && r_value->b;
		}
		else if (op == "or") {
			value->b = l_value->b || r_value->b;
		}
	}
	else { // now comparator operators
		currentExpressionType = parser::TYPE::T_BOOL;
		if (l_type == parser::TYPE::T_BOOL) {
			value->b = (op == "==") ? l_value->b == r_value->b : l_value->b != r_value->b;
		}
		else if (l_type == parser::TYPE::T_STRING) {
			value->b = (op == "==") ? l_value->s == r_value->s : l_value->s != r_value->s;
		}
		else {
			long double l = l_value->f, r = r_value->f;

			if (l_type == parser::TYPE::T_INT) {
				l = l_value->i;
			}
			if (r_type == parser::TYPE::T_INT) {
				r = r_value->i;
			}
			if (op == "==") {
				value->b = l == r;
			}
			else if (op == "!=") {
				value->b = l != r;
			}
			else if (op == "<") {
				value->b = l < r;
			}
			else if (op == ">") {
				value->b = l > r;
			}
			else if (op == ">=") {
				value->b = l >= r;
			}
			else if (op == "<=") {
				value->b = l <= r;
			}
		}
	}

	// update current expression
	currentExpressionValue = value;
}

void visitor::Interpreter::visit(parser::ASTIdentifierNode* id) {
	std::string actualIdentifier = id->identifier;
	if (id->identifierVector.size() > 1) {
		actualIdentifier = axe::join(id->identifierVector, ".");
	}

	// determine innermost scope in which variable is declared
	unsigned long i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(actualIdentifier); i--);

	// update current expression
	currentExpressionType = scopes[i]->typeof(actualIdentifier);
	currentExpressionValue = scopes[i]->valueof(actualIdentifier);
	if (currentExpressionType == parser::TYPE::T_STRUCT) {
		currentExpressionTypeName = scopes[i]->typenameof(actualIdentifier);
	}
	else {
		currentExpressionTypeName = "";
	}
}

void visitor::Interpreter::visit(parser::ASTUnaryExprNode* un) {
	// update current expression
	un->expr->accept(this);
	switch (currentExpressionType) {
	case parser::TYPE::T_INT:
		if (un->unaryOp == "-")
			currentExpressionValue->i *= -1;
		break;
	case parser::TYPE::T_FLOAT:
		if (un->unaryOp == "-")
			currentExpressionValue->f *= -1;
		break;
	case parser::TYPE::T_BOOL:
		currentExpressionValue->b ^= 1;
	}
}

void visitor::Interpreter::visit(parser::ASTExprFunctionCallNode* func) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;
	std::vector<std::pair<parser::TYPE, Value_t*>> currentFunctionArguments;

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

void visitor::Interpreter::visit(parser::ASTTypeParseNode* typeParser) {
	// visit expression node to update current value/type
	typeParser->expr->accept(this);

	switch (typeParser->type) {
	case parser::TYPE::T_BOOL:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue->b = currentExpressionValue->i != 0;
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue->b = currentExpressionValue->f != .0;
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue->b = currentExpressionValue->c != '\0';
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue->b = currentExpressionValue->s.empty();
			break;
		}
		break;

	case parser::TYPE::T_INT:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue->i = currentExpressionValue->b;
			break;
		case parser::TYPE::T_INT:
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue->i = static_cast<long long>(round(currentExpressionValue->f));
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue->i = currentExpressionValue->c;
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue->i = std::stoll(currentExpressionValue->s);
			break;
		}
		break;

	case parser::TYPE::T_FLOAT:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue->f = currentExpressionValue->b;
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue->f = static_cast<long double>(currentExpressionValue->i);
			break;
		case parser::TYPE::T_FLOAT:
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue->f = currentExpressionValue->c;
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue->f = std::stold(currentExpressionValue->s);
			break;
		}
		break;

	case parser::TYPE::T_STRING:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue->s = currentExpressionValue->b ? "true" : "false";
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue->s = std::to_string(currentExpressionValue->i);
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue->s = std::to_string(currentExpressionValue->f);
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue->s = currentExpressionValue->c;
			break;
		case parser::TYPE::T_STRING:
			break;
		}
		break;

	}

	currentExpressionType = typeParser->type;
}

void visitor::Interpreter::visit(parser::ASTThisNode* thisNode) {
	Value_t* value = new Value_t();
	value->s = scopes.back()->getName();
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = std::move(value);
}

void visitor::Interpreter::visit(parser::ASTExprReadNode* read) {
	std::string line;
	std::getline(std::cin, line);

	currentExpressionValue->s = std::move(line);
}

std::pair<parser::TYPE, Value_t*> Interpreter::currentExpr() {
	return std::move(std::make_pair(currentExpressionType, currentExpressionValue));
};

std::string Interpreter::msgHeader(unsigned int row, unsigned int col) {
	return currentProgram->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
}

std::string visitor::typeStr(parser::TYPE t) {
	switch (t) {
	case parser::TYPE::T_VOID:
		return "void";
	case parser::TYPE::T_BOOL:
		return "bool";
	case parser::TYPE::T_INT:
		return "int";
	case parser::TYPE::T_FLOAT:
		return "float";
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
