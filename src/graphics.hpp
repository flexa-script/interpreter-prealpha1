#ifndef CPGRAPHICS_HPP
#define CPGRAPHICS_HPP

#include <functional>

#include "module.hpp"
#include "vendor/axegraphics.hpp"

namespace modules {
	class Graphics : public Module {
	public:
		Graphics();
		~Graphics();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
		void register_functions(visitor::Compiler* visitor) override;
		void register_functions(VirtualMachine* vm) override;
	};
}

#endif // !CPGRAPHICS_HPP
