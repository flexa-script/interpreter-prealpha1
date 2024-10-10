#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <cstdint>

#include "vmconstants.hpp"

#define byteopnd(x) reinterpret_cast<uint8_t*>(x)
#define byteopnd8(x) byteopnd(uint8_t(x))
#define byteopnd_n byteopnd8(0)
#define byteopnd_t byteopnd8(true)
#define byteopnd_f byteopnd8(false)

uint8_t* byteopnd_s(std::string str) {
	const char* cs = reinterpret_cast<const char*>(str.size());
	const char* cstr = str.c_str();
	size_t size = strlen(cs) + strlen(cstr) + 1;
	char* concat{ new char[size] };
	strcpy_s(concat, size, cs);
	strcat_s(concat, size, cstr);
	return byteopnd(concat);
}

struct BytecodeInstruction {
	OpCode opcode;
	uint8_t* operand;

	//BytecodeInstruction(OpCode opcode, uint8_t* operand)
	//	: opcode(opcode), operand(operand) {}

	//BytecodeInstruction(OpCode opcode, uint64_t operand)
	//	: opcode(opcode), operand(reinterpret_cast<uint8_t*>(operand)) {}
};

#endif // !BYTECODE_HPP
