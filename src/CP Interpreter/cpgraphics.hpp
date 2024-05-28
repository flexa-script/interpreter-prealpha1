#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <map>
#include <string>
#include <functional>

#include "visitor.hpp"
#include "semantic_analysis.hpp"
#include "interpreter.hpp"
#include "vendor/graphics.hpp"

namespace modules {
	class CPGraphics {
	private:
		axe::Graphics graphic_engine;

	public:
		CPGraphics();

		void register_semantic_functions(visitor::Visitor* semantic_analyser);
		void register_interpreter_functions(visitor::Visitor* interpreter);
	};
}

#endif // CPGRAPHICS_HPP
