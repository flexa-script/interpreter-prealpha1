#ifndef VMCONSTANTS_HPP
#define VMCONSTANTS_HPP

enum OpCode : short {
	//OP_PUSH,
	//OP_LOAD, // LOAD FROM ADDRESS (VARIABLES, CONSTANTS)
	//OP_ASSGN,
	//OP_JSR, // JUMP TO SUBROUTINE
	//OP_JMPF, // JUMP IF FALSE
	//OP_JMPT, // JUMP IF TRUE
	//OP_JMP, // UNCONDITIONAM JUMP
	//OP_RET, // RETURN FROM SUBROUTINE
	//OP_EXIT,
	//OP_TRYS,
	//OP_TRYE,
	//OP_THROW
	OP_RES,
	OP_LOAD_BOOL_CONST,  // Carrega uma constante na pilha
	OP_LOAD_INT_CONST,  // Carrega uma constante na pilha
	OP_LOAD_FLOAT_CONST,  // Carrega uma constante na pilha
	OP_LOAD_CHAR_CONST,  // Carrega uma constante na pilha
	OP_LOAD_STRING_CONST,  // Carrega uma constante na pilha
	OP_LOAD_VAR,    // Carrega uma variável na pilha
	OP_STORE_VAR,   // Armazena o valor do topo da pilha em uma variável
	OP_ADD,         // Soma os dois valores no topo da pilha
	OP_SUB,         // Subtrai os dois valores no topo da pilha
	OP_MUL,         // Multiplica os dois valores no topo da pilha
	OP_DIV,         // Divide os dois valores no topo da pilha
	OP_PRINT,       // Imprime o valor do topo da pilha
	OP_JUMP,        // Salta para uma instrução
	OP_JUMP_IF_FALSE, // Salta se o topo da pilha for falso
	OP_JUMP_IF_TRUE,  // Salta se o topo da pilha for TRUE
	OP_CALL,          // Chama uma função
	OP_RETURN,        // Retorna de uma função
	OP_FUN_START,
	OP_FUN_END,
	OP_TRY_START,
	OP_TRY_END,
	OP_THROW,
	OP_EXIT,
	OP_HALT         // Finaliza a execução
};

#endif // !VMCONSTANTS_HPP
