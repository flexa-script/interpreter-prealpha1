#ifndef SEMANTIC_SCOPE_H
#define SEMANTIC_SCOPE_H

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "ast.h"


namespace visitor {

	class SemanticScope {
	private:
		std::vector<parser::StructureDefinition_t> structureSymbolTable;
		std::vector<parser::VariableDefinition_t> variableSymbolTable;
		std::vector<parser::FunctionDefinition_t> functionSymbolTable;

	public:
		SemanticScope();

		bool alreadyDeclaredStructureDefinition(std::string);
		bool alreadyDeclaredVariable(std::string);
		bool alreadyDeclaredFunction(std::string, std::vector<parser::TYPE>);
		void declareStructureDefinition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declare(std::string, parser::TYPE, std::string, parser::TYPE, std::vector<int>, bool, bool, unsigned int, unsigned int);
		void declare(std::string, parser::TYPE, std::string, std::vector<parser::TYPE>, bool, unsigned int, unsigned int);
		void changeVarType(std::string, parser::TYPE);
		void changeVarTypeName(std::string, std::string);
		parser::StructureDefinition_t findDeclaredStructureDefinition(std::string);
		bool findAnyVar(std::string);
		bool isConst(std::string);
		parser::TYPE arrayType(std::string);
		parser::VariableDefinition_t var(std::string);
		std::string typeName(std::string);
		parser::TYPE type(std::string);
		std::string typeName(std::string, std::vector<parser::TYPE>);
		parser::TYPE type(std::string, std::vector<parser::TYPE>);
		unsigned int declarationLine(std::string);
		unsigned int declarationLine(std::string, std::vector<parser::TYPE>);
		std::vector<parser::VariableDefinition_t> getVariableSymbolTable();

		std::vector<std::pair<std::string, std::string>> functionList();

	};

	std::string typeStr(parser::TYPE);
}

#endif //SEMANTIC_SCOPE_H
