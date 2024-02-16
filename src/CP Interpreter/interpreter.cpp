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

	throw std::runtime_error("IERR: can't found '" + identifier + "'");
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	if (alreadyDeclared(identifier)) {
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
	structures.push_back(type);
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


Interpreter::Interpreter()
	: currentExpressionValue(Value_t(parser::TYPE::T_ND)) {
	// add global scope
	scopes.push_back(new InterpreterScope());
	currentExpressionType = parser::TYPE::T_ND;
	currentProgram = nullptr;
}

Interpreter::Interpreter(InterpreterScope* globalScope, std::vector<parser::ASTProgramNode*> programs)
	: programs(programs), currentProgram(programs[0]), currentExpressionValue(Value_t(parser::TYPE::T_ND)) {
	// add global scope
	scopes.push_back(globalScope);
	currentExpressionType = parser::TYPE::T_ND;
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

	if (decl->expr && currentExpressionType != parser::TYPE::T_NULL && currentExpressionValue.hasValue) {
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
			case parser::TYPE::T_STRUCT:
				auto identifierVector = std::vector<std::string>();
				identifierVector.push_back(decl->identifier);
				if (currentExpressionValue.actualType == parser::TYPE::T_NULL) {
					currentExpressionValue.setType(decl->type);
					currentExpressionValue.str.first = decl->typeName;
				}
				declareStructureVariable(identifierVector, currentExpressionValue);
				break;
			}
			break;
		case parser::TYPE::T_ARRAY:
			scopes.back()->declare(decl->identifier, currentExpressionValue.arr);
			break;
		case parser::TYPE::T_STRUCT:
			auto identifierVector = std::vector<std::string>();
			identifierVector.push_back(decl->identifier);
			if (currentExpressionValue.actualType == parser::TYPE::T_NULL) {
				currentExpressionValue.setType(decl->type);
				currentExpressionValue.str.first = decl->typeName;
			}
			declareStructureVariable(identifierVector, currentExpressionValue);
			break;
		}
	}
	else {
		if (decl->type == parser::TYPE::T_STRUCT) {
			auto identifierVector = std::vector<std::string>();
			identifierVector.push_back(decl->identifier);
			auto nllVal = Value_t(parser::TYPE::T_STRUCT);
			nllVal.setNull();
			nllVal.str.first = decl->typeName;
			declareStructureVariable(identifierVector, nllVal);
		}
		else {
			scopes.back()->declareNull(decl->identifier, decl->type);
		}
	}
}

