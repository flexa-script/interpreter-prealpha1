#include <cwchar>
#include <windows.h>

#include "console.hpp"

#include "interpreter.hpp"
#include "semantic_analysis.hpp"

using namespace modules;

Console::Console() {}

Console::~Console() = default;

void Console::register_functions(visitor::SemanticAnalyser* visitor) {
	visitor->builtin_functions["set_console_color"] = nullptr;
	visitor->builtin_functions["set_console_cursor_position"] = nullptr;
	visitor->builtin_functions["set_console_font"] = nullptr;
}

void Console::register_functions(visitor::Interpreter* visitor) {

	visitor->builtin_functions["set_console_color"] = [this, visitor]() {
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, visitor->builtin_arguments[0]->get_i() * 16 | visitor->builtin_arguments[1]->get_i());
	};

	visitor->builtin_functions["set_console_cursor_position"] = [this, visitor]() {
		COORD pos = { visitor->builtin_arguments[0]->get_i(), visitor->builtin_arguments[1]->get_i() };
		HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleCursorPosition(output, pos);
	};

	visitor->builtin_functions["set_console_font"] = [this, visitor]() {
		auto pfontname = std::wstring(visitor->builtin_arguments[0]->get_s().begin(), visitor->builtin_arguments[0]->get_s().end());
		int pwidth = visitor->builtin_arguments[1]->get_i();
		int pheight = visitor->builtin_arguments[2]->get_i();

		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = pwidth;
		cfi.dwFontSize.Y = pheight;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

#pragma warning(suppress : 4996)
		std::wcscpy(cfi.FaceName, pfontname.c_str());
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	};

}
