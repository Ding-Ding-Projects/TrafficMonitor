// Hidden-window checks: a real static control placed at the measured height must show all of its text.
#include "TextExtent.h"
#include <cstdio>
#include <string>

namespace
{
    int failures = 0;
    void Check(bool value, const char* name) { std::printf("%s %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }
    int Px(int value, int dpi) { return MulDiv(value, dpi, 96); }

    // Lowest row (or -1) that differs from the background colour of an empty control of the same size.
    int InkBottom(HWND control, int width, int height, COLORREF background)
    {
        HDC screen = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screen);
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ old = SelectObject(dc, bitmap);
        SendMessageW(control, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_ERASEBKGND);
        GdiFlush();
        const auto* pixels = static_cast<const unsigned*>(bits);
        const unsigned reference = (GetRValue(background) << 16) | (GetGValue(background) << 8) | GetBValue(background);
        int bottom = -1;
        for (int y = height - 1; y >= 0 && bottom < 0; --y)
            for (int x = 0; x < width; ++x)
                if ((pixels[y * width + x] & 0xFFFFFF) != reference) { bottom = y; break; }
        SelectObject(dc, old);
        DeleteObject(bitmap);
        DeleteDC(dc);
        ReleaseDC(nullptr, screen);
        return bottom;
    }

    COLORREF Background(HWND parent, int width, int height, HFONT font, HINSTANCE instance)
    {
        HWND empty = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, 0, width, height, parent, nullptr, instance, nullptr);
        SendMessageW(empty, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
        HDC screen = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screen);
        HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
        HGDIOBJ old = SelectObject(dc, bitmap);
        SendMessageW(empty, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_ERASEBKGND);
        const COLORREF color = GetPixel(dc, 0, 0);
        SelectObject(dc, old);
        DeleteObject(bitmap);
        DeleteDC(dc);
        ReleaseDC(nullptr, screen);
        DestroyWindow(empty);
        return color;
    }
}

int main()
{
    SetProcessDPIAware();
    const std::wstring english =
        L"Automatic checks run at startup and every five minutes. A staged update never restarts the app without your choice unless automatic restart is on; "
        L"it is otherwise applied at the next launch. Use Check for updates in the monitor menu for a manual check. Current service: ";
    const std::wstring cantonese =
        L"啟動時同每五分鐘自動檢查一次。除非開啟自動重新啟動，否則已準備嘅更新唔會未經你同意重新啟動。"
        L"目前狀態：";
    const std::wstring failure =
        L"Update failed. Stage: download. Error code 12007: no system description. HTTP status 404. "
        L"See https://example.invalid/releases/download/v1.2.3/TrafficMonitor_x64_Setup_1.2.3.exe?a=1&b=2";
    const std::wstring texts[] = {
        english + failure,
        cantonese + L"更新失敗。HTTP 狀態 404。",
        english + failure + L"\r\n" + cantonese + L"更新失敗。HTTP 狀態 404。",
        english + L"Waiting for next check",
    };
    const int dpis[] = { 96, 120, 144, 192 };
    const int widths[] = { 320, 480, 700, 836 };

    WNDCLASSW cls{};
    cls.lpfnWndProc = DefWindowProcW;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = L"TextExtentHost";
    RegisterClassW(&cls);

    bool allFit = true;
    int checked = 0;
    for (const int dpi : dpis)
    {
        NONCLIENTMETRICSW metrics{ sizeof(metrics) };
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
        LOGFONTW logical = metrics.lfMessageFont;
        logical.lfHeight = -Px(14, dpi);
        HFONT font = CreateFontIndirectW(&logical);
        for (const auto& text : texts)
            for (const int width : widths)
            {
                HWND host = CreateWindowW(L"TextExtentHost", L"", WS_POPUP, 0, 0, 3000, 3000, nullptr, nullptr, cls.hInstance, nullptr);
                HDC dc = GetDC(host);
                HGDIOBJ previous = SelectObject(dc, font);
                const int height = (std::max)(24, NativeMaterial::WrappedTextHeight(dc, text.c_str(), Px(width, dpi), dpi));
                SelectObject(dc, previous);
                ReleaseDC(host, dc);
                const int placed = Px(height, dpi);
                const int widthPx = Px(width, dpi);
                const COLORREF background = Background(host, widthPx, placed, font, cls.hInstance);
                HWND tall = CreateWindowW(L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, 0, widthPx, 2400, host, nullptr, cls.hInstance, nullptr);
                SendMessageW(tall, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                HWND fitted = CreateWindowW(L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, 0, widthPx, placed, host, nullptr, cls.hInstance, nullptr);
                SendMessageW(fitted, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                const int full = InkBottom(tall, widthPx, 2400, background);
                const int shown = InkBottom(fitted, widthPx, placed, background);
                const bool fits = full >= 0 && full < placed && shown == full;
                if (!fits) std::printf("  text=%d dpi=%d width=%d placed=%d lowestInkUnconstrained=%d lowestInkPlaced=%d\n", static_cast<int>(&text - texts), dpi, width, placed, full, shown);
                allFit = allFit && fits;
                ++checked;
                DestroyWindow(host);
            }
        DeleteObject(font);
    }
    std::printf("checked %d text/width/scale combinations\n", checked);
    Check(allFit, "updater paragraph fits its measured height in English, Cantonese and bilingual text at 100% to 200%");

    Check(NativeMaterial::CoveredContentHeight(500, { 120, 640, 300 }) == 640, "content extent grows to the lowest placed control");
    Check(NativeMaterial::CoveredContentHeight(500, { 120, 300 }) == 500, "content extent keeps its measured height when controls end earlier");
    Check(NativeMaterial::CoveredContentHeight(500, {}) == 500, "empty page keeps its measured height");
    return failures ? 1 : 0;
}
