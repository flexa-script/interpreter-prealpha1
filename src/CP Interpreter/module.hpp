#ifndef MODULE_HPP
#define MODULE_HPP

#include <string>
#include "interpreter.hpp"
#include "semantic_analysis.hpp"

namespace modules {
	class Module {
	public:
		static const std::string INSTANCE_ID_NAME;

	public:
		Module() = default;
		~Module() = default;

		virtual void register_functions(SemanticAnalyser* visitor) = 0;
		virtual void register_functions(Interpreter* visitor) = 0;
	};
}

#endif // !MODULE_HPP



