/**
 * MIT License
 * Copyright (c) 2024 Carlos Machado
 * v1.0.0
 */

#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <windows.h>

class SimpleGraphicsEngine {
public:
    SimpleGraphicsEngine();
    ~SimpleGraphicsEngine();

    bool Initialize(const wchar_t* title, int width, int height);
    void Run();
    void ClearScreen(COLORREF color);
    void DrawPixel(int x, int y, COLORREF color);

protected:
    virtual void OnInitialize() {}
    virtual void OnUpdate() {}
    virtual void OnDraw() {}

private:
    HWND hwnd;
    HDC hdc;
    HBITMAP hbmBackBuffer;
    HDC hdcBackBuffer;
    int screenWidth, screenHeight;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // GRAPHICS_HPP
