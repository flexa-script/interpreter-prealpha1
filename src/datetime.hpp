#ifndef DATETIME_HPP
#define DATETIME_HPP

#include <ctime>

#include "module.hpp"
#include "types.hpp"

namespace modules {
	class DateTime : public Module {
	public:
		DateTime();
		~DateTime();

		cp_struct tm_to_date_time(time_t t, tm* tm);

		void register_functions(visitor::SemanticAnalyser* visitor) override;
		void register_functions(visitor::Interpreter* visitor) override;
	};
}

#endif // !DATETIME_HPP
