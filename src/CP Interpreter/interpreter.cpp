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

parser::StructureDefinition_t InterpreterScope::findDeclaredStructureDefinition(std::string identifier) {
	for (auto variable : structures) {
		if (variable.identifier == identifier) {
			return variable;
		}
	}
	throw std::runtime_error("can't found '" + identifier + "'");
}

bool InterpreterScope::alreadyDeclared(std::string identifier) {
	return variableSymbolTable.find(identifier) != variableSymbolTable.end();
}

bool InterpreterScope::alreadyDeclared(std::string identifier, std::vector<parser::TYPE> signature) {
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
				//return false;
				found = false;
			}
		}
		if (found) return true;
	}

	return false;
}

Value_t* InterpreterScope::declareNull(std::string identifier, parser::TYPE type) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->setNull();
	variableSymbolTable[identifier] = std::make_pair(type, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_bool boolValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(boolValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_BOOL, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_int intValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(intValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_INT, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_float realValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(realValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_FLOAT, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_char charValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(charValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_CHAR, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_string stringValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(stringValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_STRING, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_any anyValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(anyValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_ANY, value);
	return value;
}

Value_t* InterpreterScope::declare(std::string identifier, cp_array arrValue) {
	Value_t* value;
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
	}
	value->set(arrValue);
	variableSymbolTable[identifier] = std::make_pair(parser::TYPE::T_ARRAY, value);
	return value;
}


Value_t* InterpreterScope::declare(std::string identifier, cp_struct strValue, std::vector<InterpreterScope*> scopes) {
	Value_t* value;

	// if already declared, we get current value to always work with the same memory value
	// this is important when we handle with structure variables, so they can be handled by self
	if (alreadyDeclared(identifier)) {
		value = valueof(identifier);
	}
	else {
		value = new Value_t();
		value->set(strValue);
	}

	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureType(strValue.first); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(strValue.first);

	for (size_t i = 0; i < value->str.second.size(); ++i) {
		auto currentIdentifier = identifier + '.' + typeStruct.variables.at(i).identifier;
		if (value->str.second.at(i).second->currentType == parser::TYPE::T_STRUCT) {
			value->str.second.at(i).second = strValue.second.at(i).second;
			variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_STRUCT, value->str.second.at(i).second);
			typeNamesTable[currentIdentifier] = value->str.second.at(i).first;
			declare(currentIdentifier, strValue.second.at(i).second->str, scopes);
		}
		else {
			value->str.second.at(i).second = strValue.second.at(i).second;
			switch (value->str.second.at(i).second->currentType) {
			case parser::TYPE::T_BOOL:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_BOOL, value->str.second.at(i).second);
				break;
			case parser::TYPE::T_INT:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_INT, value->str.second.at(i).second);
				break;
			case parser::TYPE::T_FLOAT:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_FLOAT, value->str.second.at(i).second);
				break;
			case parser::TYPE::T_CHAR:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_CHAR, value->str.second.at(i).second);
				break;
			case parser::TYPE::T_STRING:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_STRING, value->str.second.at(i).second);
				break;
			case parser::TYPE::T_ARRAY:
				variableSymbolTable[currentIdentifier] = std::make_pair(parser::TYPE::T_ARRAY, value->str.second.at(i).second);
				break;
			}
		}
	}

	return value;
}

