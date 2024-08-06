#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <functional>

#include "module.hpp"
#include "vendor/axegraphics.hpp"

namespace modules {
	class Graphics : public Module {
	private:
		std::vector<axe::Image*> images;
		std::vector<axe::Window*> windows;

	public:
		Graphics();
		~Graphics();

		void register_functions(SemanticAnalyser* visitor) override;
		void register_functions(Interpreter* visitor) override;
	};
}

#endif // !CPGRAPHICS_HPP
