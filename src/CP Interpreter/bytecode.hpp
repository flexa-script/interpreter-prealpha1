#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <cstdint>

#include "vmconstants.hpp"

#define byteoperand(x) reinterpret_cast<uint8_t*>(x)

struct BytecodeInstruction {
	OpCode opcode;
	uint8_t* operand;

	//BytecodeInstruction(OpCode opcode, uint8_t* operand)
	//	: opcode(opcode), operand(operand) {}

	//BytecodeInstruction(OpCode opcode, uint64_t operand)
	//	: opcode(opcode), operand(reinterpret_cast<uint8_t*>(operand)) {}
};

#endif // !BYTECODE_HPP
