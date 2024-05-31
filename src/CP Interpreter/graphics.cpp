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
		win->str->second[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		win->str->second[INSTANCE_ID_NAME]->i = windows.size() - 1;

		// initialize window graphic engine and return value
		auto str = win->str->second["title"]->s;
		auto wstr = std::wstring(str.begin(), str.end());
		auto rval = Value(parser::Type::T_BOOL);
		rval.b = windows[win->str->second[INSTANCE_ID_NAME]->i]->initialize(
			wstr.c_str(),
			(int)win->str->second["width"]->i,
			(int)win->str->second["height"]->i
		);
		interpreter->current_expression_value = rval;
	};

	interpreter->builtin_functions["clear_screen"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int r, g, b;
				r = (int)interpreter->last_function_arguments[1].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[1].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[1].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->clear_screen(RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, r, g, b;
				x = (int)interpreter->last_function_arguments[1].second->i;
				y = (int)interpreter->last_function_arguments[2].second->i;
				r = (int)interpreter->last_function_arguments[3].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[3].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[3].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_line"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x1, y1, x2, y2, r, g, b;
				x1 = (int)interpreter->last_function_arguments[1].second->i;
				y1 = (int)interpreter->last_function_arguments[2].second->i;
				x2 = (int)interpreter->last_function_arguments[3].second->i;
				y2 = (int)interpreter->last_function_arguments[4].second->i;
				r = (int)interpreter->last_function_arguments[5].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[5].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[5].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_line(x1, y1, x2, y2, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_rect"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->last_function_arguments[1].second->i;
				y = (int)interpreter->last_function_arguments[2].second->i;
				width = (int)interpreter->last_function_arguments[3].second->i;
				height = (int)interpreter->last_function_arguments[4].second->i;
				r = (int)interpreter->last_function_arguments[5].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[5].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[5].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_rect"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->last_function_arguments[1].second->i;
				y = (int)interpreter->last_function_arguments[2].second->i;
				width = (int)interpreter->last_function_arguments[3].second->i;
				height = (int)interpreter->last_function_arguments[4].second->i;
				r = (int)interpreter->last_function_arguments[5].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[5].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[5].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->fill_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_circle"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->last_function_arguments[1].second->i;
				yc = (int)interpreter->last_function_arguments[2].second->i;
				radius = (int)interpreter->last_function_arguments[3].second->i;
				r = (int)interpreter->last_function_arguments[4].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[4].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[4].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_circle"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->last_function_arguments[1].second->i;
				yc = (int)interpreter->last_function_arguments[2].second->i;
				radius = (int)interpreter->last_function_arguments[3].second->i;
				r = (int)interpreter->last_function_arguments[4].second->str->second["r"]->i;
				g = (int)interpreter->last_function_arguments[4].second->str->second["g"]->i;
				b = (int)interpreter->last_function_arguments[4].second->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->fill_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				windows[win->str->second[INSTANCE_ID_NAME]->i]->update();
			}
		}
	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				windows[win->str->second[INSTANCE_ID_NAME]->i]->~Window();
				windows[win->str->second[INSTANCE_ID_NAME]->i] = nullptr;
				// set null
				win->str->second["title"]->set_null();
				win->str->second["width"]->set_null();
				win->str->second["height"]->set_null();
				win->str->second[INSTANCE_ID_NAME]->set_null();
				win->set_null();
			}
		}
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		Value* win = interpreter->last_function_arguments[0].second;
		auto val = Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				val.b = windows[win->str->second[INSTANCE_ID_NAME]->i]->is_quit();
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
