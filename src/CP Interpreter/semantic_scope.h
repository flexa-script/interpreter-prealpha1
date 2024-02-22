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
		std::map<std::string, parser::StructureDefinition_t> structureSymbolTable;
		std::map<std::string, parser::VariableDefinition_t> variableSymbolTable;
		std::multimap<std::string, parser::FunctionDefinition_t> functionSymbolTable;

	public:
		SemanticScope();

		bool alreadyDeclaredStructureDefinition(std::string);
		bool alreadyDeclaredVariable(std::string);
		bool alreadyDeclaredFunction(std::string, std::vector<parser::TYPE>);

		void declareStructureDefinition(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declareVariable(std::string, parser::TYPE, std::string, parser::TYPE, std::vector<int>, bool, bool, bool, unsigned int, unsigned int, bool);
		void declareFunction(std::string, parser::TYPE, std::string, std::vector<parser::TYPE>, bool, unsigned int, unsigned int);

		void assignVariable(std::string, bool);
		void changeVariableType(std::string, parser::TYPE);
		void changeVariableTypeName(std::string, std::string);

		parser::StructureDefinition_t findDeclaredStructureDefinition(std::string);
		parser::VariableDefinition_t findDeclaredVariable(std::string);
		parser::FunctionDefinition_t findDeclaredFunction(std::string, std::vector<parser::TYPE>);
	};
}

#endif //SEMANTIC_SCOPE_H
