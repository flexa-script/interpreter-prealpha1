#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"


namespace parser {

	class Parser {
	private:
		lexer::Lexer* lex;
		lexer::Token currentToken;
		lexer::Token nextToken;

	public:
		std::string name;

	public:
		explicit Parser(lexer::Lexer*, std::string);

		Parser(lexer::Lexer*, std::string, unsigned int);

		ASTProgramNode* parseProgram();

		ASTExprNode* parseExpression();  // public for repl

	private:
		void consumeToken();
		std::string msgHeader();

		// statement Nodes
		ASTStatementNode* parseProgramStatement();

		ASTUsingNode* parseUsingStatement();

		ASTStatementNode* parseBlockStatement();

		ASTDeclarationNode* parseDeclarationStatement();

		ASTAssignmentNode* parseAssignmentStatement();

		ASTPrintNode* parsePrintStatement();

		ASTReadNode* parseReadStatement();

		ASTFunctionCallNode* parseFunctionCall();

		ASTReturnNode* parseReturnStatement();

		ASTBlockNode* parseBlock();

		ASTBlockNode* parseStructBlock();

		ASTStatementNode* parseStructBlockStatement();

		ASTIfNode* parseIfStatement();

		ASTWhileNode* parseWhileStatement();

		ASTFunctionDefinitionNode* parseFunctionDefinition();

		ASTStructDefinitionNode* parseStructDefinition();

		ASTStatementNode* parseIdentifier();

		// expression nodes
		ASTExprNode* parseExpressionTail(ASTExprNode*);

		ASTExprNode* parseLogicalExpression();

		ASTExprNode* parseLogicalExpressionTail(ASTExprNode*);

		ASTExprNode* parseRelationalExpression();

		ASTExprNode* parseRelationalExpressionTail(ASTExprNode*);

		ASTExprNode* parseSimpleExpression();

		ASTExprNode* parseSimpleExpressionTail(ASTExprNode*);

		ASTExprNode* parseTerm();

		ASTExprNode* parseTermTail(ASTExprNode*);

		ASTExprNode* parseFactor();

		ASTExprFunctionCallNode* parseExprFunctionCall();

		ASTExprReadNode* parseExprRead();

		ASTFloatParseNode* parseExprToFloat();

		ASTIntParseNode* parseExprToInt();

		ASTStringParseNode* parseExprToString();

		ASTThisNode* parseExprThis();

		// parse types and parameters
		TYPE parseType(std::string&);

		std::vector<ASTExprNode*>* parseActualParams();

		VariableDefinition_t* parseFormalParam();
	};
}

#endif //PARSER_H
