#include <iostream>

#include "graphics.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;
using namespace visitor;

Graphics::Graphics() {}

Graphics::~Graphics() = default;

void Graphics::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["create_window"] = nullptr;
	visitor->builtin_functions["get_current_width"] = nullptr;
	visitor->builtin_functions["get_current_height"] = nullptr;
	visitor->builtin_functions["get_current_content_width"] = nullptr;
	visitor->builtin_functions["get_current_content_height"] = nullptr;
	visitor->builtin_functions["clear_screen"] = nullptr;
	visitor->builtin_functions["get_text_size"] = nullptr;
	visitor->builtin_functions["draw_text"] = nullptr;
	visitor->builtin_functions["draw_pixel"] = nullptr;
	visitor->builtin_functions["draw_line"] = nullptr;
	visitor->builtin_functions["draw_rect"] = nullptr;
	visitor->builtin_functions["fill_rect"] = nullptr;
	visitor->builtin_functions["draw_circle"] = nullptr;
	visitor->builtin_functions["fill_circle"] = nullptr;
	visitor->builtin_functions["create_font"] = nullptr;
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

		cp_struct str = cp_struct();
		str["title"] = new Value(visitor->builtin_arguments[0]);
		str["width"] = new Value(visitor->builtin_arguments[1]);
		str["height"] = new Value(visitor->builtin_arguments[2]);
		str["content_width"] = new Value(cp_int(visitor->builtin_arguments[1]->get_i()) - 16);
		str["content_height"] = new Value(cp_int(visitor->builtin_arguments[2]->get_i() - 39));

		// create a new window graphic engine
		str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		str[INSTANCE_ID_NAME]->set(cp_int(new axe::Window()));

		// initialize window graphic engine and return value
		auto res = ((axe::Window*)str[INSTANCE_ID_NAME]->get_i())->initialize(
			str["title"]->get_s(),
			(int)str["width"]->get_i(),
			(int)str["height"]->get_i()
		);

		win->set(str, "Window", "cp");

		if (!res) {
			win->set_null();
		}

		visitor->current_expression_value = win;
		};

	visitor->builtin_functions["clear_screen"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int r, g, b;
		r = (int)visitor->builtin_arguments[1]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[1]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[1]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->clear_screen(RGB(r, g, b));
		};

	visitor->builtin_functions["get_current_width"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		visitor->current_expression_value=new Value(cp_int(((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->get_current_width()));
		};

	visitor->builtin_functions["get_current_height"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		visitor->current_expression_value = new Value(cp_int(((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->get_current_height()));
		};

	visitor->builtin_functions["get_current_content_width"] = [this, visitor]() {
		visitor->builtin_functions["get_current_width"]();
		visitor->current_expression_value->set(cp_int(visitor->current_expression_value->get_i() - 16));
		};

	visitor->builtin_functions["get_current_content_height"] = [this, visitor]() {
		visitor->builtin_functions["get_current_height"]();
		visitor->current_expression_value->set(cp_int(visitor->current_expression_value->get_i() - 39));
		};

	visitor->builtin_functions["draw_pixel"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, r, g, b;
		x = (int)visitor->builtin_arguments[1]->get_i();
		y = (int)visitor->builtin_arguments[2]->get_i();
		r = (int)visitor->builtin_arguments[3]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[3]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[3]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->draw_pixel(x, y, RGB(r, g, b));
		};

	visitor->builtin_functions["draw_line"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!win->get_str()[INSTANCE_ID_NAME]->get_i()) {
			throw std::runtime_error("Window is corrupted");
		}
		int x1, y1, x2, y2, r, g, b;
		x1 = (int)visitor->builtin_arguments[1]->get_i();
		y1 = (int)visitor->builtin_arguments[2]->get_i();
		x2 = (int)visitor->builtin_arguments[3]->get_i();
		y2 = (int)visitor->builtin_arguments[4]->get_i();
		r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->draw_line(x1, y1, x2, y2, RGB(r, g, b));
		};

	visitor->builtin_functions["draw_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, width, height, r, g, b;
		x = (int)visitor->builtin_arguments[1]->get_i();
		y = (int)visitor->builtin_arguments[2]->get_i();
		width = (int)visitor->builtin_arguments[3]->get_i();
		height = (int)visitor->builtin_arguments[4]->get_i();
		r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->draw_rect(x, y, width, height, RGB(r, g, b));

		};

	visitor->builtin_functions["fill_rect"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x, y, width, height, r, g, b;
		x = (int)visitor->builtin_arguments[1]->get_i();
		y = (int)visitor->builtin_arguments[2]->get_i();
		width = (int)visitor->builtin_arguments[3]->get_i();
		height = (int)visitor->builtin_arguments[4]->get_i();
		r = (int)visitor->builtin_arguments[5]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[5]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[5]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->fill_rect(x, y, width, height, RGB(r, g, b));

		};

	visitor->builtin_functions["draw_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int xc, yc, radius, r, g, b;
		xc = (int)visitor->builtin_arguments[1]->get_i();
		yc = (int)visitor->builtin_arguments[2]->get_i();
		radius = (int)visitor->builtin_arguments[3]->get_i();
		r = (int)visitor->builtin_arguments[4]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[4]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[4]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->draw_circle(xc, yc, radius, RGB(r, g, b));

		};

	visitor->builtin_functions["fill_circle"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int xc, yc, radius, r, g, b;
		xc = (int)visitor->builtin_arguments[1]->get_i();
		yc = (int)visitor->builtin_arguments[2]->get_i();
		radius = (int)visitor->builtin_arguments[3]->get_i();
		r = (int)visitor->builtin_arguments[4]->get_str()["r"]->get_i();
		g = (int)visitor->builtin_arguments[4]->get_str()["g"]->get_i();
		b = (int)visitor->builtin_arguments[4]->get_str()["b"]->get_i();
		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->fill_circle(xc, yc, radius, RGB(r, g, b));

		};

	visitor->builtin_functions["create_font"] = [this, visitor]() {
		// initialize image struct values
		Value* font_value = new Value(parser::Type::T_STRUCT);

		auto str = cp_struct();
		str["size"] = new Value(visitor->builtin_arguments[0]);
		str["name"] = new Value(visitor->builtin_arguments[1]);
		str["weight"] = new Value(visitor->builtin_arguments[2]);
		str["italic"] = new Value(visitor->builtin_arguments[3]);
		str["underline"] = new Value(visitor->builtin_arguments[4]);
		str["strike"] = new Value(visitor->builtin_arguments[5]);
		str["orientation"] = new Value(visitor->builtin_arguments[6]);

		auto font = axe::Font::create_font(
			str["size"]->get_i(),
			str["name"]->get_s(),
			str["weight"]->get_i(),
			str["italic"]->get_b(),
			str["underline"]->get_b(),
			str["strike"]->get_b(),
			str["orientation"]->get_i()
		);
		if (!font) {
			throw std::runtime_error("there was an error creating font");
		}
		str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		str[INSTANCE_ID_NAME]->set(cp_int(font));

		font_value->set(str, "Font", "cp");

		visitor->current_expression_value = font_value;
		};

	visitor->builtin_functions["draw_text"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		int x = (int)visitor->builtin_arguments[1]->get_i();
		int y = (int)visitor->builtin_arguments[2]->get_i();
		std::string text = visitor->builtin_arguments[3]->get_s();
		int r = (int)visitor->builtin_arguments[4]->get_str()["r"]->get_i();
		int g = (int)visitor->builtin_arguments[4]->get_str()["g"]->get_i();
		int b = (int)visitor->builtin_arguments[4]->get_str()["b"]->get_i();

		Value* font_value = visitor->builtin_arguments[5];
		if (parser::is_void(font_value->type)) {
			throw std::exception("font is null");
		}
		axe::Font* font = (axe::Font*)font_value->get_str()[INSTANCE_ID_NAME]->get_i();
		if (!font) {
			throw std::runtime_error("there was an error handling font");
		}

		((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->draw_text(x, y, text, RGB(r, g, b), font);
		};

	visitor->builtin_functions["get_text_size"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::runtime_error("Window is null");
		}
		if (!((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
			throw std::runtime_error("Window is corrupted");
		}
		std::string text = visitor->builtin_arguments[1]->get_s();
		Value* font_value = visitor->builtin_arguments[2];
		if (parser::is_void(font_value->type)) {
			throw std::exception("font is null");
		}
		axe::Font* font = (axe::Font*)font_value->get_str()[INSTANCE_ID_NAME]->get_i();
		if (!font) {
			throw std::runtime_error("there was an error handling font");
		}

		auto point = ((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->get_text_size(text, font);

		cp_struct str = cp_struct();
		str["width"] = new Value(cp_int(point.cx * 2 * 0.905));
		str["height"] = new Value(cp_int(point.cy * 2 * 0.875));

		Value* res = new Value(str, "Size", "cp");

		visitor->current_expression_value = res;
		};

	visitor->builtin_functions["load_image"] = [this, visitor]() {
		// initialize image struct values
		Value* img = new Value(parser::Type::T_STRUCT);

		auto str = cp_struct();
		str["path"] = new Value(visitor->builtin_arguments[0]);

		// loads image
		auto image = axe::Image::load_image(str["path"]->get_s());
		if (!image) {
			throw std::runtime_error("there was an error loading image");
		}
		str[INSTANCE_ID_NAME] = new Value(parser::Type::T_INT);
		str[INSTANCE_ID_NAME]->set(cp_int(image));

		str["width"] = new Value(parser::Type::T_INT);
		str["width"]->set(cp_int(image->width));
		str["height"] = new Value(parser::Type::T_INT);
		str["height"]->set(cp_int(image->height));

		img->set(str, "Image", "cp");

		visitor->current_expression_value = img;
		};

	visitor->builtin_functions["draw_image"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (parser::is_void(win->type)) {
			throw std::exception("window is null");
		}
		auto window = ((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i());
		if (!window) {
			throw std::runtime_error("there was an error handling window");
		}
		Value* img = visitor->builtin_arguments[1];
		if (parser::is_void(img->type)) {
			throw std::exception("window is null");
		}
		auto image = ((axe::Image*)img->get_str()[INSTANCE_ID_NAME]->get_i());
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
			if (((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
				((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->update();
			}
		}
		};

	visitor->builtin_functions["destroy_window"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		if (!parser::is_void(win->type)) {
			if (((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
				((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->~Window();
				win->set_null();
			}
		}
		};

	visitor->builtin_functions["is_quit"] = [this, visitor]() {
		Value* win = visitor->builtin_arguments[0];
		auto val = new Value(parser::Type::T_BOOL);
		if (!parser::is_void(win->type)) {
			if (((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())) {
				val->set(cp_bool(((axe::Window*)win->get_str()[INSTANCE_ID_NAME]->get_i())->is_quit()));
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
