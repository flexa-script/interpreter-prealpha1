#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <map>
#include <string>
#include <functional>

#include "interpreter.hpp"
#include "vendor/graphics.hpp"

namespace modules {
	class CPGraphics {
	private:
		std::vector<axe::Graphics> graphic_engine;

	public:
		CPGraphics();

		void register_functions(visitor::Interpreter* interpreter);
	};
}

#endif // CPGRAPHICS_HPP
