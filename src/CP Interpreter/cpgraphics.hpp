#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <map>
#include <string>
#include <functional>

#include "vendor/graphics.hpp"

namespace visitor {
	class SemanticAnalyser;
	class Interpreter;
}

class Value;

namespace parser {
	enum Type;
}

namespace modules {
	class CPGraphics {
	private:
		axe::Graphics graphic_engine;

	public:
		CPGraphics();

		void register_semantic_functions(visitor::SemanticAnalyser* semantic_analyser);
		void register_interpreter_functions(visitor::Interpreter* interpreter);
	};
}

#endif // CPGRAPHICS_HPP
