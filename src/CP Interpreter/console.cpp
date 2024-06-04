#include <cwchar>
#include <windows.h>

#include "console.hpp"

using namespace modules;

Console::Console() {}

void Console::register_functions(visitor::Interpreter* interpreter) {

	interpreter->builtin_functions["set_console_color"] = [this, interpreter]() {
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, interpreter->builtin_arguments[0]->i * 16 | interpreter->builtin_arguments[1]->i);
	};

	interpreter->builtin_functions["set_console_cursor_position"] = [this, interpreter]() {
		COORD pos = { interpreter->builtin_arguments[0]->i, interpreter->builtin_arguments[1]->i };
		HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleCursorPosition(output, pos);
	};

	interpreter->builtin_functions["set_console_font"] = [this, interpreter]() {
		auto pfontname = std::wstring(interpreter->builtin_arguments[0]->s.begin(), interpreter->builtin_arguments[0]->s.end());
		int pwidth = interpreter->builtin_arguments[1]->i;
		int pheight = interpreter->builtin_arguments[2]->i;

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
