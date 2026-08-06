import core;
import std;

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

#include "Win32AppShell.h"

namespace appWin32
{
    namespace
    {
        std::string OpenLevelFileDialog(HWND ownerWindow)
        {
            wchar_t selectedPath[32768]{};

            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = ownerWindow;
            dialog.lpstrFile = selectedPath;
            dialog.nMaxFile = static_cast<DWORD>(std::size(selectedPath));

            dialog.lpstrFilter =
                L"CoreEngine level files (*.json)\0"
                L"*.json\0"
                L"All files (*.*)\0"
                L"*.*\0";

            dialog.lpstrTitle = L"Open Level";

            dialog.Flags =
                OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR;

            if (!GetOpenFileNameW(&dialog))
            {
                return {};
            }

            return std::filesystem::path(selectedPath).string();
        }

        void RequestOpenLevel(
            AppShellContext& shell,
            HWND ownerWindow)
        {
            std::string selectedLevelPath = OpenLevelFileDialog(ownerWindow);

            if (selectedLevelPath.empty())
            {
                return;
            }

            shell.requestedLevelPath = std::move(selectedLevelPath);
        }
#if defined(CORE_USE_DX12)       
        void ToggleDebugWindowVisibility(AppShellContext& shell)
        {
            if (shell.debugWindow && shell.debugWindow->hwnd)
            {
                ShowWindow(shell.debugWindow->hwnd, shell.showDebugWindow ? SW_SHOW : SW_HIDE);
                if (shell.showDebugWindow)
                {
                    SetForegroundWindow(shell.debugWindow->hwnd);
                }
            }

            UpdateMainMenuDebugWindowCheck(shell);
        }
#endif
    }

