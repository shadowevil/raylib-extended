#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <cstdarg>
#include <regex>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define CloseWindow	CloseWindowWin32
#define ShowCursor	ShowCursorWin32
#include <Windows.h>
#include <shlobj.h>
#include <shellapi.h>
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#include <tlhelp32.h>
extern "C" {
#include "raylib.h"
#include "rlgl.h"
}
#include "raymath.h"

#undef RGB
struct RGB {
    RGB(uint8_t r, uint8_t g, uint8_t b) {
        rgb8 = (uint8_t)(((r & 0xE0)) | ((g >> 3) & 0x1C) | (b >> 6));
        rgb16 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        rgb32 = (uint32_t)(((uint8_t)(r) | ((uint16_t)((uint8_t)(g)) << 8)) | (((uint32_t)(uint8_t)(b)) << 16));
    }

    constexpr operator uint8_t() const { return rgb8; }
    constexpr operator uint16_t() const { return rgb16; }
    constexpr operator uint32_t() const { return rgb32; }
    constexpr operator Color() const {
        return Color{ (uint8_t)(rgb32 & 0xFF), (uint8_t)((rgb32 >> 8) & 0xFF), (uint8_t)((rgb32 >> 16) & 0xFF), 255 };
    }
    constexpr operator Vector3() const {
        return Vector3{ (float)(rgb32 & 0xFF) / 255.0f, (float)((rgb32 >> 8) & 0xFF) / 255.0f, (float)((rgb32 >> 16) & 0xFF) / 255.0f };
    }
#ifdef _WIN32
    constexpr operator COLORREF() const {
        return (COLORREF)rgb32;
    }
#endif

private:
    uint8_t rgb8;
    uint16_t rgb16;
    uint32_t rgb32;
};

#undef RGBA
struct RGBA {
    RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        rgba8 = (uint8_t)(((r & 0xE0)) | ((g >> 3) & 0x1C) | (b >> 6)); // RGB 3-3-2
        rgba16 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)); // RGB 5-6-5
        rgba32 = (uint32_t)(r | (g << 8) | (b << 16) | (a << 24)); // RGBA8888
    }

    constexpr operator uint8_t() const { return rgba8; }
    constexpr operator uint16_t() const { return rgba16; }
    constexpr operator uint32_t() const { return rgba32; }
    constexpr operator Color() const {
        return Color{
            (uint8_t)(rgba32 & 0xFF),
            (uint8_t)((rgba32 >> 8) & 0xFF),
            (uint8_t)((rgba32 >> 16) & 0xFF),
            (uint8_t)((rgba32 >> 24) & 0xFF)
        };
    }
    constexpr operator Vector4() const {
        return Vector4{
            (float)(rgba32 & 0xFF) / 255.0f,
            (float)((rgba32 >> 8) & 0xFF) / 255.0f,
            (float)((rgba32 >> 16) & 0xFF) / 255.0f,
            (float)((rgba32 >> 24) & 0xFF) / 255.0f
        };
    }
#ifdef _WIN32
    constexpr operator COLORREF() const {
        return (COLORREF)(rgba32 & 0x00FFFFFF);
    }
#endif

private:
    uint8_t rgba8;
    uint16_t rgba16;
    uint32_t rgba32;
};

inline void SetWindowClickThrough(bool enable) {
    HWND hwnd = (HWND)GetWindowHandle(); // Raylib helper to get Win32 handle
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (enable) {
        exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    }
    else {
        exStyle &= ~WS_EX_TRANSPARENT; // keep layered but not transparent-to-clicks
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}

inline void SetWindowTopMost(bool enable) {
    HWND hwnd = (HWND)GetWindowHandle(); // Raylib helper to get native Win32 HWND
    if (enable) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

inline int GetTaskbarHeight(int monitorIndex) {
    if (monitorIndex == -1)
        monitorIndex = GetCurrentMonitor();

    if (monitorIndex < 0 || monitorIndex >= GetMonitorCount())
        return 0;

    // Raylib gives us monitor rectangle
    int monX = (int)GetMonitorPosition(monitorIndex).x;
    int monY = (int)GetMonitorPosition(monitorIndex).y;
    int monW = GetMonitorWidth(monitorIndex);
    int monH = GetMonitorHeight(monitorIndex);

    RECT monitorRect;
    monitorRect.left = monX;
    monitorRect.top = monY;
    monitorRect.right = monX + monW;
    monitorRect.bottom = monY + monH;

    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);

    HMONITOR hMon = MonitorFromRect(&monitorRect, MONITOR_DEFAULTTONEAREST);
    if (GetMonitorInfo(hMon, &mi)) {
        int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
        int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;
        int workW = mi.rcWork.right - mi.rcWork.left;
        int workH = mi.rcWork.bottom - mi.rcWork.top;

        if (workH < fullH) {
            // taskbar top or bottom
            return fullH - workH;
        }
        else if (workW < fullW) {
            // taskbar left or right
            return fullW - workW;
        }
    }

    return 0; // hidden or fullscreen
}

inline void HideFromTaskbar() {
    HWND hwnd = (HWND)GetWindowHandle();
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    // remove "AppWindow", add "ToolWindow"
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOOLWINDOW;

    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    // Apply changes
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);
}

inline Vector2 GetCursorPosition() {
    HWND hwnd = (HWND)GetWindowHandle();  // Obtain the window handle from Raylib
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    return Vector2{ (float)p.x, (float)p.y };
}

