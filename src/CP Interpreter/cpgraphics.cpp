#include "cpgraphics.hpp"

using namespace modules;


CPGraphics::CPGraphics() : graphic_engine(std::vector<axe::Graphics>()) {}

void CPGraphics::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		// initialize window struct values
		Value* win = interpreter->last_function_arguments[0].second;
		win->str->second["title"] = new Value(interpreter->last_function_arguments[1].second);
		win->str->second["width"] = new Value(interpreter->last_function_arguments[2].second);
		win->str->second["height"] = new Value(interpreter->last_function_arguments[3].second);

		// create a new window graphic engine
		graphic_engine.push_back(axe::Graphics());
		win->str->second["__instance_index"] = new Value(parser::Type::T_INT);
		win->str->second["__instance_index"]->i = graphic_engine.size() - 1;

		// initialize window graphic engine and return value
		auto str = win->str->second["title"]->s;
		auto wstr = std::wstring(str.begin(), str.end());
		auto rval = Value(parser::Type::T_BOOL);
		rval.b = graphic_engine[win->str->second["__instance_index"]->i].initialize(
			wstr.c_str(),
			(int)win->str->second["width"]->i,
			(int)win->str->second["height"]->i
		);
		interpreter->current_expression_value = rval;
	};

	interpreter->builtin_functions["clear_screen"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		int r, g, b;
		r = (int)interpreter->last_function_arguments[1].second->str->second["r"]->i;
		g = (int)interpreter->last_function_arguments[1].second->str->second["g"]->i;
		b = (int)interpreter->last_function_arguments[1].second->str->second["b"]->i;
		graphic_engine[win->str->second["__instance_index"]->i].clear_screen(RGB(r, g, b));
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		int x, y, r, g, b;
		x = (int)interpreter->last_function_arguments[1].second->i;
		y = (int)interpreter->last_function_arguments[2].second->i;
		r = (int)interpreter->last_function_arguments[3].second->str->second["r"]->i;
		g = (int)interpreter->last_function_arguments[3].second->str->second["g"]->i;
		b = (int)interpreter->last_function_arguments[3].second->str->second["b"]->i;
		graphic_engine[win->str->second["__instance_index"]->i].draw_pixel(x, y, RGB(r, g, b));
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		graphic_engine[win->str->second["__instance_index"]->i].update();
	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		graphic_engine[win->str->second["__instance_index"]->i].~Graphics();
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		auto val = Value(parser::Type::T_BOOL);
		val.b = graphic_engine[win->str->second["__instance_index"]->i].is_quit();
		interpreter->current_expression_value = val;
	};
}
