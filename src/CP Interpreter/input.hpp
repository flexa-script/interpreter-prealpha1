#ifndef INPUT_HPP
#define INPUT_HPP

#include <functional>
#include "module.hpp"

namespace modules {
	class Input : public Module {
	public:
		Input();
		~Input();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !INPUT_HPP
