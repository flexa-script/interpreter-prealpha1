#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "ast.h"


namespace visitor {

	class SemanticScope {
	private:
		std::vector<parser::StructureDefinition_t> structures;
		std::vector<parser::VariableDefinition_t> variableSymbolTable;
		std::vector<parser::FunctionDefinition_t> functionSymbolTable;
		//std::multimap<std::string, std::tuple<parser::TYPE, std::vector<parser::TYPE>, unsigned int>> functionSymbolTable;

	public:
		bool isAnyVar(std::string);
		bool isConst(std::string);
		bool alreadyDeclaredStructureType(std::string);
		bool alreadyDeclared(std::string);
		bool alreadyDeclared(std::string, std::vector<parser::TYPE>);
		void declareStructureType(std::string, std::vector<parser::VariableDefinition_t>, unsigned int, unsigned int);
		void declare(std::string, parser::TYPE, std::string, parser::TYPE, bool, bool, unsigned int, unsigned int);
		void declare(std::string, parser::TYPE, std::string, std::vector<parser::TYPE>, bool, unsigned int, unsigned int);
		void changeVarType(std::string, parser::TYPE);
		void changeVarTypeName(std::string, std::string);
		parser::StructureDefinition_t findDeclaredStructureType(std::string);
		parser::TYPE arrayType(std::string);
		std::string typeName(std::string);
		parser::TYPE type(std::string);
		std::string typeName(std::string, std::vector<parser::TYPE>);
		parser::TYPE type(std::string, std::vector<parser::TYPE>);
		unsigned int declarationLine(std::string);
		unsigned int declarationLine(std::string, std::vector<parser::TYPE>);

		std::vector<std::pair<std::string, std::string>> functionList();

	};

	class SemanticAnalyser : Visitor {
	private:
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* currentProgram;
		std::vector<SemanticScope*> scopes;
		std::stack<parser::TYPE> functions;
		parser::TYPE currentExpressionType;
		std::string currentExpressionTypeName;
		bool currentExpressionIsArray;
		std::vector<parser::VariableDefinition_t> currentFunctionParameters;

	private:
		bool returns(parser::ASTStatementNode*);
		void declareStructureTypeVariables(std::string, std::string);
		void determineArrayType(cp_array);
		std::string msgHeader(unsigned int, unsigned int);

	public:
		SemanticAnalyser(SemanticScope*, std::vector<parser::ASTProgramNode*>);
		~SemanticAnalyser();

		void start();
		void visit(parser::ASTProgramNode*) override;
		void visit(parser::ASTUsingNode*) override;
		void visit(parser::ASTDeclarationNode*) override;
		void visit(parser::ASTAssignmentNode*) override;
		void visit(parser::ASTPrintNode*) override;
		void visit(parser::ASTReadNode*) override;
		void visit(parser::ASTFunctionCallNode*) override;
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTBlockNode*) override;
		void visit(parser::ASTIfNode*) override;
		void visit(parser::ASTWhileNode*) override;
		void visit(parser::ASTFunctionDefinitionNode*) override;
		void visit(parser::ASTStructDefinitionNode*) override;
		void visit(parser::ASTLiteralNode<cp_bool>*) override;
		void visit(parser::ASTLiteralNode<cp_int>*) override;
		void visit(parser::ASTLiteralNode<cp_float>*) override;
		void visit(parser::ASTLiteralNode<char>*) override;
		void visit(parser::ASTLiteralNode<cp_string>*) override;
		void visit(parser::ASTLiteralNode<cp_any>*) override;
		void visit(parser::ASTLiteralNode<cp_array>*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTExprFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTExprReadNode*) override;
		void visit(parser::ASTThisNode*) override;
	};

	std::string typeStr(parser::TYPE);
}

#endif //SEMANTIC_ANALYSIS_H