void Interpreter::declareStructureVariable(std::vector<std::string> identifierVector, Value_t newValue) {
	Value_t* value = nullptr;
	std::string typeName;
	parser::StructureDefinition_t typeStruct;
	int declScopeIdx = 0;
	int strDefScopeIdx = 0;

	if (identifierVector.size() == 1) {
		for (declScopeIdx = scopes.size() - 1; !scopes[declScopeIdx]->alreadyDeclared(identifierVector[0]); --declScopeIdx) {
			if (declScopeIdx <= 0) {
				declScopeIdx = -1;
				break;
			}
		}
		if (declScopeIdx >= 0) {
			if (!newValue.hasValue || newValue.actualType == parser::TYPE::T_NULL) {
				value = scopes[declScopeIdx]->valueof(identifierVector[0]);
				scopes[declScopeIdx]->declareNullStruct(identifierVector[0], value->actualType, value->str.first);
			}
			else {
				value = scopes[declScopeIdx]->declare(identifierVector[0], newValue.str);
			}
		}
		else {
			value = scopes.back()->declare(identifierVector[0], newValue.str);
			for (declScopeIdx = scopes.size() - 1; !scopes[declScopeIdx]->alreadyDeclared(identifierVector[0]); --declScopeIdx);
		}

		typeName = scopes[declScopeIdx]->typenameof(identifierVector[0]);
		typeStruct = scopes[strDefScopeIdx]->findDeclaredStructureDefinition(typeName);

		for (size_t j = 0; j < typeStruct.variables.size(); ++j) {
			bool found = false;
			for (size_t k = 0; k < value->str.second.size(); ++k) {
				if (value->str.second[k].first == typeStruct.variables[j].identifier) {
					found = true;
					break;
				}
			}
			if (!found) {
				cp_struct_value strVal;
				strVal.first = typeStruct.variables[j].identifier;
				strVal.second = new Value_t(typeStruct.variables[j].type);
				strVal.second->setNull();
				value->str.second.push_back(strVal);
			}
		}
	}
	else {
		Value_t* currValue = nullptr;

		for (declScopeIdx = scopes.size() - 1; !scopes[declScopeIdx]->alreadyDeclared(identifierVector[0]); --declScopeIdx) {
			if (declScopeIdx <= 0) {
				declScopeIdx = -1;
				break;
			}
		}
		if (declScopeIdx >= 0) {
			value = scopes[declScopeIdx]->valueof(identifierVector[0]);
		}

		typeName = scopes[declScopeIdx]->typenameof(identifierVector[0]);
		for (strDefScopeIdx = scopes.size() - 1; strDefScopeIdx >= 0 && !scopes[strDefScopeIdx]->alreadyDeclaredStructureType(typeName); --strDefScopeIdx);

		typeStruct = scopes[strDefScopeIdx]->findDeclaredStructureDefinition(typeName);
		currValue = value;
		if (!currValue) throw std::runtime_error("error");

		for (size_t i = 1; i < identifierVector.size(); ++i) {
			for (size_t j = 0; j < typeStruct.variables.size(); ++j) {
				if (identifierVector[i] == typeStruct.variables[j].identifier) {
					bool found = false;
					for (size_t k = 0; k < currValue->str.second.size(); ++k) {
						if (currValue->str.second[k].first == typeStruct.variables[j].identifier) {
							found = true;
							currValue = currValue->str.second[k].second;
							break;
						}
					}
					if (typeStruct.variables[j].type == parser::TYPE::T_STRUCT) {
						for (strDefScopeIdx = scopes.size() - 1; strDefScopeIdx >= 0 && !scopes[strDefScopeIdx]->alreadyDeclaredStructureType(typeStruct.variables[j].typeName); --strDefScopeIdx);
						typeStruct = scopes[strDefScopeIdx]->findDeclaredStructureDefinition(typeStruct.variables[j].typeName);
					}

					break;
				}
			}
		}

		if (!currValue) throw std::runtime_error("error");

		if (newValue.hasValue) {
			switch (currValue->actualType) {
			case parser::TYPE::T_BOOL:
				currValue->set(newValue.b);
				break;
			case parser::TYPE::T_INT:
				currValue->set(newValue.i);
				break;
			case parser::TYPE::T_FLOAT:
				currValue->set(newValue.f);
				break;
			case parser::TYPE::T_CHAR:
				currValue->set(newValue.c);
				break;
			case parser::TYPE::T_STRING:
				currValue->set(newValue.s);
				break;
			case parser::TYPE::T_ANY:
				switch (newValue.actualType) {
				case parser::TYPE::T_BOOL:
					currValue->set(newValue.b);
					break;
				case parser::TYPE::T_INT:
					currValue->set(newValue.i);
					break;
				case parser::TYPE::T_FLOAT:
					currValue->set(newValue.f);
					break;
				case parser::TYPE::T_CHAR:
					currValue->set(newValue.c);
					break;
				case parser::TYPE::T_STRING:
					currValue->set(newValue.s);
					break;
				case parser::TYPE::T_STRUCT:
					currValue->set(newValue.str);
					break;
				case parser::TYPE::T_ARRAY:
					currValue->set(newValue.arr);
					break;
				}
				break;
			case parser::TYPE::T_STRUCT:
				currValue->set(newValue.str);
				break;
			case parser::TYPE::T_ARRAY:
				currValue->set(newValue.arr);
				break;
			}
		}
		else {
			currValue->setNull();
		}

	}

}

