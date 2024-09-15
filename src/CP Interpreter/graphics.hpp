#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <functional>

#include "module.hpp"
#include "vendor/axegraphics.hpp"

namespace modules {
	class Graphics : public Module {
	private:
		std::vector<axe::Image*> images;
		std::vector<axe::Font*> fonts;
		std::vector<axe::Window*> windows;

	public:
		Graphics();
		~Graphics();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !CPGRAPHICS_HPP
