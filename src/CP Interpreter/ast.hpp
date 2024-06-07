#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <array>
#include <vector>
#include <map>

#include "visitor.hpp"


namespace parser {
	class CodePosition {
	public:
		unsigned int row;
		unsigned int col;

		CodePosition(unsigned int row = 0, unsigned int col = 0) : row(row), col(col) {};
	};

	class TypeDefinition {
	public:
		parser::Type type;
		std::string type_name;
		std::string type_name_space;
		parser::Type array_type;
		std::vector<ASTExprNode*> dim;

		TypeDefinition(Type, Type, std::vector<ASTExprNode*>, std::string, std::string);
		TypeDefinition() = default;
	};

	class SemanticValue : public TypeDefinition, public CodePosition {
	public:
		unsigned int hash;
		bool is_const;

		// complete constructor
		SemanticValue(parser::Type, parser::Type, std::vector<ASTExprNode*>,
			std::string, std::string, unsigned int, bool, unsigned int, unsigned int);
		// simplified constructor
		SemanticValue(parser::Type, unsigned int, unsigned int);
		SemanticValue() = default;

		void copy_from(SemanticValue*);
	};

	class SemanticVariable : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		SemanticValue* value;
		bool is_const;

		SemanticVariable(std::string, Type, Type, std::vector<ASTExprNode*>, std::string, std::string,
			SemanticValue*, bool, unsigned int, unsigned int);
		SemanticVariable() = default;

