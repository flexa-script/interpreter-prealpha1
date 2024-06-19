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

		TypeDefinition(Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space);

		TypeDefinition();

		static TypeDefinition get_basic(parser::Type type);
		static TypeDefinition get_array(Type type, Type array_type, std::vector<ASTExprNode*>&& dim = std::vector<ASTExprNode*>());
		static TypeDefinition get_struct(Type type, const std::string& type_name, const std::string& type_name_space);
	};

	class SemanticValue : public TypeDefinition, public CodePosition {
	public:
		unsigned int hash;
		bool is_const;

		// complete constructor
		SemanticValue(parser::Type type, parser::Type array_type, std::vector<ASTExprNode*>&& dim,
			const std::string& type_name, const std::string& type_name_space, unsigned int hash,
			bool is_const, unsigned int row, unsigned int col);

		// simplified constructor
		SemanticValue(Type type, unsigned int row, unsigned int col);

		SemanticValue();

		void copy_from(SemanticValue* value);
	};

	class SemanticVariable : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		SemanticValue* value;
		bool is_const;

		SemanticVariable(const std::string& identifier, Type type, Type array_type, const std::vector<ASTExprNode*>& dim,
			const std::string& type_name, const std::string& type_name_space, SemanticValue* value, bool is_const, unsigned int row, unsigned int col);

		SemanticVariable();

		void copy_from(SemanticVariable* var);
	};

	class VariableDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		ASTExprNode* default_value;
		bool is_rest;

		VariableDefinition(const std::string& identifier, Type type, const std::string& type_name,
			const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
			ASTExprNode* default_value, bool is_rest, unsigned int row, unsigned int col);

		VariableDefinition();

		static VariableDefinition get_basic(const std::string& identifier, parser::Type type,
			ASTExprNode* default_value = nullptr, bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

		static VariableDefinition get_array(const std::string& identifier, parser::Type type, parser::Type array_type,
			std::vector<ASTExprNode*>&& dim = std::vector<ASTExprNode*>(), ASTExprNode* default_value = nullptr,
			bool is_rest = false, unsigned int row = 0, unsigned int col = 0);

		static VariableDefinition get_struct(const std::string& identifier, parser::Type type,
			const std::string& type_name, const std::string& type_name_space, ASTExprNode* default_value = nullptr,
			bool is_rest = false, unsigned int row = 0, unsigned int col = 0);
	};

	class FunctionDefinition : public TypeDefinition, public CodePosition {
	public:
		std::string identifier;
		std::vector<parser::TypeDefinition> signature;
		std::vector<parser::VariableDefinition> parameters;

		FunctionDefinition(const std::string& identifier, Type type, const std::string& type_name,
			const std::string& type_name_space, Type array_type, const std::vector<ASTExprNode*>& dim,
			const std::vector<TypeDefinition>& signature, const std::vector<VariableDefinition>& parameters,
			unsigned int row, unsigned int col);

		FunctionDefinition(const std::string& identifier, unsigned int row, unsigned int col);

		FunctionDefinition();

		void check_signature() const;
	};

	class StructureDefinition : public CodePosition {
	public:
		std::string identifier;
		std::vector<VariableDefinition> variables;

		StructureDefinition(const std::string& identifier, std::vector<VariableDefinition>& variables,
			unsigned int row, unsigned int col);

		StructureDefinition();
	};

	class Identifier {
	public:
		std::string identifier;
		std::vector<ASTExprNode*> access_vector;

		Identifier(const std::string& identifier, std::vector<ASTExprNode*>&& access_vector);

		Identifier();
	};

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

	class ASTProgramNode : public ASTNode {
	public:
		std::string name;
		std::string alias;
		std::vector<ASTNode*> statements;
		std::vector<std::string> libs;

		explicit ASTProgramNode(const std::string& name, const std::string& alias,
			std::vector<ASTNode*>&& statements);

		void accept(visitor::Visitor*) override;
	};

	class ASTUsingNode : public ASTStatementNode {
	public:
		std::vector<std::string> library;

		ASTUsingNode(const std::vector<std::string>& library,
			unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTAsNamespaceNode : public ASTStatementNode {
	public:
		std::string nmspace;

		ASTAsNamespaceNode(const std::string& nmspace, unsigned int col, unsigned int row);

		void accept(visitor::Visitor*) override;
	};

	class ASTDeclarationNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		ASTExprNode* expr;
		bool is_const;

		ASTDeclarationNode(const std::string& identifier, Type type, Type array_type,
			std::vector<ASTExprNode*>&& dim, const std::string& type_name,
			const std::string& type_name_space, ASTExprNode* expr, bool is_const,
			unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTAssignmentNode : public ASTStatementNode {
	public:
		std::vector<Identifier> identifier_vector;
		std::string nmspace;
		std::string op;
		ASTExprNode* expr;

		ASTAssignmentNode(std::vector<Identifier>&& identifier_vector, const std::string& nmspace,
			const std::string& op, ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTReturnNode : public ASTStatementNode {
	public:
		ASTExprNode* expr;

		ASTReturnNode(ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTBlockNode : public ASTStatementNode {
	public:
		std::vector<ASTNode*> statements;

		ASTBlockNode(std::vector<ASTNode*>&& statements, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTContinueNode : public ASTStatementNode {
	public:
		ASTContinueNode(unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTBreakNode : public ASTStatementNode {
	public:
		ASTBreakNode(unsigned int row, unsigned int col);

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
		std::map<ASTExprNode*, unsigned int> case_blocks;
		std::map<unsigned int, unsigned int> parsed_case_blocks;
		unsigned int default_block;
		std::vector<ASTNode*> statements;

		ASTSwitchNode(ASTExprNode* condition, std::vector<ASTNode*>&& statements,
			std::map<ASTExprNode*, unsigned int>&& case_blocks,
			unsigned int default_block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTIfNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* if_block;
		std::vector<ASTElseIfNode*> else_ifs;
		ASTBlockNode* else_block;

		ASTIfNode(ASTExprNode* condition, ASTBlockNode* if_block, std::vector<ASTElseIfNode*>&& else_ifs,
			ASTBlockNode* else_block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTElseIfNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* block;

		ASTElseIfNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTEnumNode : public ASTStatementNode {
	public:
		std::vector<std::string> identifiers;

		ASTEnumNode(std::vector<std::string>&& identifiers, unsigned int row, unsigned int col);

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

		ASTForNode(std::array<ASTNode*, 3>&& dci, ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTForEachNode : public ASTStatementNode {
	public:
		ASTDeclarationNode* itdecl;
		ASTNode* collection;
		ASTBlockNode* block;

		ASTForEachNode(ASTDeclarationNode* itdecl, ASTNode* collection, ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTWhileNode : public ASTStatementNode {
	public:
		ASTExprNode* condition;
		ASTBlockNode* block;

		ASTWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTDoWhileNode : public ASTWhileNode {
	public:
		ASTDoWhileNode(ASTExprNode* condition, ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTStructDefinitionNode : public ASTStatementNode {
	public:
		std::string identifier;
		std::vector<VariableDefinition> variables;

		ASTStructDefinitionNode(const std::string& identifier, std::vector<VariableDefinition>&& variables,
			unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

	class ASTFunctionDefinitionNode : public ASTStatementNode, public TypeDefinition {
	public:
		std::string identifier;
		std::vector<VariableDefinition> parameters;
		std::vector<std::string> variable_names;
		std::vector<TypeDefinition> signature;
		ASTBlockNode* block;

		ASTFunctionDefinitionNode(const std::string& identifier, std::vector<VariableDefinition>&& parameters,
			Type type, const std::string& type_name, const std::string& type_name_space, Type array_type, std::vector<ASTExprNode*>&& dim,
			ASTBlockNode* block, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
	};

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

		ASTArrayConstructorNode(const std::vector<ASTExprNode*>& values, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTStructConstructorNode : public ASTExprNode {
	public:
		std::string type_name;
		std::string nmspace;
		std::map<std::string, ASTExprNode*> values;

		ASTStructConstructorNode(const std::string& type_name, const std::string& nmspace,
			const std::map<std::string, ASTExprNode*>& values, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTNullNode : public ASTExprNode {
	public:
		ASTNullNode(unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTThisNode : public ASTExprNode {
	public:
		ASTThisNode(unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTBinaryExprNode : public ASTExprNode {
	public:
		std::string op;
		ASTExprNode* left;
		ASTExprNode* right;

		ASTBinaryExprNode(const std::string& op, ASTExprNode* left, ASTExprNode* right, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTUnaryExprNode : public ASTExprNode {
	public:
		std::string unary_op;
		ASTExprNode* expr;

		ASTUnaryExprNode(const std::string& unary_op, ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTIdentifierNode : public ASTExprNode {
	public:
		std::vector<Identifier> identifier_vector;
		std::string nmspace;

		explicit ASTIdentifierNode(std::vector<Identifier>&& identifier_vector, std::string nmspace, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTernaryNode : public ASTExprNode {
	public:
		ASTExprNode* condition;
		ASTExprNode* value_if_true;
		ASTExprNode* value_if_false;

		ASTTernaryNode(ASTExprNode* condition, ASTExprNode* value_if_true, ASTExprNode* value_if_false, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTInNode : public ASTExprNode {
	public:
		ASTExprNode* value;
		ASTExprNode* collection;
		bool vbv;

		ASTInNode(ASTExprNode* value, ASTExprNode* collection, bool vbv, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTFunctionCallNode : public ASTExprNode {
	public:
		std::string identifier;
		std::string nmspace;
		std::vector<ASTExprNode*> access_vector;
		std::vector<std::pair<bool, ASTExprNode*>> parameters;

		ASTFunctionCallNode(const std::string& identifier, const std::string& nmspace, std::vector<ASTExprNode*>&& access_vector,
			std::vector<std::pair<bool, ASTExprNode*>>&& parameters, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypeParseNode : public ASTExprNode {
	public:
		Type type;
		ASTExprNode* expr;

		ASTTypeParseNode(Type type, ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

	class ASTTypingNode : public ASTExprNode {
	public:
		std::string image;
		ASTExprNode* expr;

		ASTTypingNode(const std::string& image, ASTExprNode* expr, unsigned int row, unsigned int col);

		void accept(visitor::Visitor*) override;
		virtual unsigned int hash(visitor::Visitor*) override;
	};

}

#endif // !AST_HPP
