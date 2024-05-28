#include "cpgraphics.hpp"

using namespace modules;


CPGraphics::CPGraphics() : graphic_engine(axe::Graphics()) {}

void CPGraphics::register_interpreter_functions(visitor::Interpreter* interpreter) {
	interpreter->builtin_functions["initialize"] = [this, interpreter]() {
		auto val = Value(parser::Type::T_BOOL);

		val.b = graphic_engine.initialize(
			(const wchar_t*)interpreter->last_function_arguments[0].second->s.c_str(),
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

	interpreter->builtin_functions["is_quit"] = [this, interpreter]() {
		auto val = Value(parser::Type::T_BOOL);
		val.b = graphic_engine.is_quit();
		interpreter->current_expression_value = val;
	};
}

void CPGraphics::register_semantic_functions(visitor::SemanticAnalyser* semantic_analyser) {
	semantic_analyser->builtin_functions["initialize"] = [semantic_analyser]() {
		if (semantic_analyser->signature.size() != 3) {
			throw std::runtime_error("");
		}
		if (!is_string(semantic_analyser->signature[0].type)) {
			throw std::runtime_error("'title' must be string");
		}
		if (!is_int(semantic_analyser->signature[1].type)) {
			throw std::runtime_error("'width' must be int");
		}
		if (!is_int(semantic_analyser->signature[2].type)) {
			throw std::runtime_error("'height' must be int");
		}
	};

	semantic_analyser->builtin_functions["clear_screen"] = [semantic_analyser]() {
		if (semantic_analyser->signature.size() != 1) {
			throw std::runtime_error("");
		}
		if (!is_struct(semantic_analyser->signature[0].type)
			|| semantic_analyser->signature[0].type_name != "Color") {
			throw std::runtime_error("'color' must be struct<Color>");
		}
	};

	semantic_analyser->builtin_functions["draw_pixel"] = [semantic_analyser]() {
		if (semantic_analyser->signature.size() != 3) {
			throw std::runtime_error("");
		}
		if (!is_int(semantic_analyser->signature[0].type)) {
			throw std::runtime_error("'x' must be int");
		}
		if (!is_int(semantic_analyser->signature[1].type)) {
			throw std::runtime_error("'y' must be int");
		}
		if (!is_struct(semantic_analyser->signature[0].type)
			|| semantic_analyser->signature[0].type_name != "Color") {
			throw std::runtime_error("'color' must be struct<Color>");
		}
	};

	semantic_analyser->builtin_functions["update"] = [semantic_analyser]() {};

	semantic_analyser->builtin_functions["is_quit"] = [semantic_analyser]() {};
}
