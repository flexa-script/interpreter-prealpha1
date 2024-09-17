#ifndef DATETIME_HPP
#define DATETIME_HPP

#include "module.hpp"

namespace modules {
	class DateTime : public Module {
	public:
		DateTime();
		~DateTime();

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !DATETIME_HPP
