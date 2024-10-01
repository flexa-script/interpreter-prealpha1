#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <cstdint>

#include "vmconstants.hpp"

#define byteopnd(x) reinterpret_cast<uint8_t*>(x)
#define byteopnd8(x) byteopnd(uint8_t(x))
#define byteopnd_n byteopnd8(0)
#define byteopnd_t byteopnd8(true)
#define byteopnd_f byteopnd8(false)

struct BytecodeInstruction {
	OpCode opcode;
	uint8_t* operand;

	//BytecodeInstruction(OpCode opcode, uint8_t* operand)
	//	: opcode(opcode), operand(operand) {}

	//BytecodeInstruction(OpCode opcode, uint64_t operand)
	//	: opcode(opcode), operand(reinterpret_cast<uint8_t*>(operand)) {}
};

#endif // !BYTECODE_HPP