void visitor::Interpreter::visit(parser::ASTAssignmentNode* assign) {
	// determine innermost scope in which variable is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(assign->identifier); i--);

	// visit expression node to update current value/type
	assign->expr->accept(this);

	auto type = scopes[i]->typeof(assign->identifier);
	if (type != parser::TYPE::T_ARRAY && type != parser::TYPE::T_STRUCT) {
		type = currentExpressionType;
	}

	if (currentExpressionType != parser::TYPE::T_NULL && currentExpressionValue.hasValue) {
		// redeclare variable, depending on type
		switch (type) {
		case parser::TYPE::T_BOOL:
			scopes[i]->declare(assign->identifier, currentExpressionValue.b);
			break;
		case parser::TYPE::T_INT:
			scopes[i]->declare(assign->identifier, currentExpressionValue.i);
			break;
		case parser::TYPE::T_FLOAT:
			scopes[i]->declare(assign->identifier, currentExpressionValue.f);
			break;
		case parser::TYPE::T_CHAR:
			scopes[i]->declare(assign->identifier, currentExpressionValue.c);
			break;
		case parser::TYPE::T_STRING:
			scopes[i]->declare(assign->identifier, currentExpressionValue.s);
			break;
		case parser::TYPE::T_STRUCT: {
			if (assign->identifierVector.size() == 1) {
				scopes[i]->declare(assign->identifier, currentExpressionValue.str);
			}
			else {
				declareStructureVariable(assign->identifierVector, currentExpressionValue);
			}
			break;
		}
		case parser::TYPE::T_ARRAY:
			Value_t* val = scopes[i]->valueof(assign->identifier);
			cp_array* currentVal = &val->arr;
			size_t s = 0;
			for (s = 0; s < assign->accessVector.size() - 1; ++s) {
				currentVal = &currentVal->at(assign->accessVector[s])->arr;
			}

			Value_t* newVal = new Value_t(currentExpressionType);
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
	else {
		if (type == parser::TYPE::T_STRUCT) {
			declareStructureVariable(assign->identifierVector, currentExpressionValue);
		}
		else {
			scopes.back()->declareNull(assign->identifier, type);
		}
	}
}

void printValue(Value_t* value) {
	if (!value->hasValue) {
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
		throw std::runtime_error("IERR: can't determine value type on printing");
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
	case parser::TYPE::T_STRUCT:
		// TODO: print struct function
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
		Value_t* value = new Value_t(currentExpressionType);
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
	for (int i = scopes.size() - 1; i >= 0; --i) {
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
		case parser::TYPE::T_STRUCT:
			scopes.back()->declare(currentFunctionParameters[i], currentFunctionArguments[i].second->str);
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

	bool result = currentExpressionType == parser::TYPE::T_BOOL ? currentExpressionValue.b : currentExpressionValue.hasValue;

	// execute appropriate blocks
	if (result) {
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

	bool result = currentExpressionType == parser::TYPE::T_BOOL ? currentExpressionValue.b : currentExpressionValue.hasValue;

	while (result) {
		// execute block
		whileNode->block->accept(this);

		// re-evaluate while condition
		whileNode->condition->accept(this);

		result = currentExpressionType == parser::TYPE::T_BOOL ? currentExpressionValue.b : currentExpressionValue.hasValue;
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
	Value_t* value = new Value_t(parser::TYPE::T_BOOL);
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_BOOL;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_int>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_INT);
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_INT;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_float>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_FLOAT);
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_FLOAT;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_char>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_CHAR);
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_CHAR;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_string>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_STRING);
	value->set(lit->val);
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_array>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_ARRAY);
	value->set(lit->val);
	determineArrayType(lit->val);
	currentExpressionValue = *value;
}

void visitor::Interpreter::visit(parser::ASTLiteralNode<cp_struct>* lit) {
	Value_t* value = new Value_t(parser::TYPE::T_STRUCT);
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
	Value_t value = Value_t(parser::TYPE::T_ND);

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
					throw std::runtime_error(msgHeader(bin->row, bin->col) + "division by zero encountered");
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

		cp_bool l = l_value.b;
		cp_bool r = r_value.b;

		if (l_type == parser::TYPE::T_STRUCT) {
			l = l_value.hasValue;
		}
		if (r_type == parser::TYPE::T_STRUCT) {
			r = r_value.hasValue;
		}

		if (op == "and") {
			value.set((cp_bool)(l && r));
		}
		else if (op == "or") {
			value.set((cp_bool)(l || r));
		}
	}
	else { // now comparator operators
		currentExpressionType = parser::TYPE::T_BOOL;

		if (l_type == parser::TYPE::T_BOOL || l_type == parser::TYPE::T_STRUCT) {

			cp_bool l = l_value.b;
			cp_bool r = r_value.b;

			if (l_type == parser::TYPE::T_STRUCT) {
				l = l_value.hasValue;
			}
			if (r_type == parser::TYPE::T_STRUCT) {
				r = r_value.hasValue;
			}

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
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(id->identifier); i--);

	currentExpressionType = scopes[i]->typeof(actualIdentifier);
	currentExpressionValue = *scopes[i]->valueof(actualIdentifier);
	currentExpressionTypeName = currentExpressionType == parser::TYPE::T_STRUCT ? scopes[i]->typenameof(actualIdentifier) : "";

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
		break;
	case parser::TYPE::T_STRUCT:
		if (un->unaryOp == "not") {
			currentExpressionType = parser::TYPE::T_BOOL;
			currentExpressionValue.set(cp_bool(!currentExpressionValue.hasValue));
		}
		break;
	}
}

