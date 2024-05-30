#ifndef FILES_HPP
#define FILES_HPP

#include <functional>

#include "module.hpp"

namespace modules {
	class Files : public Module {
	public:
		Files();

		void register_functions(visitor::Interpreter* interpreter) override;
	};
}

#endif // !FILES_HPP
