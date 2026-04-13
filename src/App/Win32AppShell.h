#pragma once

#if defined(CORE_USE_DX12)
#include <backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace appWin32
{
    struct AppShellContext;

    struct Win32Window
    {
        HWND hwnd{};
        AppShellContext* shell{nullptr};
        int width{};
        int height{};
        bool pendingResize{ false };
        int pendingWidth{};
        int pendingHeight{};
        bool minimized{ false };
        bool running{ true };
    };
    
    struct AppShellContext
    {
        Win32Window* mainWindow{nullptr};
        Win32Window* debugWindow{nullptr};
        rendern::Win32Input* input{nullptr};
        HMENU mainMenu{nullptr};
        bool showDebugWindow{false};
        bool imguiInitialized{false};
    };

    constexpr UINT IDM_MAIN_EXIT = 0x1001;
    constexpr UINT IDM_VIEW_DEBUG_WINDOW = 0x2001;

#if defined(CORE_USE_DX12)
    void UpdateMainMenuDebugWindowCheck(AppShellContext& shell);
#endif

    // Header-safe helpers implemented in Win32AppShell.cpp (which includes <windows.h>).
    // These exist so headers can avoid including <windows.h> when using `import std;`.
    Win32Window* GetWindowUserData(HWND hwnd) noexcept;
    bool TryGetCursorPosClient(HWND hwnd, int& outX, int& outY) noexcept;
    void DestroyWindowSafe(HWND hwnd) noexcept;

    HMENU CreateMainMenu(bool enableDebugItem, bool debugChecked);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateWindowWin32(
        Win32Window& window,
        AppShellContext& shell,
        int width,
        int height,
        const std::wstring& title,
        bool show = true,
        HMENU menu = nullptr);
    void PumpMessages(Win32Window& window);
    void TinySleep();
} // namespace appWin32
