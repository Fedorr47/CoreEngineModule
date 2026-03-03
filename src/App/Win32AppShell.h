#pragma once

#if defined(CORE_USE_DX12)
#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace appWin32
{
    struct Win32Window
    {
        HWND hwnd{};
        int width{};
        int height{};
        bool pendingResize{ false };
        int pendingWidth{};
        int pendingHeight{};
        bool minimized{ false };
        bool running{ true };
    };

    extern Win32Window* g_window; // main window
    extern rendern::Win32Input* g_input;

#if defined(CORE_USE_DX12)
    extern Win32Window* g_debugWindow;
    extern bool g_showDebugWindow;
    extern bool g_imguiInitialized;
#endif

    constexpr UINT IDM_MAIN_EXIT = 0x1001;
    constexpr UINT IDM_VIEW_DEBUG_WINDOW = 0x2001;

    extern HMENU g_mainMenu;

#if defined(CORE_USE_DX12)
    void UpdateMainMenuDebugWindowCheck();
#endif

    // Header-safe helpers implemented in Win32AppShell.cpp (which includes <windows.h>).
    // These exist so headers can avoid including <windows.h> when using `import std;`.
    bool TryGetCursorPosClient(HWND hwnd, int& outX, int& outY) noexcept;
    void DestroyWindowSafe(HWND hwnd) noexcept;

    HMENU CreateMainMenu(bool enableDebugItem, bool debugChecked);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    Win32Window CreateWindowWin32(int width, int height, const std::wstring& title, bool show = true, HMENU menu = nullptr);
    void PumpMessages(Win32Window& window);
    void TinySleep();
} // namespace appWin32