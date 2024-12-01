#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include <unordered_map>
#include <functional>

#include "module.hpp"

class FunctionDefinition;

namespace modules {
	enum BuintinFuncs {
		PRINT,
		PRINTLN,
		READ,
		READCH,
		LEN,
		SLEEP,
		SYSTEM
	};

	extern std::string BUILTIN_NAMES[];

	class Builtin : public Module {
	private:
		std::unordered_map<std::string, FunctionDefinition> func_decls;

	public:
		Builtin();
		~Builtin();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
		void register_functions(visitor::Compiler* visitor) override;
		void register_functions(VirtualMachine* vm) override;

	private:
		void build_decls();
	};
}

#endif // !BUILTIN_HPP
