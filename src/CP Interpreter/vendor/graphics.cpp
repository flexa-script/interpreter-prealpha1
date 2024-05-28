#include "graphics.hpp"

using namespace axe;


Graphics::Graphics()
	: hwnd(nullptr), hdc(nullptr), hbm_back_buffer(nullptr),
	hdc_back_buffer(nullptr), screen_width(0), screen_height(0) {}

Graphics::~Graphics() {
	if (hbm_back_buffer) {
		DeleteObject(hbm_back_buffer);
	}
	if (hdc_back_buffer) {
		DeleteDC(hdc_back_buffer);
	}
	if (hwnd) {
		DestroyWindow(hwnd);
	}
}

bool Graphics::initialize(const wchar_t* title, int width, int height) {
	screen_width = width;
	screen_height = height;

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = window_proc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = L"SimpleGraphicsEngineWindowClass";

	if (!RegisterClass(&wc)) {
		return false;
	}

	hwnd = CreateWindowEx(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, GetModuleHandle(NULL), this);

	if (!hwnd) {
		return false;
	}

	ShowWindow(hwnd, SW_SHOW);

	hdc = GetDC(hwnd);

	hdc_back_buffer = CreateCompatibleDC(hdc);
	hbm_back_buffer = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdc_back_buffer, hbm_back_buffer);

	return true;
}

void Graphics::clear_screen(COLORREF color) {
	if (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			HBRUSH hBrush = CreateSolidBrush(color);
			RECT rect = { 0, 0, screen_width, screen_height };
			FillRect(hdc_back_buffer, &rect, hBrush);
			DeleteObject(hBrush);
		}
	}
	else {
		quit = true;
	}
}

void Graphics::draw_pixel(int x, int y, COLORREF color) {
	SetPixel(hdc_back_buffer, x, y, color);
}

void Graphics::draw_line(int x1, int y1, int x2, int y2, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdc_back_buffer, pen);
	MoveToEx(hdc_back_buffer, x1, y1, nullptr);
	LineTo(hdc_back_buffer, x2, y2);
	SelectObject(hdc_back_buffer, oldPen);
	DeleteObject(pen);
}

void Graphics::draw_rect(int x, int y, int width, int height, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdc_back_buffer, pen);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc_back_buffer, GetStockObject(NULL_BRUSH));
	Rectangle(hdc_back_buffer, x, y, x + width, y + height);
	SelectObject(hdc_back_buffer, oldPen);
	SelectObject(hdc_back_buffer, oldBrush);
	DeleteObject(pen);
}

void Graphics::fill_rect(int x, int y, int width, int height, COLORREF color) {
	RECT rect = { x, y, x + width, y + height };
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(hdc_back_buffer, &rect, brush);
	DeleteObject(brush);
}

void Graphics::draw_circle(int xc, int yc, int radius, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdc_back_buffer, pen);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc_back_buffer, GetStockObject(NULL_BRUSH));
	Ellipse(hdc_back_buffer, xc - radius, yc - radius, xc + radius, yc + radius);
	SelectObject(hdc_back_buffer, oldPen);
	SelectObject(hdc_back_buffer, oldBrush);
	DeleteObject(pen);
}

void Graphics::fill_circle(int xc, int yc, int radius, COLORREF color) {
	HBRUSH brush = CreateSolidBrush(color);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc_back_buffer, brush);
	HPEN oldPen = (HPEN)SelectObject(hdc_back_buffer, GetStockObject(NULL_PEN));
	Ellipse(hdc_back_buffer, xc - radius, yc - radius, xc + radius, yc + radius);
	SelectObject(hdc_back_buffer, oldPen);
	SelectObject(hdc_back_buffer, oldBrush);
	DeleteObject(brush);
}

void Graphics::update() {
	BitBlt(hdc, 0, 0, screen_width, screen_height, hdc_back_buffer, 0, 0, SRCCOPY);
}

bool Graphics::is_quit() {
	return quit;
}

LRESULT CALLBACK Graphics::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Graphics* graphic_engine;

	if (uMsg == WM_CREATE) {
		graphic_engine = reinterpret_cast<Graphics*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(graphic_engine));
	}
	else {
		graphic_engine = reinterpret_cast<Graphics*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (graphic_engine) {
		switch (uMsg) {
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_PAINT: {
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			BitBlt(ps.hdc, 0, 0, graphic_engine->screen_width, graphic_engine->screen_height, graphic_engine->hdc_back_buffer, 0, 0, SRCCOPY);
			EndPaint(hwnd, &ps);
			return 0;
		}
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
