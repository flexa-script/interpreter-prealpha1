
#ifndef INTERPRETER_SCOPE_H
#define INTERPRETER_SCOPE_H

#include <map>
#include <stack>
#include <any>

#include "visitor.h"
#include "ast.h"


namespace visitor {
	class InterpreterScope {
	public:
		bool hasStringAccess = false;

	private:
		std::string name;
		std::map<std::string, parser::StructureDefinition_t> structureSymbolTable;
		std::map<std::string, Value_t*> variableSymbolTable;
		std::multimap<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>> functionSymbolTable;

	public:
		InterpreterScope();
		InterpreterScope(std::string);

		bool alreadyDeclaredStructureDefinition(std::string);
		bool alreadyDeclaredVariable(std::string);
		bool alreadyDeclaredFunction(std::string, std::vector<parser::TYPE>);

		Value_t* declareNullVariable(std::string, parser::TYPE);
		Value_t* declareNullStructVariable(std::string, std::string);
		Value_t* declareVariable(std::string, cp_bool);
		Value_t* declareVariable(std::string, cp_int);
		Value_t* declareVariable(std::string, cp_float);
		Value_t* declareVariable(std::string, cp_char);
		Value_t* declareVariable(std::string, cp_string);
		Value_t* declareVariable(std::string, cp_struct);
		Value_t* declareVariable(std::string, cp_array);

		void declareFunction(std::string, std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*);
		void declareStructureDefinition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);

		parser::StructureDefinition_t findDeclaredStructureDefinition(std::string);
		Value_t* accessValue(std::vector<std::string>, std::vector<unsigned int>);
		Value_t* accessValueOfArray(Value_t*, std::vector<unsigned int>);
		Value_t* accessValueOfStructure(Value_t*, std::vector<std::string>);

		std::vector<std::string> variablenamesof(std::string, std::vector<parser::TYPE>);
		parser::ASTBlockNode* blockof(std::string, std::vector<parser::TYPE>);

		std::string getName();
	};
}

#endif //INTERPRETER_SCOPE_H
