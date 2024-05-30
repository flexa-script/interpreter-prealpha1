#include "graphics.hpp"
#include <iostream>

using namespace modules;


Graphics::Graphics() : windows(std::vector<axe::Window*>()) {}

void Graphics::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		// initialize window struct values
		Value* win = interpreter->last_function_arguments[0].second;
		win->str->second["title"] = new Value(interpreter->last_function_arguments[1].second);
		win->str->second["width"] = new Value(interpreter->last_function_arguments[2].second);
		win->str->second["height"] = new Value(interpreter->last_function_arguments[3].second);

		// create a new window graphic engine
		windows.push_back(new axe::Window());
		win->str->second["__instance_index"] = new Value(parser::Type::T_INT);
		win->str->second["__instance_index"]->i = windows.size() - 1;

		// initialize window graphic engine and return value
		auto str = win->str->second["title"]->s;
		auto wstr = std::wstring(str.begin(), str.end());
		auto rval = Value(parser::Type::T_BOOL);
		rval.b = windows[win->str->second["__instance_index"]->i]->initialize(
			wstr.c_str(),
			(int)win->str->second["width"]->i,
			(int)win->str->second["height"]->i
		);
		interpreter->current_expression_value = rval;
	};

	interpreter->builtin_functions["clear_screen"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second["__instance_index"]->i]) {
				int r, g, b;
				r = (int)interpreter->last_function_arguments[1].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[1].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[1].second->str->second["b"]->i;
				windows[win->str->second["__instance_index"]->i]->clear_screen(RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second["__instance_index"]->i]) {
				int x, y, r, g, b;
				x = (int)interpreter->last_function_arguments[1].second->i;
				y = (int)interpreter->last_function_arguments[2].second->i;
				r = (int)interpreter->last_function_arguments[3].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[3].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[3].second->str->second["b"]->i;
				windows[win->str->second["__instance_index"]->i]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second["__instance_index"]->i]) {
				windows[win->str->second["__instance_index"]->i]->update();
			}
		}
	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second["__instance_index"]->i]) {
				windows[win->str->second["__instance_index"]->i]->~Window();
				windows[win->str->second["__instance_index"]->i] = nullptr;
			}
		}
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		auto val = Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second["__instance_index"]->i]) {
				val.b = windows[win->str->second["__instance_index"]->i]->is_quit();
			}
			else {
				val.b = true;
			}
		}
		else {
			val.b = true;
		}
		interpreter->current_expression_value = val;
	};
}
