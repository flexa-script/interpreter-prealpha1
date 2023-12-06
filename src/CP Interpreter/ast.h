#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

#include "visitor.h"


namespace parser {
	// Types
	enum class TYPE {
		T_ND, T_VOID, T_NULL, T_ANY, T_BOOL, T_INT, T_FLOAT, T_CHAR, T_STRING
	};

	// Abstract Nodes
	class ASTNode {
	public:
		virtual void accept(visitor::Visitor*) = 0;
	};

	class ASTStatementNode : public ASTNode {
	public:
		void accept(visitor::Visitor*) override = 0;
	};

	class ASTExprNode : public ASTNode {
	public:
		void accept(visitor::Visitor*) override = 0;
	};

	// Statement Nodes
	class ASTProgramNode : public ASTNode {
	public:
		std::string name;

		explicit ASTProgramNode(std::vector<ASTNode*>, std::string);

		std::vector<ASTNode*> statements;

		void accept(visitor::Visitor*) override;
	};

	class ASTUsingNode : public ASTStatementNode {
	public:
		ASTUsingNode(std::string, unsigned int, unsigned int);

		std::string library;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTDeclarationNode : public ASTStatementNode {
	public:
		ASTDeclarationNode(TYPE, std::string, ASTExprNode*, unsigned int, unsigned int);

		TYPE type;
		std::string identifier;
		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTAssignmentNode : public ASTStatementNode {
	public:
		ASTAssignmentNode(std::string, ASTExprNode*, unsigned int, unsigned int);

		std::string identifier;
		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTPrintNode : public ASTStatementNode {
	public:
		ASTPrintNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTReadNode : public ASTStatementNode {
	public:
		ASTReadNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTFunctionCallNode : public ASTStatementNode {
	public:
		ASTFunctionCallNode(std::string, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::string identifier;
		std::vector<ASTExprNode*> parameters;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTReturnNode : public ASTStatementNode {
	public:
		ASTReturnNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTBlockNode : public ASTStatementNode {
	public:
		ASTBlockNode(std::vector<ASTStatementNode*>, unsigned int, unsigned int);

		std::vector<ASTStatementNode*> statements;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTIfNode : public ASTStatementNode {
	public:
		ASTIfNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int, ASTBlockNode* = nullptr);

		ASTExprNode* condition;
		ASTBlockNode* ifBlock;
		ASTBlockNode* elseBlock;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTWhileNode : public ASTStatementNode {
	public:
		ASTWhileNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int);

		ASTExprNode* condition;
		ASTBlockNode* block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTFunctionDefinitionNode : public ASTStatementNode {
	public:
		ASTFunctionDefinitionNode(std::string, std::vector<std::pair<std::string, TYPE>>,
			TYPE, ASTBlockNode*, unsigned int, unsigned int);

		std::string identifier;
		std::vector<std::pair<std::string, TYPE>> parameters;
		std::vector<std::string> variableNames;
		std::vector<TYPE> signature;
		TYPE type;
		ASTBlockNode* block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	// Expression Nodes
	template<typename T>
	class ASTLiteralNode : public ASTExprNode {
	public:
		ASTLiteralNode(T val, unsigned int row, unsigned int col) : val(val), row(row), col(col) {};
		T val;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTBinaryExprNode : public ASTExprNode {
	public:
		ASTBinaryExprNode(std::string, ASTExprNode*, ASTExprNode*, unsigned int, unsigned int);

		std::string op;
		ASTExprNode* left;
		ASTExprNode* right;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTIdentifierNode : public ASTExprNode {
	public:
		explicit ASTIdentifierNode(std::string, unsigned int, unsigned int);

		std::string identifier;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTUnaryExprNode : public ASTExprNode {
	public:
		ASTUnaryExprNode(std::string, ASTExprNode*, unsigned int, unsigned int);

		std::string unaryOp;
		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTExprFunctionCallNode : public ASTExprNode {
	public:
		ASTExprFunctionCallNode(std::string, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::string identifier;
		std::vector<ASTExprNode*> parameters;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTFloatParseNode : public ASTExprNode {
	public:
		ASTFloatParseNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTIntParseNode : public ASTExprNode {
	public:
		ASTIntParseNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTStringParseNode : public ASTExprNode {
	public:
		ASTStringParseNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTExprReadNode : public ASTExprNode {
	public:
		ASTExprReadNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};
}

#endif //AST_H
