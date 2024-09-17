#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include "module.hpp"

namespace modules {
	class Builtin : public Module {
	public:
		Builtin();
		~Builtin();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !BUILTIN_HPP
