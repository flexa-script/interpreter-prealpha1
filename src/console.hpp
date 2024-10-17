#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <functional>

#include "module.hpp"

namespace modules {
	class Console : public Module {
	public:
		Console();
		~Console();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !CONSOLE_HPP
