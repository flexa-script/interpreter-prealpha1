#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <iostream>
#include <cstdint>

#include "vm_constants.hpp"
#include "types.hpp"

namespace vm {

	class BytecodeInstruction {
	public:
		OpCode opcode;
		uint8_t* operand;

		BytecodeInstruction();
		BytecodeInstruction(OpCode opcode, uint8_t* operand);
		BytecodeInstruction(OpCode opcode, uint8_t operand);
		BytecodeInstruction(OpCode opcode, size_t operand);
		BytecodeInstruction(OpCode opcode, cp_bool operand);
		BytecodeInstruction(OpCode opcode, cp_int operand);
		BytecodeInstruction(OpCode opcode, cp_float operand);
		BytecodeInstruction(OpCode opcode, cp_char operand);
		BytecodeInstruction(OpCode opcode, const cp_string& operand);

		uint8_t get_uint8_operand() const;
		size_t get_size_operand() const;
		cp_bool get_bool_operand() const;
		cp_int get_int_operand() const;
		cp_float get_float_operand() const;
		cp_char get_char_operand() const;
		cp_string get_string_operand() const;

		template <typename T>
		static uint8_t* to_byteopnd(T value);
		static uint8_t* to_byteopnd(const std::string& str);

		static void write_bytecode_table(const std::vector<BytecodeInstruction>& instructions, const std::string& filename);
	};

}

#endif // !BYTECODE_HPP
