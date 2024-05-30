#include "files.hpp"

using namespace modules;

Files::Files() {}

void Files::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		// initialize window struct values
		Value* win = interpreter->last_function_arguments[0].second;
		win->str->second["title"] = new Value(interpreter->last_function_arguments[1].second);
		win->str->second["width"] = new Value(interpreter->last_function_arguments[2].second);
		win->str->second["height"] = new Value(interpreter->last_function_arguments[3].second);

	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			// is not the ideal
			win->str->second["title"]->set_null();
			win->str->second["width"]->set_null();
			win->str->second["height"]->set_null();
			win->set_null();
		}
	};

}