    void UpdateMainMenuDebugWindowCheck(AppShellContext& shell)
    {
        if (!shell.mainMenu)
        {
            return;
        }

        const UINT enableFlags = MF_BYCOMMAND | ((shell.debugWindow && shell.debugWindow->hwnd) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(shell.mainMenu, IDM_VIEW_DEBUG_WINDOW, enableFlags);

        const UINT checkFlags = MF_BYCOMMAND | (shell.showDebugWindow ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(shell.mainMenu, IDM_VIEW_DEBUG_WINDOW, checkFlags);

        if (shell.mainWindow && shell.mainWindow->hwnd)
        {
            DrawMenuBar(shell.mainWindow->hwnd);
        }
    }

    HMENU CreateMainMenu(bool enableDebugItem, bool debugChecked)
    {
        HMENU menu = CreateMenu();
        HMENU filePopup = CreatePopupMenu();
        HMENU viewPopup = CreatePopupMenu();

        AppendMenuW(filePopup, MF_STRING, IDM_FILE_OPEN_LEVEL, L"Open Level...\tCtrl+O");
        AppendMenuW(filePopup,MF_SEPARATOR,0,nullptr);
        AppendMenuW( filePopup, MF_STRING, IDM_MAIN_EXIT, L"Exit");
        AppendMenuW( menu, MF_POPUP, reinterpret_cast<UINT_PTR>(filePopup), L"File");

        UINT viewFlags = MF_STRING;

        if (!enableDebugItem)
        {
            viewFlags |= MF_GRAYED;
        }

        if (debugChecked)
        {
            viewFlags |= MF_CHECKED;
        }

        AppendMenuW(viewPopup,viewFlags,IDM_VIEW_DEBUG_WINDOW, L"Open Debug Window\tF1");
        AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(viewPopup),L"View");

        return menu;
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            auto* window = static_cast<Win32Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            if (window)
            {
                window->hwnd = hwnd;
            }
        }

        Win32Window* self = GetWindowUserData(hwnd);
        AppShellContext* shell = self ? self->shell : nullptr;

        if (shell && shell->input && shell->mainWindow && hwnd == shell->mainWindow->hwnd)
        {
            shell->input->OnWndProc(hwnd, msg, wParam, lParam);
        }

#if defined(CORE_USE_DX12)
        if (shell
            && shell->imguiInitialized
            && shell->debugWindow
            && hwnd == shell->debugWindow->hwnd)
        {
            if (msg != WM_SIZE && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            {
                return 1;
            }
        }
#endif

        switch (msg)
        {
        case WM_COMMAND:
        {
                const int cmdId = static_cast<int>(LOWORD(wParam));

                const bool bIsMainWindow =
                    shell != nullptr
                    && shell->mainWindow != nullptr
                    && hwnd == shell->mainWindow->hwnd;

                if (!bIsMainWindow)
                {
                    break;
                }

                switch (cmdId)
                {
                case IDM_FILE_OPEN_LEVEL:
                    RequestOpenLevel(*shell, hwnd);
                    return 0;

                case IDM_MAIN_EXIT:
                    DestroyWindow(hwnd);
                    return 0;

#if defined(CORE_USE_DX12)
                case IDM_VIEW_DEBUG_WINDOW:
                    shell->showDebugWindow = !shell->showDebugWindow;
                    ToggleDebugWindowVisibility(*shell);
                    return 0;
#endif

                default:
                    break;
                }

                break;
        }
        case WM_CLOSE:
#if defined(CORE_USE_DX12)
            if (shell && shell->debugWindow && hwnd == shell->debugWindow->hwnd)
            {
                ShowWindow(hwnd, SW_HIDE);
                shell->showDebugWindow = false;
                UpdateMainMenuDebugWindowCheck(*shell);
                return 0;
            }
#endif

            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            if (shell && shell->mainWindow && hwnd == shell->mainWindow->hwnd)
            {
                shell->mainWindow->running = false;
                PostQuitMessage(0);
            }
            return 0;

        case WM_SIZE:
        {
            if (self)
            {
                const int newW = static_cast<int>(LOWORD(lParam));
                const int newH = static_cast<int>(HIWORD(lParam));
                self->width = newW;
                self->height = newH;
                self->pendingWidth = newW;
                self->pendingHeight = newH;
                self->pendingResize = true;
                self->minimized = (wParam == SIZE_MINIMIZED) || (newW == 0) || (newH == 0);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN:
            {
                const bool bIsMainWindow =
                    shell != nullptr
                    && shell->mainWindow != nullptr
                    && hwnd == shell->mainWindow->hwnd;

                const bool bWasDown = (lParam & (1 << 30)) != 0;
                const bool bIsControlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

                if (bIsMainWindow
                    && !bWasDown
                    && bIsControlDown
                    && wParam == 'O')
                {
                    RequestOpenLevel(*shell, hwnd);
                    return 0;
                }

                if (wParam == VK_ESCAPE)
                {
                    if (bIsMainWindow)
                    {
                        DestroyWindow(hwnd);
                        return 0;
                    }
                }
                if (wParam == VK_F1)
                {
                    if (!bWasDown && shell)
                    {
                        shell->showDebugWindow = !shell->showDebugWindow;
                        ToggleDebugWindowVisibility(*shell);
                    }
                    return 0;
                }
                break;
            }

        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    Win32Window* GetWindowUserData(HWND hwnd) noexcept
    {
        if (!hwnd)
        {
            return nullptr;
        }

        return reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    void CreateWindowWin32(
        Win32Window& window,
        AppShellContext& shell,
        int width,
        int height,
        const std::wstring& title,
        bool show,
        HMENU menu)
    {
        window = {};
        window.shell = &shell;
        window.width = width;
        window.height = height;

        const HINSTANCE instanceHandle = GetModuleHandleW(nullptr);
        const wchar_t* className = L"CoreEngineModuleWindowClass";

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WndProc;
        windowClass.hInstance = instanceHandle;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = className;

        if (!RegisterClassExW(&windowClass))
        {
            const DWORD errorCode = GetLastError();
            if (errorCode != ERROR_CLASS_ALREADY_EXISTS)
            {
                throw std::runtime_error("RegisterClassExW failed");
            }
        }

        const DWORD style = WS_OVERLAPPEDWINDOW;

        RECT rect{ 0, 0, width, height };
        AdjustWindowRect(&rect, style, menu != nullptr);

        window.hwnd = CreateWindowExW(
            0,
            className,
            title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            menu,
            instanceHandle,
            &window);

        if (!window.hwnd)
        {
            throw std::runtime_error("CreateWindowExW failed");
        }

        ShowWindow(window.hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(window.hwnd);
    }

    bool TryGetCursorPosClient(HWND hwnd, int& outX, int& outY) noexcept
    {
        if (!hwnd)
        {
            return false;
        }

        POINT p{};
        if (!GetCursorPos(&p) || !ScreenToClient(hwnd, &p))
        {
            return false;
        }

        outX = static_cast<int>(p.x);
        outY = static_cast<int>(p.y);
        return true;
    }

    void DestroyWindowSafe(HWND hwnd) noexcept
    {
        if (!hwnd)
        {
            return;
        }
        ::DestroyWindow(hwnd);
    }

    void PumpMessages(Win32Window& window)
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                window.running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void TinySleep()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
} // namespace appWin32
