#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <functional>

#include "module.hpp"

namespace modules {
	class Console : public Module {
	public:
		Console();

		void register_functions(visitor::Interpreter* interpreter) override;
	};
}

#endif // !CONSOLE_HPP
