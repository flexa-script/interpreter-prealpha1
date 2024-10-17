#ifndef SOUND_HPP
#define SOUND_HPP

#include "module.hpp"

namespace modules {
	class Sound : public Module {
	public:
		Sound();
		~Sound();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !SOUND_HPP
