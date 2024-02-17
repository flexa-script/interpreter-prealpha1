#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include <map>
#include <vector>
#include <stack>
#include <xutility>

#include "ast.h"
#include "semantic_scope.h"


namespace visitor {

	class SemanticAnalyser : Visitor {
	private:
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* currentProgram;
		std::vector<SemanticScope*> scopes;
		std::stack<parser::TYPE> functions;
		parser::TYPE currentExpressionType;
		std::string currentExpressionTypeName;
		bool isCurrentExpressionArray;
		bool isFunctionDefinitionContext;
		std::vector<parser::VariableDefinition_t> currentFunctionParameters;

	private:
		bool returns(parser::ASTStatementNode*);
		void declareStructureDefinitionVariables(std::string, std::string, cp_struct, parser::ASTLiteralNode<cp_struct>*);
		void declareStructureDefinitionFirstLevelVariables(std::string, std::string);
		parser::VariableDefinition_t findDeclaredVariable(std::string);
		std::string findTypeName(std::string);
		parser::TYPE findType(std::string);
		bool findAnyVar(std::string);
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
		void visit(parser::ASTLiteralNode<cp_array>*) override;
		void visit(parser::ASTLiteralNode<cp_struct>*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTExprFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTExprReadNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;
	};
}

#endif //SEMANTIC_ANALYSIS_H
