#ifndef FILES_HPP
#define FILES_HPP

#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>

#include "module.hpp"

namespace modules {
	class Files : public Module {
	public:
		Files();
		~Files();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
		void register_functions(visitor::Compiler* visitor) override;
		void register_functions(VirtualMachine* vm) override;
	};
}

#endif // !FILES_HPP
