#include <Windows.h>

#include "input.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;

Input::Input() {}

Input::~Input() = default;

void Input::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["update_key_states"] = nullptr;
	visitor->builtin_functions["is_key_pressed"] = nullptr;
	visitor->builtin_functions["is_key_released"] = nullptr;
	visitor->builtin_functions["get_mouse_position"] = nullptr;
	visitor->builtin_functions["set_mouse_position"] = nullptr;
	visitor->builtin_functions["is_mouse_button_pressed"] = nullptr;
}

void Input::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["update_key_states"] = [this, visitor]() {
		previous_key_state = current_key_state;

		for (int i = 0; i < KEY_COUNT; ++i) {
			current_key_state[i] = GetAsyncKeyState(i) & 0x8000;
		}

		};

	visitor->builtin_functions["is_key_pressed"] = [this, visitor]() {
		int key = visitor->builtin_arguments[0]->get_i();
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(cp_bool(current_key_state[key])));

		};

	visitor->builtin_functions["is_key_released"] = [this, visitor]() {
		int key = visitor->builtin_arguments[0]->get_i();
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(cp_bool(previous_key_state[key] && !current_key_state[key])));

		};

	visitor->builtin_functions["get_mouse_position"] = [this, visitor]() {
		POINT point;
		GetCursorPos(&point);

		cp_struct str = cp_struct();
		str["x"] = visitor->alocate_value(new RuntimeValue(cp_int(point.x * 2 * 0.905)));
		str["y"] = visitor->alocate_value(new RuntimeValue(cp_int(point.y * 2 * 0.875)));
		RuntimeValue* res = visitor->alocate_value(new RuntimeValue(str, "Point", "cp"));

		visitor->current_expression_value = res;

		};

	visitor->builtin_functions["set_mouse_position"] = [this, visitor]() {
		int x = visitor->builtin_arguments[0]->get_i();
		int y = visitor->builtin_arguments[1]->get_i();
		SetCursorPos(x, y);

		};

	visitor->builtin_functions["is_mouse_button_pressed"] = [this, visitor]() {
		int button = visitor->builtin_arguments[0]->get_i();
		bool is_pressed = (GetAsyncKeyState(button) & 0x8000) != 0;
		visitor->current_expression_value = visitor->alocate_value(new RuntimeValue(cp_bool(is_pressed)));

		};
}
