
#ifndef INTERPRETER_SCOPE_H
#define INTERPRETER_SCOPE_H

#include <map>
#include <stack>
#include <any>

#include "visitor.h"
#include "ast.h"


namespace visitor {
	class InterpreterScope {
	private:
		std::string name;
		std::vector<parser::StructureDefinition_t> structureSymbolTable;
		std::map<std::string, Value_t*> variableSymbolTable;
		std::multimap<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>> functionSymbolTable;

	public:
		InterpreterScope();
		InterpreterScope(std::string);

		bool alreadyDeclaredStructureDefinition(std::string);
		bool alreadyDeclaredVariable(std::string);
		bool alreadyDeclaredFunction(std::string, std::vector<parser::TYPE>);
		Value_t* declareNull(std::string, parser::TYPE);
		Value_t* declareNullStruct(std::string, parser::TYPE, std::string);
		Value_t* declare(std::string, cp_bool);
		Value_t* declare(std::string, cp_int);
		Value_t* declare(std::string, cp_float);
		Value_t* declare(std::string, cp_char);
		Value_t* declare(std::string, cp_string);
		Value_t* declare(std::string, cp_struct);
		Value_t* declare(std::string, cp_array);
		void declare(std::string, std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*);
		void declareStructureDefinition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);

		parser::StructureDefinition_t findDeclaredStructureDefinition(std::string);

		std::string typenameof(std::string);
		parser::TYPE typeof(std::string);
		Value_t* valueof(std::string);

		std::vector<std::string> variablenamesof(std::string, std::vector<parser::TYPE>);
		parser::ASTBlockNode* blockof(std::string, std::vector<parser::TYPE>);

		std::vector<std::tuple<std::string, std::string, std::string>>  variableList();

		std::string getName();
	};

	std::string typeStr(parser::TYPE);
}

#endif //INTERPRETER_SCOPE_H
