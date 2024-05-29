#include "cpgraphics.hpp"

using namespace modules;


CPGraphics::CPGraphics() : graphic_engine(axe::Graphics()) {}

void CPGraphics::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		auto val = Value(parser::Type::T_BOOL);

		auto str = interpreter->last_function_arguments[0].second->s;
		auto wstr = std::wstring(str.begin(), str.end());

		val.b = graphic_engine.initialize(
			wstr.c_str(),
			(int)interpreter->last_function_arguments[1].second->i,
			(int)interpreter->last_function_arguments[2].second->i
		);

		interpreter->current_expression_value = val;
	};

	interpreter->builtin_functions["clear_screen"] = [this, interpreter]() {
		int r, g, b;
		r = (int)interpreter->last_function_arguments[0].second->str->second["r"]->i;
		g = (int)interpreter->last_function_arguments[0].second->str->second["g"]->i;
		b = (int)interpreter->last_function_arguments[0].second->str->second["b"]->i;
		graphic_engine.clear_screen(RGB(r, g, b));
	};

	interpreter->builtin_functions["draw_pixel"] = [this, interpreter]() {
		int x, y, r, g, b;
		x = (int)interpreter->last_function_arguments[0].second->i;
		y = (int)interpreter->last_function_arguments[1].second->i;
		r = (int)interpreter->last_function_arguments[2].second->str->second["r"]->i;
		g = (int)interpreter->last_function_arguments[2].second->str->second["g"]->i;
		b = (int)interpreter->last_function_arguments[2].second->str->second["b"]->i;
		graphic_engine.draw_pixel(x, y, RGB(r, g, b));
	};

	interpreter->builtin_functions["update"] = [this, interpreter]() {
		graphic_engine.update();
	};

	interpreter->builtin_functions["destroy"] = [this, interpreter]() {
		graphic_engine.~Graphics();
	};

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		auto val = Value(parser::Type::T_BOOL);
		val.b = graphic_engine.is_quit();
		interpreter->current_expression_value = val;
	};
}
