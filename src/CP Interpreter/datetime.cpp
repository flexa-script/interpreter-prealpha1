#include <ctime>

#include "datetime.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;

DateTime::DateTime() {}

DateTime::~DateTime() = default;

void DateTime::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_date_time"] = nullptr;
	visitor->builtin_functions["local_date_time"] = nullptr;
	visitor->builtin_functions["utc_date_time"] = nullptr;
	visitor->builtin_functions["diff_date_time"] = nullptr;
	visitor->builtin_functions["format_date_time"] = nullptr;
	visitor->builtin_functions["ascii_date_time"] = nullptr;
	visitor->builtin_functions["clock"] = nullptr;
}

void DateTime::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["create_date_time"] = [this, visitor]() {
		tm* tm = new struct tm();
		time_t t;

		if (visitor->builtin_arguments.size() == 0) {
			t = time(nullptr);
			gmtime_s(tm, &t);
		}
		else if (visitor->builtin_arguments.size() == 1) {
			t = visitor->builtin_arguments[0]->get_i();
			gmtime_s(tm, &t);
		}
		else {
			tm->tm_year = visitor->builtin_arguments[0]->get_i() - 1900;
			tm->tm_mon = visitor->builtin_arguments[1]->get_i() - 1;
			tm->tm_mday = visitor->builtin_arguments[2]->get_i();
			tm->tm_hour = visitor->builtin_arguments[3]->get_i();
			tm->tm_min = visitor->builtin_arguments[4]->get_i();
			tm->tm_sec = visitor->builtin_arguments[5]->get_i();
			t = mktime(tm);
		}

		cp_struct dt_str = cp_struct();
		dt_str["timestamp"] = new Value(cp_int(t));
		dt_str["second"] = new Value(cp_int(tm->tm_sec));
		dt_str["minute"] = new Value(cp_int(tm->tm_min));
		dt_str["hour"] = new Value(cp_int(tm->tm_hour));
		dt_str["day"] = new Value(cp_int(tm->tm_mday));
		dt_str["month"] = new Value(cp_int(tm->tm_mon));
		dt_str["year"] = new Value(cp_int(tm->tm_year));
		dt_str["week_day"] = new Value(cp_int(tm->tm_wday));
		dt_str["year_day"] = new Value(cp_int(tm->tm_yday));
		dt_str["is_dst"] = new Value(cp_int(tm->tm_isdst));

		visitor->current_expression_value = new Value(dt_str, "DateTime", "cp");

		};

	visitor->builtin_functions["local_date_time"] = [this, visitor]() {
		

		};

	visitor->builtin_functions["utc_date_time"] = [this, visitor]() {


		};

	visitor->builtin_functions["diff_date_time"] = [this, visitor]() {


		};

	visitor->builtin_functions["format_date_time"] = [this, visitor]() {


		};

	visitor->builtin_functions["ascii_date_time"] = [this, visitor]() {


		};

	visitor->builtin_functions["clock"] = [this, visitor]() {


		};

}
