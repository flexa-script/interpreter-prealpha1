#include "graphics.hpp"

SimpleGraphicsEngine::SimpleGraphicsEngine() : hwnd(nullptr), hdc(nullptr), hbmBackBuffer(nullptr), hdcBackBuffer(nullptr), screenWidth(0), screenHeight(0) {}

SimpleGraphicsEngine::~SimpleGraphicsEngine() {
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

bool SimpleGraphicsEngine::Initialize(const wchar_t* title, int width, int height) {
    screenWidth = width;
    screenHeight = height;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
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

    OnInitialize();

    return true;
}

void SimpleGraphicsEngine::Run() {
    MSG msg = { 0 };

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            OnUpdate();
            OnDraw();

            BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcBackBuffer, 0, 0, SRCCOPY);
        }
    }
}

void SimpleGraphicsEngine::ClearScreen(COLORREF color) {
    HBRUSH hBrush = CreateSolidBrush(color);
    RECT rect = { 0, 0, screenWidth, screenHeight };
    FillRect(hdcBackBuffer, &rect, hBrush);
    DeleteObject(hBrush);
}

void SimpleGraphicsEngine::DrawPixel(int x, int y, COLORREF color) {
    SetPixel(hdcBackBuffer, x, y, color);
}

LRESULT CALLBACK SimpleGraphicsEngine::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SimpleGraphicsEngine* engine;

    if (uMsg == WM_CREATE) {
        engine = reinterpret_cast<SimpleGraphicsEngine*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(engine));
    }
    else {
        engine = reinterpret_cast<SimpleGraphicsEngine*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
