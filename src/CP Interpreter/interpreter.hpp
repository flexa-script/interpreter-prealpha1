
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <map>
#include <stack>
#include <any>

#include "visitor.hpp"
#include "ast.hpp"
#include "interpreter_scope.hpp"


namespace visitor {

	class Interpreter : public Visitor {
	private:
		std::vector<parser::ASTProgramNode*> programs;
		parser::ASTProgramNode* currentProgram;

		std::vector<InterpreterScope*> scopes;

		Value_t currentExpressionValue;

		std::vector<std::string> currentFunctionParameters;
		std::vector<std::pair<parser::TYPE, Value_t*>> currentFunctionArguments;
		std::string currentFunctionName;
		std::string returnFromFunctionName;
		bool returnFromFunction = false;
		bool isSwitch = false;
		bool isLoop = false;
		bool breakBlock = false;
		bool executedElif = false;

	private:
		std::vector<unsigned int> evaluateAccessVector(std::vector<parser::ASTExprNode*>);
		void declareStructureVariable(std::string, Value_t);
		void declareStructureDefinitionFirstLevelVariables(cp_struct*);
		void printValue(Value_t);
		void printArray(cp_array);
		void printStruct(cp_struct);

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
		void visit(parser::ASTReturnNode*) override;
		void visit(parser::ASTBlockNode*) override;
		void visit(parser::ASTBreakNode*) override;
		void visit(parser::ASTSwitchNode*) override;
		void visit(parser::ASTElseIfNode*) override;
		void visit(parser::ASTIfNode*) override;
		void visit(parser::ASTForNode*) override;
		void visit(parser::ASTForEachNode*) override;
		void visit(parser::ASTWhileNode*) override;
		void visit(parser::ASTFunctionDefinitionNode*) override;
		void visit(parser::ASTStructDefinitionNode*) override;
		void visit(parser::ASTLiteralNode<cp_bool>*) override;
		void visit(parser::ASTLiteralNode<cp_int>*) override;
		void visit(parser::ASTLiteralNode<cp_float>*) override;
		void visit(parser::ASTLiteralNode<cp_char>*) override;
		void visit(parser::ASTLiteralNode<cp_string>*) override;
		void visit(parser::ASTArrayConstructorNode*) override;
		void visit(parser::ASTStructConstructorNode*) override;
		void visit(parser::ASTBinaryExprNode*) override;
		void visit(parser::ASTIdentifierNode*) override;
		void visit(parser::ASTUnaryExprNode*) override;
		void visit(parser::ASTFunctionCallNode*) override;
		void visit(parser::ASTTypeParseNode*) override;
		void visit(parser::ASTTypeNode*) override;
		void visit(parser::ASTLenNode*) override;
		void visit(parser::ASTRoundNode*) override;
		void visit(parser::ASTReadNode*) override;
		void visit(parser::ASTNullNode*) override;
		void visit(parser::ASTThisNode*) override;

		std::pair<parser::TYPE, Value_t*> currentExpr();
		Value_t getCurrentExpressionValue() {
			return currentExpressionValue;
		}
	};
}

#endif //INTERPRETER_H
