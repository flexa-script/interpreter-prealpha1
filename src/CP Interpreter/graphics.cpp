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
		win->type_name_space = "cp";
		win->type_name = "Window";

		cp_struct str = cp_struct();
		str["title"] = new Value(visitor->builtin_arguments[0]);
		str["width"] = new Value(visitor->builtin_arguments[1]);
		str["height"] = new Value(visitor->builtin_arguments[2]);

		// create a new window graphic engine
		windows.push_back(new axe::Window());
		str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		str[INSTANCE_ID_NAME]->set(cp_int(windows.size() - 1));

		// initialize window graphic engine and return value
		auto res = windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->initialize(
			str["title"]->get_s(),
			(int)str["width"]->get_i(),
			(int)str["height"]->get_i()
		);
		if (!res) {
			win->set_null();
		}

		win->set(str);

		visitor->current_expression_value = win;
	};

	visitor->builtin_functions["clear_screen"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int r, g, b;
				r = (int)visitor->builtin_arguments[1]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[1]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[1]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->clear_screen(RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_pixel"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int x, y, r, g, b;
				x = (int)visitor->builtin_arguments[1]->get_i();
				y = (int)visitor->builtin_arguments[2]->get_i();
				r = (int)visitor->builtin_arguments[3]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[3]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[3]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_line"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int x1, y1, x2, y2, r, g, b;
				x1 = (int)visitor->builtin_arguments[1]->get_i();
				y1 = (int)visitor->builtin_arguments[2]->get_i();
				x2 = (int)visitor->builtin_arguments[3]->get_i();
				y2 = (int)visitor->builtin_arguments[4]->get_i();
				r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->draw_line(x1, y1, x2, y2, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int x, y, width, height, r, g, b;
				x = (int)visitor->builtin_arguments[1]->get_i();
				y = (int)visitor->builtin_arguments[2]->get_i();
				width = (int)visitor->builtin_arguments[3]->get_i();
				height = (int)visitor->builtin_arguments[4]->get_i();
				r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->draw_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["fill_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int x, y, width, height, r, g, b;
				x = (int)visitor->builtin_arguments[1]->get_i();
				y = (int)visitor->builtin_arguments[2]->get_i();
				width = (int)visitor->builtin_arguments[3]->get_i();
				height = (int)visitor->builtin_arguments[4]->get_i();
				r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->fill_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["draw_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int xc, yc, radius, r, g, b;
				xc = (int)visitor->builtin_arguments[1]->get_i();
				yc = (int)visitor->builtin_arguments[2]->get_i();
				radius = (int)visitor->builtin_arguments[3]->get_i();
				r = (int)visitor->builtin_arguments[4]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[4]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[4]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->draw_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["fill_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				int xc, yc, radius, r, g, b;
				xc = (int)visitor->builtin_arguments[1]->get_i();
				yc = (int)visitor->builtin_arguments[2]->get_i();
				radius = (int)visitor->builtin_arguments[3]->get_i();
				r = (int)visitor->builtin_arguments[4]->get_str()["r"]->get_i();
				g = (int)visitor->builtin_arguments[4]->get_str()["g"]->get_i();
				b = (int)visitor->builtin_arguments[4]->get_str()["b"]->get_i();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->fill_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	visitor->builtin_functions["load_image"] = [this, visitor]() {
		// initialize image struct values
		Value* img = new Value(parser::Type::T_STRUCT);
		img->get_str()["path"] = new Value(visitor->builtin_arguments[0]);
		img->type_name_space = "cp";
		img->type_name= "Image";

		// loads image
		auto image = axe::Image::load_image(img->get_str()["path"]->get_s());
		if (!image) {
			throw std::runtime_error("there was an error loading image");
		}
		images.push_back(image);
		img->get_str()[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		img->get_str()[INSTANCE_ID_NAME]->set(cp_int(images.size() - 1));

		img->get_str()["width"] = new Value(parser::Type::T_INT);
		img->get_str()["width"]->set(cp_int(image->width));
		img->get_str()["height"] = new Value(parser::Type::T_INT);
		img->get_str()["height"]->set(cp_int(image->height));

		visitor->current_expression_value = img;
	};

	visitor->builtin_functions["draw_image"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type) ) {
			throw std::exception("window is null");
		}
		auto window = windows[win->get_str()[INSTANCE_ID_NAME]->get_i()];
		if (!window) {
			throw std::runtime_error("there was an error handling window");
		}
		Value* img = visitor->builtin_arguments[1];
		if (parser::is_void(img->type)) {
			throw std::exception("window is null");
		}
		auto image = images[win->get_str()[INSTANCE_ID_NAME]->get_i()];
		if (!image) {
			throw std::runtime_error("there was an error handling image");
		}
		int x = (int)visitor->builtin_arguments[2]->get_i();
		int y = (int)visitor->builtin_arguments[3]->get_i();
		window->draw_image(image, x, y);
	};

	visitor->builtin_functions["update"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->update();
			}
		}
	};

	visitor->builtin_functions["destroy_window"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->~Window();
				windows[win->get_str()[INSTANCE_ID_NAME]->get_i()] = nullptr;
				win->set_null();
			}
		}
	};

	visitor->builtin_functions["is_quit"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		auto val = new Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->type)) {
			if (windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]) {
				val->set(cp_bool(windows[win->get_str()[INSTANCE_ID_NAME]->get_i()]->is_quit()));
			}
			else {
				val->set(cp_bool(true));
			}
		}
		else {
			val->set(cp_bool(true));
		}
		visitor->current_expression_value = val;
	};
}
