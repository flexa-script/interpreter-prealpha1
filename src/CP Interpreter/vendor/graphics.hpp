/**
 * MIT License
 * Copyright (c) 2024 Carlos Machado
 * v1.0.0
 */

#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <windows.h>

namespace axe {

	class Graphics {
	private:
		HWND hwnd;
		HDC hdc;
		HBITMAP hbm_back_buffer;
		HDC hdc_back_buffer;
		int screen_width, screen_height;
		MSG msg = { 0 };
		bool quit = false;

	public:
		Graphics();
		~Graphics();

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

	private:
		static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

}

#endif // GRAPHICS_HPP
