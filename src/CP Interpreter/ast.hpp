#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <array>
#include <vector>
#include <map>

#include "visitor.hpp"


namespace parser {

	typedef struct VariableDefinition {
		VariableDefinition(std::string, Type, std::string, Type, Type, std::vector<ASTExprNode*>, ASTExprNode*, bool, unsigned int, unsigned int, bool = false);
		VariableDefinition(std::string, Type, std::string, Type, Type, std::vector<ASTExprNode*>, unsigned int, unsigned int);
		VariableDefinition() = default;
		std::string identifier;
		parser::Type type;
		parser::Type any_type;
		parser::Type array_type;
		std::vector<ASTExprNode*> dim;
		std::string type_name;
		ASTExprNode* expr;
		bool is_parameter;
		bool is_const;
		unsigned int row;
		unsigned int col;
	} VariableDefinition_t;

	typedef struct FunctionDefinition {
		FunctionDefinition(std::string, Type, std::string, Type, Type, std::vector<ASTExprNode*>, std::vector<parser::Type>,
			std::vector<parser::VariableDefinition_t>,unsigned int, unsigned int);
		FunctionDefinition() = default;
		std::string identifier;
		parser::Type type;
		parser::Type any_type;
		parser::Type array_type;
		std::vector<ASTExprNode*> dim;
		std::string type_name;
		std::vector<parser::Type> signature;
		std::vector<parser::VariableDefinition_t> parameters;
		unsigned int row;
		unsigned int col;
	} FunctionDefinition_t;