void InterpreterScope::declareStructureDefinition(std::string name, std::vector<parser::VariableDefinition_t> variables, unsigned int row, unsigned int col) {
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
				//throw std::runtime_error("there was an error determining '" + identifier + "' function parameters");
				found = false;
			}
		}
		if (found) return std::get<1>(fun.second);
	}

	throw std::runtime_error("something went wrong when determining the typename of '" + identifier + "' variable");
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
			else if (var.second.second->a.type() == typeid(cp_bool)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::any_cast<cp_bool>(var.second.second->a) ? "true" : "false"));
			}
			else if (var.second.second->a.type() == typeid(cp_int)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<cp_int>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(cp_float)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<cp_float>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(cp_char)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::to_string(std::any_cast<cp_char>(var.second.second->a))));
			}
			else if (var.second.second->a.type() == typeid(cp_string)) {
				list.emplace_back(std::make_tuple(var.first, "any", std::any_cast<cp_string>(var.second.second->a)));
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

		// declare variable, depending on type
		switch (decl->type) {
		case parser::TYPE::T_BOOL:
			scopes.back()->declare(decl->identifier, currentExpressionValue.b);
			break;
		case parser::TYPE::T_INT:
			scopes.back()->declare(decl->identifier, currentExpressionValue.i);
			break;
		case parser::TYPE::T_FLOAT:
			if (currentExpressionType == parser::TYPE::T_INT)
				scopes.back()->declare(decl->identifier, (cp_float)currentExpressionValue.i);
			else
				scopes.back()->declare(decl->identifier, currentExpressionValue.f);
			break;
		case parser::TYPE::T_CHAR:
			scopes.back()->declare(decl->identifier, currentExpressionValue.c);
			break;
		case parser::TYPE::T_STRING:
			if (currentExpressionType == parser::TYPE::T_CHAR)
				scopes.back()->declare(decl->identifier, std::to_string(currentExpressionValue.c));
			else
				scopes.back()->declare(decl->identifier, currentExpressionValue.s);
			break;
		case parser::TYPE::T_ANY:
			switch (currentExpressionType) {
			case parser::TYPE::T_BOOL:
				scopes.back()->declare(decl->identifier, currentExpressionValue.b);
				break;
			case parser::TYPE::T_INT:
				scopes.back()->declare(decl->identifier, currentExpressionValue.i);
				break;
			case parser::TYPE::T_FLOAT:
				scopes.back()->declare(decl->identifier, currentExpressionValue.f);
				break;
			case parser::TYPE::T_CHAR:
				scopes.back()->declare(decl->identifier, currentExpressionValue.c);
				break;
			case parser::TYPE::T_STRING:
				scopes.back()->declare(decl->identifier, currentExpressionValue.s);
				break;
			case parser::TYPE::T_ARRAY:
				scopes.back()->declare(decl->identifier, currentExpressionValue.arr);
				break;
			case parser::TYPE::T_STRUCT: {
				//cp_struct str = declareStructureTypeVariables(decl->identifier, currentExpressionTypeName);
				//scopes.back()->declare(decl->identifier, str, scopes);
				scopes.back()->declare(decl->identifier, currentExpressionValue.str, scopes);
				break;
			}
			}
			break;
		case parser::TYPE::T_ARRAY:
			scopes.back()->declare(decl->identifier, currentExpressionValue.arr);
			break;
		case parser::TYPE::T_STRUCT: {
			//cp_struct str = declareStructureTypeVariables(decl->identifier, decl->typeName);
			//scopes.back()->declare(decl->identifier, str, scopes);
			//if (decl->expr) {
				scopes.back()->declare(decl->identifier, currentExpressionValue.str, scopes);
			//}
			break;
		}
		}
	}
	else {
		scopes.back()->declareNull(decl->identifier, decl->type);
	}


}

cp_struct visitor::Interpreter::declareStructureTypeVariables(std::string identifier, std::string typeName) {
	cp_struct str;
	str.first = typeName;
	str.second = cp_struct_values();
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclaredStructureType(typeName); --i);
	auto typeStruct = scopes[i]->findDeclaredStructureDefinition(typeName);
	for (auto varTypeStruct : typeStruct.variables) {
		auto currentIdentifier = identifier + '.' + varTypeStruct.identifier;
		if (varTypeStruct.type == parser::TYPE::T_STRUCT) {
			cp_struct subStr = declareStructureTypeVariables(currentIdentifier, varTypeStruct.typeName);
			Value_t* val = scopes.back()->declare(currentIdentifier, subStr, scopes);
			str.second.push_back(cp_struct_value(varTypeStruct.identifier, val));
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
			str.second.push_back(cp_struct_value(varTypeStruct.identifier, val));
		}
	}
	return str;
}

