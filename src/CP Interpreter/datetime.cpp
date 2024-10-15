#include "datetime.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;

DateTime::DateTime() {}

DateTime::~DateTime() = default;

cp_struct DateTime::tm_to_date_time(time_t t, tm* tm) {
	cp_struct dt_str = cp_struct();
	dt_str["timestamp"] = new RuntimeValue(cp_int(t));
	dt_str["second"] = new RuntimeValue(cp_int(tm->tm_sec));
	dt_str["minute"] = new RuntimeValue(cp_int(tm->tm_min));
	dt_str["hour"] = new RuntimeValue(cp_int(tm->tm_hour));
	dt_str["day"] = new RuntimeValue(cp_int(tm->tm_mday));
	dt_str["month"] = new RuntimeValue(cp_int(tm->tm_mon + 1));
	dt_str["year"] = new RuntimeValue(cp_int(tm->tm_year));
	dt_str["week_day"] = new RuntimeValue(cp_int(tm->tm_wday));
	dt_str["year_day"] = new RuntimeValue(cp_int(tm->tm_yday));
	dt_str["is_dst"] = new RuntimeValue(cp_int(tm->tm_isdst));

	return dt_str;
}

void DateTime::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_date_time"] = nullptr;
	visitor->builtin_functions["diff_date_time"] = nullptr;
	visitor->builtin_functions["format_date_time"] = nullptr;
	visitor->builtin_functions["format_local_date_time"] = nullptr;
	visitor->builtin_functions["ascii_date_time"] = nullptr;
	visitor->builtin_functions["ascii_local_date_time"] = nullptr;
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

		visitor->current_expression_value = new RuntimeValue(tm_to_date_time(t, tm), "DateTime", "cp");

		};

	visitor->builtin_functions["diff_date_time"] = [this, visitor]() {
		time_t lt = visitor->builtin_arguments[0]->get_str()["timestamp"]->get_i();
		time_t rt = visitor->builtin_arguments[1]->get_str()["timestamp"]->get_i();
		time_t t = difftime(lt, rt);
		tm* tm = new struct tm();
		gmtime_s(tm, &t);

		visitor->current_expression_value = new RuntimeValue(tm_to_date_time(t, tm), "DateTime", "cp");

		};

	visitor->builtin_functions["format_date_time"] = [this, visitor]() {
		time_t t = visitor->builtin_arguments[0]->get_str()["timestamp"]->get_i();
		std::string fmt = visitor->builtin_arguments[1]->get_s();
		tm* tm = new struct tm();
		gmtime_s(tm, &t);
		char buffer[80];
		strftime(buffer, 80, fmt.c_str(), tm);

		visitor->current_expression_value = new RuntimeValue(std::string{ buffer });

		};

	visitor->builtin_functions["format_local_date_time"] = [this, visitor]() {
		time_t t = visitor->builtin_arguments[0]->get_str()["timestamp"]->get_i();
		std::string fmt = visitor->builtin_arguments[1]->get_s();
		tm* tm = new struct tm();
		localtime_s(tm, &t);
		char buffer[80];
		strftime(buffer, 80, fmt.c_str(), tm);

		visitor->current_expression_value = new RuntimeValue(std::string{ buffer });

		};

	visitor->builtin_functions["ascii_date_time"] = [this, visitor]() {
		time_t t = visitor->builtin_arguments[0]->get_str()["timestamp"]->get_i();
		tm* tm = new struct tm();
		gmtime_s(tm, &t);
		char buffer[26];
		
		if (asctime_s(buffer, sizeof(buffer), tm) != 0) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

		visitor->current_expression_value = new RuntimeValue(std::string{ buffer });

		};

	visitor->builtin_functions["ascii_local_date_time"] = [this, visitor]() {
		time_t t = visitor->builtin_arguments[0]->get_str()["timestamp"]->get_i();
		tm* tm = new struct tm();
		localtime_s(tm, &t);
		char buffer[26];

		if (asctime_s(buffer, sizeof(buffer), tm) != 0) {
			throw std::runtime_error("Error trying to convert date/time to ASCII string");
		}

		visitor->current_expression_value = new RuntimeValue(std::string{ buffer });

		};

	visitor->builtin_functions["clock"] = [this, visitor]() {
		visitor->current_expression_value = new RuntimeValue(cp_int(clock()));

		};

}
