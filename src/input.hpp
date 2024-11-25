#ifndef INPUT_HPP
#define INPUT_HPP

#include <thread>
#include <atomic>
#include <mutex>
#include <array>

#include "module.hpp"

namespace modules {
	class Input : public Module {
	public:
		static const int KEY_COUNT = 256;

	private:
		std::array<bool, KEY_COUNT> previous_key_state = { false };
		std::array<bool, KEY_COUNT> current_key_state = { false };
		std::thread key_update_thread;
		std::mutex state_mutex;
		std::atomic<bool> running;

		void key_update_loop();

	public:
		Input();
		~Input();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;

		void start();
		void stop();
	};
}

#endif // !INPUT_HPP