void visitor::Interpreter::visit(parser::ASTAssignmentNode* assign) {
	std::string actualIdentifier = assign->identifier;
	if (assign->identifierVector.size() > 1) {
		actualIdentifier = axe::join(assign->identifierVector, ".");
	}

	// determine innermost scope in which variable is declared
	size_t i;
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
		scopes[i]->declare(actualIdentifier, currentExpressionValue.b);
		break;
	case parser::TYPE::T_INT:
		scopes[i]->declare(actualIdentifier, currentExpressionValue.i);
		break;
	case parser::TYPE::T_FLOAT:
		scopes[i]->declare(actualIdentifier, currentExpressionValue.f);
		break;
	case parser::TYPE::T_CHAR:
		scopes[i]->declare(actualIdentifier, currentExpressionValue.c);
		break;
	case parser::TYPE::T_STRING:
		scopes[i]->declare(actualIdentifier, currentExpressionValue.s);
		break;
	case parser::TYPE::T_STRUCT: {
		scopes[i]->declare(actualIdentifier, currentExpressionValue.str, scopes);
		break;
	}
	case parser::TYPE::T_ARRAY:
		Value_t* val = scopes[i]->valueof(actualIdentifier);
		cp_array* currentVal = &val->arr;
		size_t s = 0;
		for (s = 0; s < assign->accessVector.size() - 1; ++s) {
			currentVal = &currentVal->at(assign->accessVector[s])->arr;
		}

		Value_t* newVal = new Value_t();
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			newVal->set(currentExpressionValue.b);
			break;
		case parser::TYPE::T_INT:
			newVal->set(currentExpressionValue.i);
			break;
		case parser::TYPE::T_FLOAT:
			newVal->set(currentExpressionValue.f);
			break;
		case parser::TYPE::T_CHAR:
			newVal->set(currentExpressionValue.c);
			break;
		case parser::TYPE::T_STRING:
			newVal->set(currentExpressionValue.s);
			break;
		case parser::TYPE::T_ARRAY:
			newVal->set(currentExpressionValue.arr);
			break;
		case parser::TYPE::T_STRUCT:
			newVal->set(currentExpressionValue.str);
			break;
		}

		currentVal->at(assign->accessVector[s]) = newVal;

		break;
	}
}

void printValue(Value_t* value) {
	if (value->currentType == parser::TYPE::T_NULL) {
		std::cout << "null";
	}
	else if (value->currentType == parser::TYPE::T_BOOL) {
		std::cout << value->b;
	}
	else if (value->currentType == parser::TYPE::T_INT) {
		std::cout << value->i;
	}
	else if (value->currentType == parser::TYPE::T_FLOAT) {
		std::cout << value->f;
	}
	else if (value->currentType == parser::TYPE::T_CHAR) {
		std::cout << value->c;
	}
	else if (value->currentType == parser::TYPE::T_STRING) {
		std::cout << value->s;
	}
	else {
		std::cout << "null";
	}
}

