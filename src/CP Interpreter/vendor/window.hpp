/**
 * MIT License
 * Copyright (c) 2024 Carlos Machado
 * v1.0.0
 */

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <windows.h>
#include <map>

namespace axe {

	class Window {
	private:
		HWND hwnd;
		HDC hdc;
		HBITMAP hbm_back_buffer;
		HDC hdc_back_buffer;
		int screen_width, screen_height;
		MSG msg = { 0 };
		bool quit = false;

		static std::map<HWND, Window*> hwnd_map;

	public:
		Window();
		~Window();

		bool initialize(const wchar_t* title, int width, int height);
		void clear_screen(COLORREF color);
		void draw_pixel(int x, int y, COLORREF color);
		void draw_line(int x1, int y1, int x2, int y2, COLORREF color);
		void draw_rect(int x, int y, int width, int height, COLORREF color);
		void fill_rect(int x, int y, int width, int height, COLORREF color);
		void draw_circle(int xc, int yc, int radius, COLORREF color);
		void fill_circle(int xc, int yc, int radius, COLORREF color);
		void update();
		bool is_quit();

		static LRESULT CALLBACK window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	private:
		LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam);
	};
}

#endif // !WINDOW_HPP
