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
		TYPE currentArrayType = TYPE::T_ND;

	public:
		std::string name;

	public:
		explicit Parser(lexer::Lexer*, std::string);
		Parser(lexer::Lexer*, std::string, unsigned int);

		ASTProgramNode* parseProgram();
		ASTExprNode* parseExpression();  // public for repl

	private:
		void consumeToken();

		// statement Nodes
		ASTNode* parseProgramStatement();

		ASTUsingNode* parseUsingStatement();

		ASTNode* parseBlockStatement();

		ASTDeclarationNode* parseDeclarationStatement();

		ASTAssignmentNode* parseAssignmentStatement();

		ASTPrintNode* parsePrintStatement();

		ASTReturnNode* parseReturnStatement();

		ASTBlockNode* parseBlock();

		ASTBlockNode* parseStructBlock();

		ASTStatementNode* parseStructBlockVariables();

		ASTIfNode* parseIfStatement();

		ASTWhileNode* parseWhileStatement();

		ASTFunctionDefinitionNode* parseFunctionDefinition();

		ASTStructDefinitionNode* parseStructDefinition();

		ASTNode* parseIdentifier();

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

		ASTStructConstructorNode* parseStructConstructorNode();

		ASTFunctionCallNode* parseExprFunctionCall();

		ASTReadNode* parseExprRead();

		ASTTypeParseNode* parseExprTypeParse();

		ASTThisNode* parseExprThis();

		ASTTypeNode* parseTypeNode();

		ASTLenNode* parseLenNode();

		ASTRoundNode* parseRoundNode();

		std::vector<ASTExprNode*>* parseActualParams();

		VariableDefinition_t* parseFormalParam();

		ASTIdentifierNode* parseIdentifierNode();

		cp_bool parseBoolLiteral();

		cp_int parseIntLiteral();

		cp_float parseFloatLiteral();

		cp_char parseCharLiteral();

		cp_string parseStringLiteral();

		cp_array parseArrayLiteral();

		// parse types and parameters
		TYPE parseType(std::string);

		std::string msgHeader();
	};
}

#endif //PARSER_H
