#ifndef FILES_HPP
#define FILES_HPP

#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>

#include "module.hpp"

namespace modules {
	class Files : public Module {
	private:
		std::vector<std::fstream*> files;

	public:
		Files();
		~Files();

		void register_functions(SemanticAnalyser* visitor) override;
		void register_functions(Interpreter* visitor) override;
	};
}

#endif // !FILES_HPP
