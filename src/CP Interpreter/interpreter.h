
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <map>
#include <stack>

#include "visitor.h"
#include "ast.h"


namespace visitor
{
	typedef struct vT
	{
		vT() : i(0), f(0), b(0), s("") {};
		__int64_t i;
		long double f;
		bool b;
		std::string s;
	} value_t;


	class InterpreterScope
	{
	public:
		InterpreterScope();
		InterpreterScope(std::string);

		bool alreadyDeclared(std::string);
		bool alreadyDeclared(std::string, std::vector<parser::TYPE>);
		void declare(std::string, __int64_t);
		void declare(std::string, long double);
		void declare(std::string, bool);
		void declare(std::string, std::string);
		void declare(std::string, std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*);

		parser::TYPE typeof(std::string);
		value_t valueof(std::string);
		std::vector<std::string> variablenamesof(std::string, std::vector<parser::TYPE>);
		parser::ASTBlockNode* blockof(std::string, std::vector<parser::TYPE>);

		std::vector<std::tuple<std::string, std::string, std::string>>  variableList();

		std::string getName();

	private:
		std::string name;
		std::map<std::string, std::pair<parser::TYPE, value_t>> variableSymbolTable;
		std::multimap<std::string, std::tuple<std::vector<parser::TYPE>, std::vector<std::string>, parser::ASTBlockNode*>> functionSymbolTable;
	};

	class Interpreter : public Visitor
	{
	private:
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* currentProgram;
		std::vector<InterpreterScope*> scopes;
		parser::TYPE currentExpressionType;
		value_t currentExpressionValue;
		std::vector<std::string> currentFunctionParameters;
		std::vector<std::pair<parser::TYPE, value_t>> currentFunctionArguments;
		std::string currentFunctionName;
		std::string returnFromFunctionName;
		bool returnFromFunction = false;

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
		void visit(parser::ASTLiteralNode<__int64_t>*) override;
		void visit(parser::ASTLiteralNode<long double>*) override;
		void visit(parser::ASTLiteralNode<bool>*) override;
		void visit(parser::ASTLiteralNode<std::string>*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTExprFunctionCallNode*) override;
		void visit(parser::ASTFloatParseNode*) override;
		void visit(parser::ASTIntParseNode*) override;
		void visit(parser::ASTStringParseNode*) override;
		void visit(parser::ASTExprReadNode*) override;

		std::pair<parser::TYPE, value_t> currentExpr();
	};

	std::string typeStr(parser::TYPE);
}

#endif //INTERPRETER_H
