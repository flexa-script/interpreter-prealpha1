#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include <map>
#include <vector>
#include <stack>

#include "ast.h"


namespace visitor {
	class SemanticScope {
	private:
		std::map<std::string,
			std::pair<parser::TYPE,
			unsigned int>> variableSymbolTable;

		std::multimap<std::string,
			std::tuple<parser::TYPE,
			std::vector<parser::TYPE>,
			unsigned int>> functionSymbolTable;

	public:
		bool alreadyDeclared(std::string);
		bool alreadyDeclared(std::string, std::vector<parser::TYPE>);
		void declare(std::string, parser::TYPE, unsigned int);
		void declare(std::string, parser::TYPE, std::vector<parser::TYPE>, unsigned int);
		parser::TYPE type(std::string);
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
		std::vector<std::pair<std::string, parser::TYPE>> currentFunctionParameters;

	private:
		bool returns(parser::ASTStatementNode*);
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
		void visit(parser::ASTLiteralNode<bool>*) override;
		void visit(parser::ASTLiteralNode<__int64_t>*) override;
		void visit(parser::ASTLiteralNode<long double>*) override;
		void visit(parser::ASTLiteralNode<char>*) override;
		void visit(parser::ASTLiteralNode<std::string>*) override;
		void visit(parser::ASTLiteralNode<std::any>*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTExprFunctionCallNode*) override;
		void visit(parser::ASTFloatParseNode*) override;
		void visit(parser::ASTIntParseNode*) override;
		void visit(parser::ASTStringParseNode*) override;
		void visit(parser::ASTExprReadNode*) override;
	};

	std::string typeStr(parser::TYPE);
}

#endif //SEMANTIC_ANALYSIS_H
