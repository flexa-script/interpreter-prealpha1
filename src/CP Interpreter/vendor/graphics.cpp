#include "graphics.hpp"

using namespace axe;


Graphics::Graphics() : hwnd(nullptr), hdc(nullptr), hbmBackBuffer(nullptr), hdcBackBuffer(nullptr), screenWidth(0), screenHeight(0) {}

Graphics::~Graphics() {
	if (hbmBackBuffer) {
		DeleteObject(hbmBackBuffer);
	}
	if (hdcBackBuffer) {
		DeleteDC(hdcBackBuffer);
	}
	if (hwnd) {
		DestroyWindow(hwnd);
	}
}

bool Graphics::initialize(const wchar_t* title, int width, int height) {
	screenWidth = width;
	screenHeight = height;

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

	hdcBackBuffer = CreateCompatibleDC(hdc);
	hbmBackBuffer = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdcBackBuffer, hbmBackBuffer);

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
			RECT rect = { 0, 0, screenWidth, screenHeight };
			FillRect(hdcBackBuffer, &rect, hBrush);
			DeleteObject(hBrush);
		}
	}
	else {
		quit = true;
	}
}

void Graphics::draw_pixel(int x, int y, COLORREF color) {
	SetPixel(hdcBackBuffer, x, y, color);
}

void Graphics::draw_line(int x1, int y1, int x2, int y2, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdcBackBuffer, pen);
	MoveToEx(hdcBackBuffer, x1, y1, nullptr);
	LineTo(hdcBackBuffer, x2, y2);
	SelectObject(hdcBackBuffer, oldPen);
	DeleteObject(pen);
}

void Graphics::draw_rect(int x, int y, int width, int height, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdcBackBuffer, pen);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdcBackBuffer, GetStockObject(NULL_BRUSH));
	Rectangle(hdcBackBuffer, x, y, x + width, y + height);
	SelectObject(hdcBackBuffer, oldPen);
	SelectObject(hdcBackBuffer, oldBrush);
	DeleteObject(pen);
}

void Graphics::fill_rect(int x, int y, int width, int height, COLORREF color) {
	RECT rect = { x, y, x + width, y + height };
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(hdcBackBuffer, &rect, brush);
	DeleteObject(brush);
}

void Graphics::draw_circle(int xc, int yc, int radius, COLORREF color) {
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HPEN oldPen = (HPEN)SelectObject(hdcBackBuffer, pen);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdcBackBuffer, GetStockObject(NULL_BRUSH));
	Ellipse(hdcBackBuffer, xc - radius, yc - radius, xc + radius, yc + radius);
	SelectObject(hdcBackBuffer, oldPen);
	SelectObject(hdcBackBuffer, oldBrush);
	DeleteObject(pen);
}

void Graphics::fill_circle(int xc, int yc, int radius, COLORREF color) {
	HBRUSH brush = CreateSolidBrush(color);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdcBackBuffer, brush);
	HPEN oldPen = (HPEN)SelectObject(hdcBackBuffer, GetStockObject(NULL_PEN));
	Ellipse(hdcBackBuffer, xc - radius, yc - radius, xc + radius, yc + radius);
	SelectObject(hdcBackBuffer, oldPen);
	SelectObject(hdcBackBuffer, oldBrush);
	DeleteObject(brush);
}

void Graphics::update() {
	BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcBackBuffer, 0, 0, SRCCOPY);
}

bool Graphics::is_quit() {
	return quit;
}

LRESULT CALLBACK Graphics::window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Graphics* engine;

	if (uMsg == WM_CREATE) {
		engine = reinterpret_cast<Graphics*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(engine));
	}
	else {
		engine = reinterpret_cast<Graphics*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (engine) {
		switch (uMsg) {
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_PAINT: {
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			BitBlt(ps.hdc, 0, 0, engine->screenWidth, engine->screenHeight, engine->hdcBackBuffer, 0, 0, SRCCOPY);
			EndPaint(hwnd, &ps);
			return 0;
		}
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
