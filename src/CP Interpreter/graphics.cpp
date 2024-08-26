#include "graphics.hpp"
#include <iostream>

using namespace modules;
using namespace visitor;

Graphics::Graphics() {}

Graphics::~Graphics() {
	for (auto& val : images) {
		delete val;
	}
	images.clear();
	for (auto& val : windows) {
		delete val;
	}
	windows.clear();
}

void Graphics::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_window"] = nullptr;
	visitor->builtin_functions["clear_screen"] = nullptr;
	visitor->builtin_functions["draw_pixel"] = nullptr;
	visitor->builtin_functions["draw_line"] = nullptr;
	visitor->builtin_functions["draw_rect"] = nullptr;
	visitor->builtin_functions["fill_rect"] = nullptr;
	visitor->builtin_functions["draw_circle"] = nullptr;
	visitor->builtin_functions["fill_circle"] = nullptr;
	visitor->builtin_functions["load_image"] = nullptr;
	visitor->builtin_functions["draw_image"] = nullptr;
	visitor->builtin_functions["update"] = nullptr;
	visitor->builtin_functions["destroy_window"] = nullptr;
	visitor->builtin_functions["is_quit"] = nullptr;
}

void Graphics::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["create_window"] = [this, visitor]() {
		// initialize window struct values
		Value* win = new Value(parser::Type::T_STRUCT);
		win->str["title"] = new Value(visitor->builtin_arguments[0]);
		win->str["width"] = new Value(visitor->builtin_arguments[1]);
		win->str["height"] = new Value(visitor->builtin_arguments[2]);
		win->type_name_space = "cp";
		win->type_name = "Window";

		// create a new window graphic engine
		windows.push_back(new axe::Window());
		win->str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		win->str[INSTANCE_ID_NAME]->i = windows.size() - 1;

		// initialize window graphic engine and return value
		auto res = windows[win->str[INSTANCE_ID_NAME]->i]->initialize(
			win->str["title"]->s,
			(int)win->str["width"]->i,
			(int)win->str["height"]->i
		);
		if (!res) {
			win->set_null();
		}
		visitor->current_expression_value = win;
	};

	visitor->builtin_functions["clear_screen"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int r, g, b;
				r = (int)visitor->builtin_arguments[1]->str["r"]->i;
				g = (int)visitor->builtin_arguments[1]->str["g"]->i;
				b = (int)visitor->builtin_arguments[1]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->clear_screen(RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_pixel"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int x, y, r, g, b;
				x = (int)visitor->builtin_arguments[1]->i;
				y = (int)visitor->builtin_arguments[2]->i;
				r = (int)visitor->builtin_arguments[3]->str["r"]->i;
				g = (int)visitor->builtin_arguments[3]->str["g"]->i;
				b = (int)visitor->builtin_arguments[3]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_line"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int x1, y1, x2, y2, r, g, b;
				x1 = (int)visitor->builtin_arguments[1]->i;
				y1 = (int)visitor->builtin_arguments[2]->i;
				x2 = (int)visitor->builtin_arguments[3]->i;
				y2 = (int)visitor->builtin_arguments[4]->i;
				r = (int)visitor->builtin_arguments[5]->str["r"]->i;
				g = (int)visitor->builtin_arguments[5]->str["g"]->i;
				b = (int)visitor->builtin_arguments[5]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->draw_line(x1, y1, x2, y2, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)visitor->builtin_arguments[1]->i;
				y = (int)visitor->builtin_arguments[2]->i;
				width = (int)visitor->builtin_arguments[3]->i;
				height = (int)visitor->builtin_arguments[4]->i;
				r = (int)visitor->builtin_arguments[5]->str["r"]->i;
				g = (int)visitor->builtin_arguments[5]->str["g"]->i;
				b = (int)visitor->builtin_arguments[5]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->draw_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["fill_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)visitor->builtin_arguments[1]->i;
				y = (int)visitor->builtin_arguments[2]->i;
				width = (int)visitor->builtin_arguments[3]->i;
				height = (int)visitor->builtin_arguments[4]->i;
				r = (int)visitor->builtin_arguments[5]->str["r"]->i;
				g = (int)visitor->builtin_arguments[5]->str["g"]->i;
				b = (int)visitor->builtin_arguments[5]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->fill_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)visitor->builtin_arguments[1]->i;
				yc = (int)visitor->builtin_arguments[2]->i;
				radius = (int)visitor->builtin_arguments[3]->i;
				r = (int)visitor->builtin_arguments[4]->str["r"]->i;
				g = (int)visitor->builtin_arguments[4]->str["g"]->i;
				b = (int)visitor->builtin_arguments[4]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->draw_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["fill_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)visitor->builtin_arguments[1]->i;
				yc = (int)visitor->builtin_arguments[2]->i;
				radius = (int)visitor->builtin_arguments[3]->i;
				r = (int)visitor->builtin_arguments[4]->str["r"]->i;
				g = (int)visitor->builtin_arguments[4]->str["g"]->i;
				b = (int)visitor->builtin_arguments[4]->str["b"]->i;
				windows[win->str[INSTANCE_ID_NAME]->i]->fill_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["load_image"] = [this, visitor]() {
		// initialize image struct values
		Value* img = new Value(parser::Type::T_STRUCT);
		img->str["path"] = new Value(visitor->builtin_arguments[0]);
		img->type_name_space = "cp";
		img->type_name= "Image";

		// loads image
		auto image = axe::Image::load_image(img->str["path"]->s);
		if (!image) {
			throw std::runtime_error("there was an error loading image");
		}
		images.push_back(image);
		img->str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		img->str[INSTANCE_ID_NAME]->i = images.size() - 1;

		img->str["width"] = new Value(parser::Type::T_INT);
		img->str["width"]->i = image->width;
		img->str["height"] = new Value(parser::Type::T_INT);
		img->str["height"]->i = image->height;

		visitor->current_expression_value = img;
	};

	visitor->builtin_functions["draw_image"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type) ) {
			throw std::exception("window is null");
		}
		auto window = windows[win->str[INSTANCE_ID_NAME]->i];
		if (!window) {
			throw std::runtime_error("there was an error handling window");
		}
		Value* img = visitor->builtin_arguments[1];
		if (parser::is_void(img->type)) {
			throw std::exception("window is null");
		}
		auto image = images[win->str[INSTANCE_ID_NAME]->i];
		if (!image) {
			throw std::runtime_error("there was an error handling image");
		}
		int x = (int)visitor->builtin_arguments[2]->i;
		int y = (int)visitor->builtin_arguments[3]->i;
		window->draw_image(image, x, y);
	};

	visitor->builtin_functions["update"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				windows[win->str[INSTANCE_ID_NAME]->i]->update();
			}
		}
	};

	visitor->builtin_functions["destroy_window"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				windows[win->str[INSTANCE_ID_NAME]->i]->~Window();
				windows[win->str[INSTANCE_ID_NAME]->i] = nullptr;
				win->set_null();
			}
		}
	};

	visitor->builtin_functions["is_quit"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		auto val = new Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->type)) {
			if (windows[win->str[INSTANCE_ID_NAME]->i]) {
				val->b = windows[win->str[INSTANCE_ID_NAME]->i]->is_quit();
			}
			else {
				val->b = true;
			}
		}
		else {
			val->b = true;
		}
		visitor->current_expression_value = val;
	};
}