inline bool IsMouseButtonDownGlobal(int button) {
    SHORT state = 0;
    switch (button) {
    case MOUSE_LEFT_BUTTON:  state = GetAsyncKeyState(VK_LBUTTON); break;
    case MOUSE_RIGHT_BUTTON: state = GetAsyncKeyState(VK_RBUTTON); break;
    case MOUSE_MIDDLE_BUTTON: state = GetAsyncKeyState(VK_MBUTTON); break;
    }
    return (state & 0x8000) != 0; // high bit = key down
}

inline bool IsMouseButtonUpGlobal(int button) {
    return !IsMouseButtonDownGlobal(button);
}

// Track previous state per button
inline bool IsMouseButtonPressedGlobal(int button) {
    static bool prev[3] = { false, false, false };
    bool now = IsMouseButtonDownGlobal(button);
    bool pressed = (now && !prev[button]);
    prev[button] = now;
    return pressed;
}

inline bool IsMouseButtonReleasedGlobal(int button) {
    static bool prev[3] = { false, false, false };
    bool now = IsMouseButtonDownGlobal(button);
    bool released = (!now && prev[button]);
    prev[button] = now;
    return released;
}

#undef MessageBox
namespace MessageBox
{
    enum class Type {
        Info,
        Warning,
        Error
    };

    enum class Buttons {
        OK,
        OKCancel,
        YesNo,
        YesNoCancel
    };

    enum class Result {
        None,
        OK,
        Cancel,
        Yes,
        No
    };

    inline static Result Show(const std::string& title,
        const std::string& message,
        Type type = Type::Info,
        Buttons buttons = Buttons::OK)
    {
        UINT style = 0;

        // Button styles
        switch (buttons) {
        case Buttons::OK: style = MB_OK; break;
        case Buttons::OKCancel: style = MB_OKCANCEL; break;
        case Buttons::YesNo: style = MB_YESNO; break;
        case Buttons::YesNoCancel: style = MB_YESNOCANCEL; break;
        }

        // Icon styles
        switch (type) {
        case Type::Info: style |= MB_ICONINFORMATION; break;
        case Type::Warning: style |= MB_ICONWARNING; break;
        case Type::Error: style |= MB_ICONERROR; break;
        }

        int r = MessageBoxA(nullptr, message.c_str(), title.c_str(), style);
        switch (r) {
        case IDOK: return Result::OK;
        case IDCANCEL: return Result::Cancel;
        case IDYES: return Result::Yes;
        case IDNO: return Result::No;
        default: return Result::None;
        }
    }
}

namespace Program
{
    enum class ExecFlags
    {
        None = 0,
        WaitForExit = 1 << 0,
        Hidden = 1 << 1,
        AsAdmin = 1 << 2
    };
    inline ExecFlags operator|(ExecFlags a, ExecFlags b) { return (ExecFlags)((int)a | (int)b); }
    inline bool operator&(ExecFlags a, ExecFlags b) { return ((int)a & (int)b) != 0; }

    inline static bool Execute(const std::filesystem::path& programPath,
        const std::vector<std::string>& args = {},
        ExecFlags flags = ExecFlags::None,
        const std::filesystem::path& workingDir = {})
    {
        if (!std::filesystem::exists(programPath))
            return false;

        std::string ext = programPath.extension().string();
        for (auto& c : ext) c = (char)tolower(c);

        std::string cmdLine = "\"" + programPath.string() + "\"";
        for (const auto& arg : args)
            cmdLine += " \"" + arg + "\"";

        if (ext == ".exe" || ext == ".bat" || ext == ".cmd") {
            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);

            if (flags & ExecFlags::Hidden)
                si.dwFlags |= STARTF_USESHOWWINDOW, si.wShowWindow = SW_HIDE;

            DWORD creationFlags = 0;
            if (flags & ExecFlags::AsAdmin) {
                ShellExecuteA(nullptr, "runas", programPath.string().c_str(),
                    args.empty() ? nullptr : cmdLine.c_str(),
                    workingDir.empty() ? nullptr : workingDir.string().c_str(), SW_SHOWNORMAL);
                return true;
            }

            if (CreateProcessA(
                nullptr,
                cmdLine.data(),
                nullptr, nullptr, FALSE,
                creationFlags, nullptr,
                workingDir.empty() ? nullptr : workingDir.string().c_str(),
                &si, &pi))
            {
                if (flags & ExecFlags::WaitForExit)
                    WaitForSingleObject(pi.hProcess, INFINITE);

                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                return true;
            }
            return false;
        }

        std::string params;
        for (const auto& arg : args) {
            if (!params.empty()) params += " ";
            params += "\"" + arg + "\"";
        }

        HINSTANCE result = ShellExecuteA(
            nullptr,
            (flags & ExecFlags::AsAdmin) ? "runas" : "open",
            programPath.string().c_str(),
            params.empty() ? nullptr : params.c_str(),
            workingDir.empty() ? nullptr : workingDir.string().c_str(),
            (flags & ExecFlags::Hidden) ? SW_HIDE : SW_SHOWNORMAL);

        return reinterpret_cast<intptr_t>(result) > 32;
    }

    inline bool IsRunning(const std::string& namePattern)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        std::regex pattern(namePattern, std::regex_constants::icase);

        bool found = false;
        if (Process32First(snapshot, &pe)) {
            do {
                std::string exe = pe.szExeFile;
                if (std::regex_search(exe, pattern)) {
                    found = true;
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }

        CloseHandle(snapshot);
        return found;
    }

    inline bool IsRunningExact(const std::string& exeName)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        bool found = false;

        if (Process32First(snapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, exeName.c_str()) == 0) {
                    found = true;
                    break;
                }
            } while (Process32Next(snapshot, &pe));
        }

        CloseHandle(snapshot);
        return found;
    }
}
