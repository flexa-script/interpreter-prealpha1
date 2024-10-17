#ifndef INPUT_HPP
#define INPUT_HPP

#include <array>
#include <functional>

#include "module.hpp"

namespace modules {
	class Input : public Module {
	public:
		static const int KEY_COUNT = 256;

	private:
		std::array<bool, KEY_COUNT> previous_key_state = { false };
		std::array<bool, KEY_COUNT> current_key_state = { false };

	public:
		Input();
		~Input();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;

	};
}

#endif // !INPUT_HPP