void printArray(cp_array value) {
	std::cout << '[';
	for (auto i = 0; i < value.size(); ++i) {
		if (value.at(i)->currentType == parser::TYPE::T_ARRAY) {
			printArray(value.at(i)->arr);
		}
		else {
			printValue(value.at(i));
		}
		if (i < value.size() - 1) {
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
		std::cout << ((currentExpressionValue.b) ? "true" : "false");
		break;
	case parser::TYPE::T_INT:
		std::cout << currentExpressionValue.i;
		break;
	case parser::TYPE::T_FLOAT:
		std::cout << currentExpressionValue.f;
		break;
	case parser::TYPE::T_CHAR:
		std::cout << currentExpressionValue.c;
		break;
	case parser::TYPE::T_STRING:
		std::cout << currentExpressionValue.s;
		break;
	case parser::TYPE::T_ANY:
		if (currentExpressionValue.a.type() == typeid(nullptr)) {
			std::cout << "null";
		}
		else if (currentExpressionValue.a.type() == typeid(cp_bool)) {
			std::cout << std::any_cast<cp_bool>(currentExpressionValue.a);
		}
		else if (currentExpressionValue.a.type() == typeid(cp_int)) {
			std::cout << std::any_cast<cp_int>(currentExpressionValue.a);
		}
		else if (currentExpressionValue.a.type() == typeid(cp_float)) {
			std::cout << std::any_cast<cp_float>(currentExpressionValue.a);
		}
		else if (currentExpressionValue.a.type() == typeid(cp_char)) {
			std::cout << std::any_cast<cp_char>(currentExpressionValue.a);
		}
		else if (currentExpressionValue.a.type() == typeid(cp_string)) {
			std::cout << std::any_cast<cp_string>(currentExpressionValue.a);
		}
		break;
	case parser::TYPE::T_STRUCT:
		std::cout << currentExpressionValue.str.first << " { ... }";
		break;
	case parser::TYPE::T_ARRAY:
		printArray(currentExpressionValue.arr);
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
		Value_t* value = new Value_t();
		value->copyFrom(&currentExpressionValue);
		currentFunctionArguments.emplace_back(currentExpressionType, value);
	}

	// update the global vector current_function_arguments
	for (auto arg : currentFunctionArguments) {
		this->currentFunctionArguments.push_back(arg);
	}

	// determine in which scope the function is declared
	size_t i;
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
	for (size_t i = scopes.size() - 1; i >= 0; --i) {
		if (!scopes[i]->getName().empty()) {
			returnFromFunctionName = scopes[i]->getName();
			returnFromFunction = true;
			break;
		}
	}
}

cp_struct visitor::Interpreter::redeclareStructureTypeVariables(std::string identifier, cp_struct str) {
	cp_struct rstr;
	rstr.first = str.first;
	rstr.second = cp_struct_values();

	for (size_t i = 0; i < str.second.size(); ++i) {
		cp_struct_value variable = str.second.at(i);
		auto currentIdentifier = identifier + '.' + variable.first;
		if (variable.second->currentType == parser::TYPE::T_STRUCT) {
			cp_struct subStr = redeclareStructureTypeVariables(currentIdentifier, variable.second->str);
			Value_t* val = scopes.back()->declare(currentIdentifier, subStr, scopes);
			rstr.second.push_back(cp_struct_value(variable.first, val));
		}
		else {
			Value_t* val;
			switch (variable.second->currentType) {
			case parser::TYPE::T_BOOL:
				val = scopes.back()->declare(currentIdentifier, variable.second->b);
				break;
			case parser::TYPE::T_INT:
				val = scopes.back()->declare(currentIdentifier, variable.second->i);
				break;
			case parser::TYPE::T_FLOAT:
				val = scopes.back()->declare(currentIdentifier, variable.second->f);
				break;
			case parser::TYPE::T_CHAR:
				val = scopes.back()->declare(currentIdentifier, variable.second->c);
				break;
			case parser::TYPE::T_STRING:
				val = scopes.back()->declare(currentIdentifier, variable.second->s);
				break;
			case parser::TYPE::T_ANY:
				val = scopes.back()->declare(currentIdentifier, variable.second->a);
				break;
			case parser::TYPE::T_ARRAY:
				val = scopes.back()->declare(currentIdentifier, variable.second->arr);
				break;
			}
			rstr.second.push_back(cp_struct_value(variable.first, val));
		}
	}
	return rstr;
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
		case parser::TYPE::T_STRUCT: {
			//cp_struct rstr = redeclareStructureTypeVariables(currentFunctionParameters[i], currentFunctionArguments[i].second->str);
			//scopes.back()->declare(currentFunctionParameters[i], rstr, scopes);
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->str, scopes);
			break;
		}
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
	if (currentExpressionValue.b) {
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

	while (currentExpressionValue.b) {
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
	scopes.back()->declareStructureDefinition(structure->identifier, structure->variables, structure->row, structure->col);
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_bool>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_BOOL;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_int>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_INT;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_float>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_FLOAT;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_char>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_CHAR;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_string>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_any>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_ANY;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_array>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	determineArrayType(lit->val);
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_struct>* lit) {
	Value_t* value = new Value_t();
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_STRUCT;
	currentExpressionValue = *value;
	currentExpressionTypeName = lit->val.first;
}

void Interpreter::determineArrayType(cp_array arr) {
	if (arr.size() > 0) {
		Value_t* val = arr.at(0);
		if (val->currentType == parser::TYPE::T_ARRAY) {
			determineArrayType(val->arr);
		}
		else {
			currentExpressionType = val->currentType;
		}
	}
}

void visitor::Interpreter::visit(parser::ASTBinaryExprNode* bin) {
	// operator
	std::string op = bin->op;

	// visit left node first
	bin->left->accept(this);
	parser::TYPE l_type = currentExpressionType;
	Value_t l_value = currentExpressionValue;

	// then right node
	bin->right->accept(this);
	parser::TYPE r_type = currentExpressionType;
	Value_t r_value = currentExpressionValue;

	// expression struct
	Value_t value = Value_t();

	// arithmetic operators for now
	if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
		// two ints
		if (l_type == parser::TYPE::T_INT && r_type == parser::TYPE::T_INT) {
			currentExpressionType = parser::TYPE::T_INT;
			if (op == "+") {
				value.set((cp_int)(l_value.i + r_value.i));
			}
			else if (op == "-") {
				value.set((cp_int)(l_value.i - r_value.i));
			}
			else if (op == "*") {
				value.set((cp_int)(l_value.i * r_value.i));
			}
			else if (op == "/") {
				if (r_value.i == 0) {
					throw std::runtime_error(msgHeader(bin->row, bin->col) + "division by zero encountered.");
				}
				value.set((cp_int)(l_value.i / r_value.i));
			}
			else if (op == "%") {
				value.set((cp_int)(l_value.i % r_value.i));
			}
		}
		else if (l_type == parser::TYPE::T_FLOAT || r_type == parser::TYPE::T_FLOAT) { // at least one real
			currentExpressionType = parser::TYPE::T_FLOAT;
			cp_float l = l_value.f, r = r_value.f;
			if (l_type == parser::TYPE::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == parser::TYPE::T_INT) {
				r = cp_float(r_value.i);
			}
			if (op == "+") {
				value.set((cp_float)(l + r));
			}
			else if (op == "-") {
				value.set((cp_float)(l - r));
			}
			else if (op == "*") {
				value.set((cp_float)(l * r));
			}
			else if (op == "/") {
				if (r == 0) {
					throw std::runtime_error(msgHeader(bin->row, bin->col) + "division by zero encountered.");
				}
				value.set((cp_float)l / r);
			}
		}
		else if (l_type == parser::TYPE::T_CHAR && r_type == parser::TYPE::T_STRING) { // char and string
			currentExpressionType = parser::TYPE::T_STRING;
			value.set(cp_string(l_value.c + r_value.s));
		}
		else if (l_type == parser::TYPE::T_STRING && r_type == parser::TYPE::T_CHAR) { // string and char
			currentExpressionType = parser::TYPE::T_STRING;
			value.set(cp_string(l_value.s + r_value.c));
		}
		else { // remaining case is for strings
			currentExpressionType = parser::TYPE::T_STRING;
			value.set(cp_string(l_value.s + r_value.s));
		}
	}
	else if (op == "and" || op == "or") { // now bool
		currentExpressionType = parser::TYPE::T_BOOL;
		if (op == "and") {
			value.set((cp_bool)(l_value.b && r_value.b));
		}
		else if (op == "or") {
			value.set((cp_bool)(l_value.b || r_value.b));
		}
	}
	else { // now comparator operators
		currentExpressionType = parser::TYPE::T_BOOL;
		if (l_type == parser::TYPE::T_BOOL) {
			value.set((cp_bool)((op == "==") ? l_value.b == r_value.b : l_value.b != r_value.b));
		}
		else if (l_type == parser::TYPE::T_STRING) {
			value.set((cp_bool)((op == "==") ? l_value.s == r_value.s : l_value.s != r_value.s));
		}
		else {
			cp_float l = l_value.f, r = r_value.f;

			if (l_type == parser::TYPE::T_INT) {
				l = cp_float(l_value.i);
			}
			if (r_type == parser::TYPE::T_INT) {
				r = cp_float(r_value.i);
			}
			if (op == "==") {
				value.set((cp_bool)(l == r));
			}
			else if (op == "!=") {
				value.set((cp_bool)(l != r));
			}
			else if (op == "<") {
				value.set((cp_bool)(l < r));
			}
			else if (op == ">") {
				value.set((cp_bool)(l > r));
			}
			else if (op == ">=") {
				value.set((cp_bool)(l >= r));
			}
			else if (op == "<=") {
				value.set((cp_bool)(l <= r));
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
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(actualIdentifier); i--);

	// update current expression
	currentExpressionType = scopes[i]->typeof(actualIdentifier);
	currentExpressionValue = *scopes[i]->valueof(actualIdentifier);
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
			currentExpressionValue.set(cp_int(currentExpressionValue.i * -1));
		break;
	case parser::TYPE::T_FLOAT:
		if (un->unaryOp == "-")
			currentExpressionValue.set(cp_float(currentExpressionValue.f * -1));
		break;
	case parser::TYPE::T_BOOL:
		currentExpressionValue.set(cp_bool(currentExpressionValue.b ^ 1));
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
		Value_t* value = new Value_t();
		value->copyFrom(&currentExpressionValue);
		currentFunctionArguments.emplace_back(currentExpressionType, value);
	}

	// update the global vector current_function_arguments
	for (auto arg : currentFunctionArguments) {
		this->currentFunctionArguments.push_back(arg);
	}

	// determine in which scope the function is declared
	size_t i;
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
			currentExpressionValue.set(cp_bool(currentExpressionValue.i != 0));
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue.set(cp_bool(currentExpressionValue.f != .0));
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue.set(cp_bool(currentExpressionValue.c != '\0'));
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue.set(cp_bool(currentExpressionValue.s.empty()));
			break;
		}
		break;

	case parser::TYPE::T_INT:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue.set(cp_int(currentExpressionValue.b));
			break;
		case parser::TYPE::T_INT:
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue.set(cp_int(currentExpressionValue.f));
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue.set(cp_int(currentExpressionValue.c));
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue.set(cp_int(std::stoll(currentExpressionValue.s)));
			break;
		}
		break;

	case parser::TYPE::T_FLOAT:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue.set(cp_float(currentExpressionValue.b));
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue.set(cp_float(currentExpressionValue.i));
			break;
		case parser::TYPE::T_FLOAT:
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue.set(cp_float(currentExpressionValue.c));
			break;
		case parser::TYPE::T_STRING:
			currentExpressionValue.set(cp_float(std::stold(currentExpressionValue.s)));
			break;
		}
		break;

	case parser::TYPE::T_STRING:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue.set(cp_string(currentExpressionValue.b ? "true" : "false"));
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue.set(cp_string(std::to_string(currentExpressionValue.i)));
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue.set(cp_string(std::to_string(currentExpressionValue.f)));
			break;
		case parser::TYPE::T_CHAR:
			currentExpressionValue.set(cp_string(std::to_string(currentExpressionValue.c)));
			break;
		case parser::TYPE::T_STRING:
			break;
		}
		break;

	}

	currentExpressionType = typeParser->type;
}

void visitor::Interpreter::visit(parser::ASTThisNode* thisNode) {
	Value_t value = Value_t();
	value.set(cp_string(scopes.back()->getName()));
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = value;
}

void visitor::Interpreter::visit(parser::ASTExprReadNode* read) {
	std::string line;
	std::getline(std::cin, line);

	currentExpressionValue.set(cp_string(std::move(line)));
}

std::pair<parser::TYPE, Value_t*> Interpreter::currentExpr() {
	return std::move(std::make_pair(currentExpressionType, &currentExpressionValue));
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
