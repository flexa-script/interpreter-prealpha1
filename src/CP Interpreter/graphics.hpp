#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <functional>

#include "module.hpp"
#include "vendor/window.hpp"

namespace modules {
	class Graphics : public Module {
	private:
		std::vector<axe::Window*> windows;

	public:
		Graphics();

		void register_functions(visitor::Interpreter* interpreter) override;
	};
}

#endif // !GRAPHICS_HPP