		void copy_from(SemanticVariable*);
	};

	class VariableDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;

		VariableDefinition(std::string, Type, std::string, std::string, Type, Type,
			std::vector<ASTExprNode*>, unsigned int, unsigned int);
		VariableDefinition() = default;
	};

	class FunctionDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		std::vector<parser::TypeDefinition> signature;
		std::vector<parser::VariableDefinition> parameters;

		FunctionDefinition(std::string, Type, std::string, std::string, Type, std::vector<ASTExprNode*>,
			std::vector<parser::TypeDefinition>, std::vector<parser::VariableDefinition>,
			unsigned int, unsigned int);
		FunctionDefinition() = default;
	};

	class StructureDefinition : public CodePosition {
	public:
		std::string identifier;
		std::vector<VariableDefinition> variables;

		StructureDefinition(std::string, std::vector<VariableDefinition>, unsigned int, unsigned int);
		StructureDefinition() = default;
	};

	class Identifier {
	public:
		std::string identifier;
		std::vector<ASTExprNode*> access_vector;

		Identifier(std::string, std::vector<ASTExprNode*>);
		Identifier() = default;
	};

	// Abstract Nodes
	class ASTNode : public CodePosition {
	public:
		ASTNode(unsigned int row, unsigned int col)
			: CodePosition(row, col) {}

		virtual void accept(visitor::Visitor*) = 0;
	};

	class ASTStatementNode : public ASTNode {
	public:
		ASTStatementNode(unsigned int row, unsigned int col)
			: ASTNode(row, col) {}

		void accept(visitor::Visitor*) override = 0;
	};

	class ASTExprNode : public ASTNode {
	public:
		ASTExprNode(unsigned int row, unsigned int col)
			: ASTNode(row, col) {}

		void accept(visitor::Visitor*) override = 0;
		virtual unsigned int hash(visitor::Visitor*) = 0;
	};

	// Statement Nodes
	class ASTProgramNode : public ASTNode {
	public:
		std::string name;
		std::string alias;
		std::vector<std::string> libs;
		std::vector<ASTNode*> statements;

		explicit ASTProgramNode(std::vector<ASTNode*>, std::string, std::string);

		void accept(visitor::Visitor*) override;
	};

	class ASTUsingNode : public ASTStatementNode {
	public:
		std::vector<std::string> library;
		std::string alias;

		ASTUsingNode(std::vector<std::string>, std::string, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTAsNamespaceNode : public ASTStatementNode {
	public:
		std::string nmspace;

		ASTAsNamespaceNode(std::string nmspace, unsigned int col, unsigned int row);

		void accept(visitor::Visitor*) override;
	};

	class ASTDeclarationNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		ASTExprNode* expr;
		bool is_const;

		ASTDeclarationNode(std::string, Type, Type, std::vector<ASTExprNode*>, std::string,
			std::string, ASTExprNode*, bool, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTAssignmentNode : public ASTStatementNode {
	public:
		std::string op;
		std::string nmspace;
		std::vector<Identifier> identifier_vector;
		ASTExprNode* expr;

		ASTAssignmentNode(std::vector<Identifier>, std::string, std::string, ASTExprNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTReturnNode : public ASTStatementNode {
	public:
		ASTExprNode* expr;

		ASTReturnNode(ASTExprNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTBlockNode : public ASTStatementNode {
	public:
		std::vector<ASTNode*> statements;

		ASTBlockNode(std::vector<ASTNode*>, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTContinueNode : public ASTStatementNode {
	public:
		ASTContinueNode(unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTBreakNode : public ASTStatementNode {
	public:
		ASTBreakNode(unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTExitNode : public ASTStatementNode {
	public:
		ASTExprNode* exit_code;

		ASTExitNode(ASTExprNode* exit_code, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTSwitchNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		std::map<ASTExprNode*, unsigned int>* case_blocks;
		std::map<unsigned int, unsigned int>* parsed_case_blocks;
		unsigned int default_block;
		std::vector<ASTNode*>* statements;

		ASTSwitchNode(ASTExprNode*, std::vector<ASTNode*>*, std::map<ASTExprNode*, unsigned int>*, unsigned int, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTElseIfNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* block;

		ASTElseIfNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTIfNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* if_block;
		std::vector<ASTElseIfNode*> else_ifs;
		ASTBlockNode* else_block;

		ASTIfNode(ASTExprNode*, ASTBlockNode*, std::vector<ASTElseIfNode*>, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTEnumNode : public ASTStatementNode {
	public:
		std::vector<std::string> identifiers;

		ASTEnumNode(std::vector<std::string> identifiers, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTTryCatchNode : public ASTStatementNode {
	public:
		ASTStatementNode* decl;
		ASTBlockNode* try_block;
		ASTBlockNode* catch_block;

		ASTTryCatchNode(ASTStatementNode* decl, ASTBlockNode* try_block, ASTBlockNode* catch_block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTThrowNode : public ASTStatementNode {
	public:
		ASTExprNode* error;

		ASTThrowNode(ASTExprNode* error, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTReticencesNode : public ASTStatementNode {
	public:
		ASTReticencesNode(unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTForNode : public ASTStatementNode {
	public:
		std::array<ASTNode*, 3> dci;
		ASTBlockNode* block;

		ASTForNode(std::array<ASTNode*, 3>, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTForEachNode : public ASTStatementNode {
	public:
		ASTDeclarationNode* itdecl; // decl or assign node
		ASTNode* collection;
		ASTBlockNode* block;

		ASTForEachNode(ASTDeclarationNode*, ASTNode*, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTWhileNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* block;

		ASTWhileNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTDoWhileNode : public ASTWhileNode {
	public:
		ASTDoWhileNode(ASTExprNode*, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTFunctionDefinitionNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		std::vector<VariableDefinition> parameters;
		std::vector<std::string> variable_names;
		std::vector<TypeDefinition> signature;
		ASTBlockNode* block;

		ASTFunctionDefinitionNode(std::string, std::vector<VariableDefinition>, Type, std::string,
			std::string, Type, std::vector<ASTExprNode*>, ASTBlockNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	class ASTStructDefinitionNode : public ASTStatementNode {
	public:
		std::string identifier;
		std::vector<VariableDefinition> variables;

		ASTStructDefinitionNode(std::string, std::vector<VariableDefinition>, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
	};

	// Expression Nodes
	template<typename T>
	class ASTLiteralNode : public ASTExprNode {
	public:
		T val;

		ASTLiteralNode(T val, unsigned int row, unsigned int col) : ASTExprNode(col, row), val(val) {};

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTArrayConstructorNode : public ASTExprNode {
	public:
		std::vector<ASTExprNode*> values;

		ASTArrayConstructorNode(std::vector<ASTExprNode*>, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTStructConstructorNode : public ASTExprNode {
	public:
		std::string type_name;
		std::string nmspace;
		std::map<std::string, ASTExprNode*> values;

		ASTStructConstructorNode(std::string, std::string, std::map<std::string, ASTExprNode*>, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTNullNode : public ASTExprNode {
	public:
		ASTNullNode(unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTThisNode : public ASTExprNode {
	public:
		ASTThisNode(unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTBinaryExprNode : public ASTExprNode {
	public:
		std::string op;
		ASTExprNode* left;
		ASTExprNode* right;

		ASTBinaryExprNode(std::string, ASTExprNode*, ASTExprNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTUnaryExprNode : public ASTExprNode {
	public:
		std::string unary_op;
		ASTExprNode* expr;

		ASTUnaryExprNode(std::string, ASTExprNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTIdentifierNode : public ASTExprNode {
	public:
		std::vector<Identifier> identifier_vector;
		std::string nmspace;

		explicit ASTIdentifierNode(std::vector<Identifier>, std::string, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTernaryNode : public ASTExprNode {
	public:
		ASTExprNode* condition;
		ASTExprNode* value_if_true;
		ASTExprNode* value_if_false;

		ASTTernaryNode(ASTExprNode* condition, ASTExprNode* value_if_true, ASTExprNode* value_if_false, unsigned int row, unsigned int col);

		void accept(visitor::Visitor* visitor) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTInNode : public ASTExprNode {
	public:
		ASTExprNode* value;
		ASTExprNode* collection;
		bool vbv;

		ASTInNode(ASTExprNode* value, ASTExprNode* collection, bool vbv, unsigned int row, unsigned int col);

		void accept(visitor::Visitor* visitor) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTFunctionCallNode : public ASTExprNode {
	public:
		std::string identifier;
		std::string nmspace;
		std::vector<ASTExprNode*> access_vector;
		std::vector<std::pair<bool, ASTExprNode*>> parameters;

		ASTFunctionCallNode(std::string, std::string, std::vector<ASTExprNode*>,
			std::vector<std::pair<bool, ASTExprNode*>> parameters, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypeParseNode : public ASTExprNode {
	public:
		Type type;
		ASTExprNode* expr;

		ASTTypeParseNode(Type, ASTExprNode*, unsigned int, unsigned int);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypingNode : public ASTExprNode {
	public:
		std::string image;
		ASTExprNode* expr;

		ASTTypingNode(std::string image, ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

}

#endif // !AST_HPP
