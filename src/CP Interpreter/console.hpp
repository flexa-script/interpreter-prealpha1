#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <functional>

#include "module.hpp"

namespace modules {
	class Console : public Module {
	public:
		Console();

		void register_functions(SemanticAnalyser* visitor) override;
		void register_functions(Interpreter* visitor) override;
	};
}

#endif // !CONSOLE_HPP
