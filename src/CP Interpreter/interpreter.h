
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <map>
#include <stack>
#include <any>

#include "visitor.h"
#include "ast.h"
#include "interpreter_scope.h"


namespace visitor {

	class Interpreter : public Visitor {
	private:
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* currentProgram;
		std::vector<InterpreterScope*> scopes;
		parser::TYPE currentExpressionType;
		std::string currentExpressionTypeName;
		Value_t currentExpressionValue;
		std::vector<std::string> currentFunctionParameters;
		std::vector<std::pair<parser::TYPE, Value_t*>> currentFunctionArguments;
		std::string currentFunctionName;
		std::string returnFromFunctionName;
		bool returnFromFunction = false;

	private:
		void determineArrayType(cp_array);
		void declareStructureVariable(std::vector<std::string>, Value_t, std::vector<unsigned int>);

		std::string msgHeader(unsigned int, unsigned int);

	public:
		Interpreter();
		Interpreter(InterpreterScope*, std::vector<parser::ASTProgramNode*>);
		~Interpreter();

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
		void visit(parser::ASTLiteralNode<cp_char>*) override;
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

		std::pair<parser::TYPE, Value_t*> currentExpr();
	};

	std::string typeStr(parser::TYPE);
}

#endif //INTERPRETER_H
