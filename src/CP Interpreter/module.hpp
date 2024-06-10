#ifndef MODULE_HPP
#define MODULE_HPP

#include <string>
#include "interpreter.hpp"

namespace modules {
	class Module {
	public:
		static const std::string INSTANCE_ID_NAME;

	public:
		Module() = default;

		virtual void register_functions(visitor::Interpreter* interpreter) = 0;
	};
}

#endif // !MODULE_HPP



