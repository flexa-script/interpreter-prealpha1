#include "graphics.hpp"
#include <iostream>

using namespace modules;


Graphics::Graphics() : windows(std::vector<axe::Window*>()) {}

void Graphics::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		// initialize window struct values
		Value* win = interpreter->builtin_arguments[0];
		win->str->second["title"] = new Value(interpreter->builtin_arguments[1]);
		win->str->second["width"] = new Value(interpreter->builtin_arguments[2]);
		win->str->second["height"] = new Value(interpreter->builtin_arguments[3]);

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
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int r, g, b;
				r = (int)interpreter->builtin_arguments[1]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[1]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[1]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->clear_screen(RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				r = (int)interpreter->builtin_arguments[3]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[3]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[3]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_line"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x1, y1, x2, y2, r, g, b;
				x1 = (int)interpreter->builtin_arguments[1]->i;
				y1 = (int)interpreter->builtin_arguments[2]->i;
				x2 = (int)interpreter->builtin_arguments[3]->i;
				y2 = (int)interpreter->builtin_arguments[4]->i;
				r = (int)interpreter->builtin_arguments[5]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[5]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[5]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_line(x1, y1, x2, y2, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_rect"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				width = (int)interpreter->builtin_arguments[3]->i;
				height = (int)interpreter->builtin_arguments[4]->i;
				r = (int)interpreter->builtin_arguments[5]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[5]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[5]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_rect"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				width = (int)interpreter->builtin_arguments[3]->i;
				height = (int)interpreter->builtin_arguments[4]->i;
				r = (int)interpreter->builtin_arguments[5]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[5]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[5]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->fill_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_circle"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->builtin_arguments[1]->i;
				yc = (int)interpreter->builtin_arguments[2]->i;
				radius = (int)interpreter->builtin_arguments[3]->i;
				r = (int)interpreter->builtin_arguments[4]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[4]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[4]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->draw_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_circle"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->builtin_arguments[1]->i;
				yc = (int)interpreter->builtin_arguments[2]->i;
				radius = (int)interpreter->builtin_arguments[3]->i;
				r = (int)interpreter->builtin_arguments[4]->str->second["r"]->i;
				g = (int)interpreter->builtin_arguments[4]->str->second["g"]->i;
				b = (int)interpreter->builtin_arguments[4]->str->second["b"]->i;
				windows[win->str->second[INSTANCE_ID_NAME]->i]->fill_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				windows[win->str->second[INSTANCE_ID_NAME]->i]->update();
			}
		}
	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->curr_type)) {
			if (windows[win->str->second[INSTANCE_ID_NAME]->i]) {
				windows[win->str->second[INSTANCE_ID_NAME]->i]->~Window();
				windows[win->str->second[INSTANCE_ID_NAME]->i] = nullptr;
				win->set_null();
			}
		}
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
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
