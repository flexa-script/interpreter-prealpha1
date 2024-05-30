#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <string>
#include <functional>

#include "interpreter.hpp"
#include "vendor/window.hpp"

namespace modules {
	class Graphics {
	private:
		std::vector<axe::Window*> windows;

	public:
		Graphics();

		void register_functions(visitor::Interpreter* interpreter);
	};
}

#endif // CPGRAPHICS_HPP