	typedef struct StructureDefinition {
		StructureDefinition(std::string, std::vector<VariableDefinition_t>, unsigned int, unsigned int);
		StructureDefinition() = default;
		std::string identifier;
		std::vector<VariableDefinition_t> variables;
		unsigned int row;
		unsigned int col;
	} StructureDefinition_t;

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
		virtual unsigned int hash(visitor::Visitor*) = 0;
	};

	// Statement Nodes
	class ASTProgramNode : public ASTNode {
	public:
		std::string name;
		std::string alias;

		explicit ASTProgramNode(std::vector<ASTNode*>, std::string);

		std::vector<ASTNode*> statements;

		void accept(visitor::Visitor*) override;
	};

	class ASTUsingNode : public ASTStatementNode {
	public:
		ASTUsingNode(std::string, std::string, unsigned int, unsigned int);

		std::string library;
		std::string alias;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTDeclarationNode : public ASTStatementNode {
	public:
		ASTDeclarationNode(Type, std::string, std::string, ASTExprNode*, bool, Type, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		Type type;
		std::string identifier;
		std::string type_name;
		ASTExprNode* expr;
		bool is_const;
		Type array_type;
		std::vector<ASTExprNode*> dim;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTAssignmentNode : public ASTStatementNode {
	public:
		ASTAssignmentNode(std::vector<std::string>, std::string, ASTExprNode*, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::string op;
		std::vector<std::string> identifier_vector;
		ASTExprNode* expr;
		std::vector<ASTExprNode*> access_vector;
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
		ASTBlockNode(std::vector<ASTNode*>, unsigned int, unsigned int);

		std::vector<ASTNode*> statements;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTContinueNode : public ASTStatementNode {
	public:
		ASTContinueNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTBreakNode : public ASTStatementNode {
	public:
		ASTBreakNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	// at hashtable it's stored the correspondent case statement start
	// it'll execute until find a break
	// at vector it's stored switch statements
	class ASTSwitchNode : public ASTStatementNode {
	public:
		ASTSwitchNode(ASTExprNode*, std::vector<ASTNode*>*, std::map<ASTExprNode*, unsigned int>*, unsigned int, unsigned int, unsigned int);

		ASTExprNode* condition;
		std::map<ASTExprNode*, unsigned int>* case_blocks;
		std::map<unsigned int, unsigned int>* parsed_case_blocks;
		unsigned int default_block;
		std::vector<ASTNode*>* statements;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTElseIfNode : public ASTStatementNode {
	public:
		ASTElseIfNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int);

		ASTExprNode* condition;
		ASTBlockNode* block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTIfNode : public ASTStatementNode {
	public:
		ASTIfNode(ASTExprNode*, ASTBlockNode*, std::vector<ASTElseIfNode*>, ASTBlockNode*, unsigned int, unsigned int);

		ASTExprNode* condition;
		ASTBlockNode* if_block;
		std::vector<ASTElseIfNode*> else_ifs;
		ASTBlockNode* else_block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTForNode : public ASTStatementNode {
	public:
		ASTForNode(std::array<ASTNode*, 3>, ASTBlockNode*, unsigned int, unsigned int);

		std::array<ASTNode*, 3> dci;
		ASTBlockNode* block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTForEachNode : public ASTStatementNode {
	public:
		ASTForEachNode(ASTNode*, ASTNode*, ASTBlockNode*, unsigned int, unsigned int);

		ASTNode* itdecl; // decl or assign node
		ASTNode* collection;
		ASTBlockNode* block;
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
		ASTFunctionDefinitionNode(std::string, std::vector<VariableDefinition_t>, Type, std::string, Type, std::vector<ASTExprNode*>, ASTBlockNode*, unsigned int, unsigned int);

		std::string identifier;
		std::vector<VariableDefinition_t> parameters;
		std::vector<std::string> variable_names;
		std::vector<Type> signature;
		Type type;
		Type array_type;
		std::vector<ASTExprNode*> dim;
		std::string type_name;
		ASTBlockNode* block;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
	};

	class ASTStructDefinitionNode : public ASTStatementNode {
	public:
		ASTStructDefinitionNode(std::string, std::vector<VariableDefinition_t>, unsigned int, unsigned int);

		std::string identifier;
		std::vector<VariableDefinition_t> variables;
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
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTArrayConstructorNode : public ASTExprNode {
	public:
		ASTArrayConstructorNode(std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::vector<ASTExprNode*> values;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTStructConstructorNode : public ASTExprNode {
	public:
		ASTStructConstructorNode(std::string, std::map<std::string, ASTExprNode*>, unsigned int, unsigned int);

		std::string type_name;
		std::map<std::string, ASTExprNode*> values;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTNullNode : public ASTExprNode {
	public:
		ASTNullNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTThisNode : public ASTExprNode {
	public:
		ASTThisNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
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
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTIdentifierNode : public ASTExprNode {
	public:
		explicit ASTIdentifierNode(std::vector<std::string>, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::vector<std::string> identifier_vector;
		std::vector<ASTExprNode*> access_vector;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTUnaryExprNode : public ASTExprNode {
	public:
		ASTUnaryExprNode(std::string, ASTExprNode*, unsigned int, unsigned int);

		std::string unary_op;
		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTFunctionCallNode : public ASTExprNode {
	public:
		ASTFunctionCallNode(std::string, std::vector<ASTExprNode*>, unsigned int, unsigned int);

		std::string identifier;
		std::vector<ASTExprNode*> parameters;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypeParseNode : public ASTExprNode {
	public:
		ASTTypeParseNode(Type, ASTExprNode*, unsigned int, unsigned int);

		Type type;
		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTRoundNode : public ASTExprNode {
	public:
		ASTRoundNode(ASTExprNode*, unsigned int, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int ndigits;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTLenNode : public ASTExprNode {
	public:
		ASTLenNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypeNode : public ASTExprNode {
	public:
		ASTTypeNode(ASTExprNode*, unsigned int, unsigned int);

		ASTExprNode* expr;
		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTReadNode : public ASTExprNode {
	public:
		ASTReadNode(unsigned int, unsigned int);

		unsigned int row;
		unsigned int col;

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};
}

#endif // AST_HPP