void visitor::Interpreter::visit(parser::ASTExprFunctionCallNode* astnode) {
	// determine the signature of the function
	std::vector<parser::TYPE> signature;
	std::vector<std::pair<parser::TYPE, Value_t*>> currentFunctionArguments;

	// for each parameter
	for (auto param : astnode->parameters) {
		// visit to update current expr type
		param->accept(this);

		// add the type of current expr to signature
		signature.push_back(currentExpressionType);

		// add the current expr to the local vector of function arguments, to be
		// used in the creation of the function scope
		Value_t* value = new Value_t(currentExpressionType);
		value->copyFrom(&currentExpressionValue);
		currentFunctionArguments.emplace_back(currentExpressionType, value);
	}

	// update the global vector current_function_arguments
	for (auto arg : currentFunctionArguments) {
		this->currentFunctionArguments.push_back(arg);
	}

	// determine in which scope the function is declared
	size_t i;
	for (i = scopes.size() - 1; !scopes[i]->alreadyDeclared(astnode->identifier, signature); i--);

	// populate the global vector of function parameter names, to be used in creation of function scope
	currentFunctionParameters = scopes[i]->variablenamesof(astnode->identifier, signature);

	currentFunctionName = astnode->identifier;

	// visit the corresponding function block
	scopes[i]->blockof(astnode->identifier, signature)->accept(this);
}

void visitor::Interpreter::visit(parser::ASTTypeParseNode* astnode) {
	// visit expression node to update current value/type
	astnode->expr->accept(this);

	switch (astnode->type) {
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
			currentExpressionValue.set(cp_bool(!currentExpressionValue.s.empty()));
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
			try {
				currentExpressionValue.set(cp_int(std::stoll(currentExpressionValue.s)));
			}
			catch (...) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + currentExpressionValue.s + "' is not a valid value to parse int");
			}
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
			try {
				currentExpressionValue.set(cp_float(std::stold(currentExpressionValue.s)));
			}
			catch (...) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + currentExpressionValue.s + "' is not a valid value to parse float");
			}
			break;
		}
		break;

	case parser::TYPE::T_CHAR:
		switch (currentExpressionType) {
		case parser::TYPE::T_BOOL:
			currentExpressionValue.set(cp_char(currentExpressionValue.b));
			break;
		case parser::TYPE::T_INT:
			currentExpressionValue.set(cp_char(currentExpressionValue.i));
			break;
		case parser::TYPE::T_FLOAT:
			currentExpressionValue.set(cp_char(currentExpressionValue.f));
			break;
		case parser::TYPE::T_CHAR:
			break;
		case parser::TYPE::T_STRING:
			if (currentExpressionValue.s.size() > 1) {
				throw std::runtime_error(msgHeader(astnode->row, astnode->col) + "'" + currentExpressionValue.s + "' is not a valid value to parse char");
			}
			else {
				currentExpressionValue.set(cp_char(currentExpressionValue.s[0]));
			}
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

	currentExpressionType = astnode->type;
}

void visitor::Interpreter::visit(parser::ASTNullNode* astnode) {
	Value_t value = Value_t(parser::TYPE::T_NULL);
	value.setNull();
	currentExpressionType = parser::TYPE::T_NULL;
	currentExpressionValue = value;
}

void visitor::Interpreter::visit(parser::ASTThisNode* astnode) {
	Value_t value = Value_t(parser::TYPE::T_STRING);
	value.set(cp_string(scopes.back()->getName()));
	currentExpressionType = parser::TYPE::T_STRING;
	currentExpressionValue = value;
}

void visitor::Interpreter::visit(parser::ASTExprReadNode* astnode) {
	std::string line;
	std::getline(std::cin, line);

	currentExpressionValue.set(cp_string(std::move(line)));
}

std::pair<parser::TYPE, Value_t*> Interpreter::currentExpr() {
	return std::move(std::make_pair(currentExpressionType, &currentExpressionValue));
};

std::string Interpreter::msgHeader(unsigned int row, unsigned int col) {
	return "(IERR) " + currentProgram->name + '[' + std::to_string(row) + ':' + std::to_string(col) + "]: ";
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
		throw std::runtime_error("IERR: invalid type encountered.");
	}
}
