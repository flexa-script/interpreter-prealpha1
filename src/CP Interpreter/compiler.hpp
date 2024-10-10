#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <memory>
#include <map>
#include <vector>
#include <stack>
#include <xutility>
#include <functional>

#include "vmconstants.hpp"
#include "bytecode.hpp"
#include "ast.hpp"

using namespace visitor;
using namespace parser;

namespace visitor {

	class Compiler : Visitor {
	public:
		std::vector<BytecodeInstruction> bytecode_program;
		std::map<std::string, void*> builtin_functions;

	private:
		size_t pointer = 0;
		std::vector<std::string> parsed_libs;
		std::map<std::string, std::vector<std::string>> program_nmspaces;
		std::stack<std::string> current_namespace;

	private:
		void add_instruction(OpCode opcode, uint8_t* operand);
		void replace_last_operand(uint8_t* operand);

		void type_definition_operations(TypeDefinition type);
		void access_sub_value_operations(std::vector<Identifier> identifier_vector);
		void nmspace_array_operations();

		bool has_sub_value(std::vector<Identifier> identifier_vector);

		bool push_namespace(const std::string nmspace);
		void pop_namespace(bool pop);
		std::string build_namespace(const std::string& identifier) const;
		const std::string& get_namespace(const std::string& nmspace = "") const override;
		const std::string& get_namespace(const ASTProgramNode* program, const std::string& nmspace = "") const override;

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

		long long hash(ASTExprNode*) override;
		long long hash(ASTIdentifierNode*) override;
		long long hash(ASTLiteralNode<cp_bool>*) override;
		long long hash(ASTLiteralNode<cp_int>*) override;
		long long hash(ASTLiteralNode<cp_float>*) override;
		long long hash(ASTLiteralNode<cp_char>*) override;
		long long hash(ASTLiteralNode<cp_string>*) override;
	};
}

#endif // !COMPILER_HPP
