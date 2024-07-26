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

void Graphics::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["create_window"] = [this, interpreter]() {
		// initialize window struct values
		Value* win = new Value(parser::Type::T_STRUCT);
		std::get<0>(*win->str) = "cp";
		std::get<1>(*win->str) = "Window";
		std::get<2>(*win->str)["title"] = new Value(interpreter->builtin_arguments[0]);
		std::get<2>(*win->str)["width"] = new Value(interpreter->builtin_arguments[1]);
		std::get<2>(*win->str)["height"] = new Value(interpreter->builtin_arguments[2]);

		// create a new window graphic engine
		windows.push_back(new axe::Window());
		std::get<2>(*win->str)[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		std::get<2>(*win->str)[INSTANCE_ID_NAME]->i = windows.size() - 1;

		// initialize window graphic engine and return value
		auto res = windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->initialize(
			std::get<2>(*win->str)["title"]->s,
			(int)std::get<2>(*win->str)["width"]->i,
			(int)std::get<2>(*win->str)["height"]->i
		);
		if (!res) {
			win->set_null();
		}
		interpreter->current_expression_value = win;
	};

	interpreter->builtin_functions["clear_screen"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int r, g, b;
				r = (int)std::get<2>(*interpreter->builtin_arguments[1]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[1]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[1]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->clear_screen(RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int x, y, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[3]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[3]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[3]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->draw_pixel(x, y, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_line"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int x1, y1, x2, y2, r, g, b;
				x1 = (int)interpreter->builtin_arguments[1]->i;
				y1 = (int)interpreter->builtin_arguments[2]->i;
				x2 = (int)interpreter->builtin_arguments[3]->i;
				y2 = (int)interpreter->builtin_arguments[4]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->draw_line(x1, y1, x2, y2, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_rect"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				width = (int)interpreter->builtin_arguments[3]->i;
				height = (int)interpreter->builtin_arguments[4]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->draw_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_rect"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int x, y, width, height, r, g, b;
				x = (int)interpreter->builtin_arguments[1]->i;
				y = (int)interpreter->builtin_arguments[2]->i;
				width = (int)interpreter->builtin_arguments[3]->i;
				height = (int)interpreter->builtin_arguments[4]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[5]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->fill_rect(x, y, width, height, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["draw_circle"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->builtin_arguments[1]->i;
				yc = (int)interpreter->builtin_arguments[2]->i;
				radius = (int)interpreter->builtin_arguments[3]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->draw_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["fill_circle"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				int xc, yc, radius, r, g, b;
				xc = (int)interpreter->builtin_arguments[1]->i;
				yc = (int)interpreter->builtin_arguments[2]->i;
				radius = (int)interpreter->builtin_arguments[3]->i;
				r = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["r"]->i;
				g = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["g"]->i;
				b = (int)std::get<2>(*interpreter->builtin_arguments[4]->str)["b"]->i;
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->fill_circle(xc, yc, radius, RGB(r, g, b));
			}
		}
	};

	interpreter->builtin_functions["load_image"] = [this, interpreter]() {
		// initialize image struct values
		Value* img = new Value(parser::Type::T_STRUCT);
		std::get<0>(*img->str) = "cp";
		std::get<1>(*img->str) = "Image";
		std::get<2>(*img->str)["path"] = new Value(interpreter->builtin_arguments[0]);

		// loads image
		auto image = axe::Image::load_image(std::get<2>(*img->str)["path"]->s);
		if (!image) {
			throw std::runtime_error("there was an error loading image");
		}
		images.push_back(image);
		std::get<2>(*img->str)[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		std::get<2>(*img->str)[INSTANCE_ID_NAME]->i = images.size() - 1;

		std::get<2>(*img->str)["width"] = new Value(parser::Type::T_INT);
		std::get<2>(*img->str)["width"]->i = image->width;
		std::get<2>(*img->str)["height"] = new Value(parser::Type::T_INT);
		std::get<2>(*img->str)["height"]->i = image->height;

		interpreter->current_expression_value = img;
	};

	interpreter->builtin_functions["draw_image"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (parser::is_void(win->type) ) {
			throw std::exception("window is null");
		}
		auto window = windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i];
		if (!window) {
			throw std::runtime_error("there was an error handling window");
		}
		Value* img = interpreter->builtin_arguments[1];
		if (parser::is_void(img->type)) {
			throw std::exception("window is null");
		}
		auto image = images[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i];
		if (!image) {
			throw std::runtime_error("there was an error handling image");
		}
		int x = (int)interpreter->builtin_arguments[2]->i;
		int y = (int)interpreter->builtin_arguments[3]->i;
		window->draw_image(image, x, y);
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->update();
			}
		}
	};

	interpreter->builtin_functions["destroy_window"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->~Window();
				windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i] = nullptr;
				win->set_null();
			}
		}
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		Value* win = interpreter->builtin_arguments[0];
		auto val = new Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->type)) {
			if (windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]) {
				val->b = windows[std::get<2>(*win->str)[INSTANCE_ID_NAME]->i]->is_quit();
			}
			else {
				val->b = true;
			}
		}
		else {
			val->b = true;
		}
		interpreter->current_expression_value = val;
	};
}
