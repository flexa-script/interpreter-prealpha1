#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <stdint.h>
#include <any>
#include <memory>

#include "vmconstants.hpp"


struct BytecodeInstruction {
	OpCode opcode;
	std::shared_ptr<std::any> operand;
};


#endif // !BYTECODE_HPP
