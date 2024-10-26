#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <memory>
#include <map>
#include <vector>
#include <stack>
#include <xutility>
#include <functional>

#include "bytecode.hpp"
#include "ast.hpp"
#include "namespace_manager.hpp"

using namespace visitor;
using namespace parser;

namespace visitor {

	class Compiler : public Visitor, public NamespaceManager {
	public:
		std::vector<BytecodeInstruction> bytecode_program;
		std::map<std::string, void*> builtin_functions;

	private:
		size_t pointer = 0;
		std::stack<size_t> deviation_stack;
		std::vector<std::string> parsed_libs;

	private:
		template <typename T>
		size_t add_instruction(OpCode opcode, T operand);
		template <typename T>
		void replace_last_operand(size_t pos, T operand);

		void type_definition_operations(TypeDefinition type);
		void access_sub_value_operations(std::vector<Identifier> identifier_vector);

		bool has_sub_value(std::vector<Identifier> identifier_vector);

		bool push_namespace(const std::string nmspace) override;
		void pop_namespace(bool pop) override;
		void set_curr_pos(unsigned int row, unsigned int col) override;
		std::string msg_header() override;

	public:
		Compiler(ASTProgramNode* main_program, std::map<std::string, ASTProgramNode*> programs);
		~Compiler() = default;

		void start();

		void visit(ASTProgramNode*) override;
		void visit(ASTUsingNode*) override;
		void visit(ASTNamespaceManagerNode*) override;
		void visit(ASTDeclarationNode*) override;
		void visit(ASTUnpackedDeclarationNode*) override;
		void visit(ASTAssignmentNode*) override;
		void visit(ASTReturnNode*) override;
		void visit(ASTExitNode*) override;
		void visit(ASTBlockNode*) override;
		void visit(ASTContinueNode*) override;
		void visit(ASTBreakNode*) override;
		void visit(ASTSwitchNode*) override;
		void visit(ASTEnumNode*) override;
		void visit(ASTTryCatchNode*) override;
		void visit(ASTThrowNode*) override;
		void visit(ASTReticencesNode*) override;
		void visit(ASTElseIfNode*) override;
		void visit(ASTIfNode*) override;
		void visit(ASTForNode*) override;
		void visit(ASTForEachNode*) override;
		void visit(ASTWhileNode*) override;
		void visit(ASTDoWhileNode*) override;
		void visit(ASTFunctionDefinitionNode*) override;
		void visit(ASTStructDefinitionNode*) override;
		void visit(ASTLiteralNode<cp_bool>*) override;
		void visit(ASTLiteralNode<cp_int>*) override;
		void visit(ASTLiteralNode<cp_float>*) override;
		void visit(ASTLiteralNode<cp_char>*) override;
		void visit(ASTLiteralNode<cp_string>*) override;
		void visit(ASTFunctionExpression*) override;
		void visit(ASTArrayConstructorNode*) override;
		void visit(ASTStructConstructorNode*) override;
		void visit(ASTBinaryExprNode*) override;
		void visit(ASTUnaryExprNode*) override;
		void visit(ASTIdentifierNode*) override;
		void visit(ASTTernaryNode*) override;
		void visit(ASTInNode*) override;
		void visit(ASTFunctionCallNode*) override;
		void visit(ASTTypeParseNode*) override;
		void visit(ASTNullNode*) override;
		void visit(ASTThisNode*) override;
		void visit(ASTTypingNode*) override;
		void visit(ASTValueNode*) override;

		long long hash(ASTExprNode*) override;
		long long hash(ASTValueNode*) override;
		long long hash(ASTIdentifierNode*) override;
		long long hash(ASTLiteralNode<cp_bool>*) override;
		long long hash(ASTLiteralNode<cp_int>*) override;
		long long hash(ASTLiteralNode<cp_float>*) override;
		long long hash(ASTLiteralNode<cp_char>*) override;
		long long hash(ASTLiteralNode<cp_string>*) override;
	};
}

#endif // !COMPILER_HPP
