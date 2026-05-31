#if !defined(_WIN32)
#error "Dx12BackendSmoke is Windows-only and must only be built by the Windows DX12 CMake gate."
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

import core;

namespace
{
    constexpr int kSmokeWindowWidth = 320;
    constexpr int kSmokeWindowHeight = 240;
    constexpr wchar_t kSmokeWindowClassName[] = L"CoreEngineModuleDx12SmokeWindowClass";

    std::string Narrow(std::wstring_view wide)
    {
        std::string result;
        result.reserve(wide.size());
        for (const wchar_t character : wide)
        {
            result.push_back(character <= 0x7f ? static_cast<char>(character) : '?');
        }
        return result;
    }

    std::string LastWin32ErrorMessage(const char* stage)
    {
        const DWORD errorCode = GetLastError();
        if (errorCode == ERROR_SUCCESS)
        {
            return std::string(stage) + " failed";
        }

        wchar_t* messageBuffer = nullptr;
        const DWORD formatResult = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&messageBuffer),
            0,
            nullptr);

        std::string message = std::string(stage) + " failed with Win32 error " + std::to_string(errorCode);
        if (formatResult != 0 && messageBuffer != nullptr)
        {
            message += ": ";
            message += Narrow(messageBuffer);
        }

        if (messageBuffer != nullptr)
        {
            LocalFree(messageBuffer);
        }

        return message;
    }

    LRESULT CALLBACK SmokeWndProc(
        [[maybe_unused]] HWND hwnd,
        [[maybe_unused]] UINT message,
        [[maybe_unused]] WPARAM wParam,
        [[maybe_unused]] LPARAM lParam)
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    class ScopedSmokeWindow final
    {
    public:
        ScopedSmokeWindow(int width, int height)
        {
            const HINSTANCE instanceHandle = GetModuleHandleW(nullptr);

            WNDCLASSEXW windowClass{};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = SmokeWndProc;
            windowClass.hInstance = instanceHandle;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszClassName = kSmokeWindowClassName;

            if (!RegisterClassExW(&windowClass))
            {
                const DWORD errorCode = GetLastError();
                if (errorCode != ERROR_CLASS_ALREADY_EXISTS)
                {
                    throw std::runtime_error(LastWin32ErrorMessage("Dx12BackendSmoke/RegisterClassExW"));
                }
            }

            RECT rect{ 0, 0, width, height };
            if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE))
            {
                throw std::runtime_error(LastWin32ErrorMessage("Dx12BackendSmoke/AdjustWindowRect"));
            }

            hwnd_ = CreateWindowExW(
                0,
                kSmokeWindowClassName,
                L"CoreEngineModule DX12 smoke",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                rect.right - rect.left,
                rect.bottom - rect.top,
                nullptr,
                nullptr,
                instanceHandle,
                nullptr);

            if (!hwnd_)
            {
                throw std::runtime_error(LastWin32ErrorMessage("Dx12BackendSmoke/CreateWindowExW"));
            }

            // Keep the smoke off-screen from a user perspective while still giving DXGI a real HWND.
            ShowWindow(hwnd_, SW_HIDE);
            UpdateWindow(hwnd_);
        }

        ~ScopedSmokeWindow()
        {
            if (hwnd_)
            {
                DestroyWindow(hwnd_);
                hwnd_ = nullptr;
            }
        }

        ScopedSmokeWindow(const ScopedSmokeWindow&) = delete;
        ScopedSmokeWindow& operator=(const ScopedSmokeWindow&) = delete;
        ScopedSmokeWindow(ScopedSmokeWindow&&) = delete;
        ScopedSmokeWindow& operator=(ScopedSmokeWindow&&) = delete;

        [[nodiscard]] HWND Hwnd() const noexcept
        {
            return hwnd_;
        }

    private:
        HWND hwnd_{};
    };

    void PumpSmokeMessages()
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    template <typename Func>
    ::testing::AssertionResult RunStage(const char* stageName, Func&& func)
    {
        try
        {
            std::forward<Func>(func)();
            return ::testing::AssertionSuccess();
        }
        catch (const std::exception& exception)
        {
            return ::testing::AssertionFailure() << stageName << ": " << exception.what();
        }
        catch (...)
        {
            return ::testing::AssertionFailure() << stageName << ": unknown exception";
        }
    }
}

TEST(Dx12BackendSmoke, StartsSubmitsOneEmptyFrameAndShutsDown)
{
    std::unique_ptr<ScopedSmokeWindow> window;
    std::unique_ptr<rhi::IRHIDevice> device;
    std::unique_ptr<rhi::IRHISwapChain> swapChain;

    ASSERT_TRUE(RunStage("create Win32 smoke window", [&]
    {
        window = std::make_unique<ScopedSmokeWindow>(kSmokeWindowWidth, kSmokeWindowHeight);
        if (window->Hwnd() == nullptr)
        {
            throw std::runtime_error("Dx12BackendSmoke/CreateWindowExW returned null HWND");
        }
        PumpSmokeMessages();
    }));
    ASSERT_NE(window, nullptr);

    ASSERT_TRUE(RunStage("create DX12 device", [&]
    {
        device = rhi::CreateDX12Device();
        if (!device)
        {
            throw std::runtime_error("rhi::CreateDX12Device returned null");
        }
        if (device->GetBackend() != rhi::Backend::DirectX12)
        {
            throw std::runtime_error("CreateDX12Device did not select the DirectX12 backend");
        }
    }));
    ASSERT_NE(device, nullptr);

    ASSERT_TRUE(RunStage("create DX12 swapchain", [&]
    {
        rhi::DX12SwapChainDesc swapChainDesc{};
        swapChainDesc.hwnd = window->Hwnd();
        swapChainDesc.bufferCount = 2;
        swapChainDesc.base.extent = rhi::Extent2D{
            static_cast<std::uint32_t>(kSmokeWindowWidth),
            static_cast<std::uint32_t>(kSmokeWindowHeight)};
        swapChainDesc.base.backbufferFormat = rhi::Format::BGRA8_UNORM;
        swapChainDesc.base.vsync = false;

        swapChain = rhi::CreateDX12SwapChain(*device, swapChainDesc);
        if (!swapChain)
        {
            throw std::runtime_error("rhi::CreateDX12SwapChain returned null");
        }
        if (swapChain->GetDesc().extent.width != static_cast<std::uint32_t>(kSmokeWindowWidth)
            || swapChain->GetDesc().extent.height != static_cast<std::uint32_t>(kSmokeWindowHeight))
        {
            throw std::runtime_error("DX12 swapchain extent does not match requested smoke window extent");
        }
    }));
    ASSERT_NE(swapChain, nullptr);

    ASSERT_TRUE(RunStage("submit one empty DX12 frame", [&]
    {
        rhi::ClearDesc clear{};
        clear.clearColor = true;
        clear.clearDepth = false;
        clear.clearStencil = false;
        clear.color = {0.02f, 0.02f, 0.03f, 1.0f};

        rhi::BeginPassDesc beginPass{};
        beginPass.frameBuffer = swapChain->GetCurrentBackBuffer();
        beginPass.extent = swapChain->GetDesc().extent;
        beginPass.clearDesc = clear;
        beginPass.swapChain = swapChain.get();
        beginPass.bindDepthStencil = false;

        rhi::CommandList commandList{};
        commandList.BeginPass(beginPass);
        commandList.SetViewport(0, 0, kSmokeWindowWidth, kSmokeWindowHeight);
        commandList.EndPass();

        device->SubmitCommandList(std::move(commandList));
    }));

    ASSERT_TRUE(RunStage("present submitted DX12 frame", [&]
    {
        swapChain->Present();
        device->WaitIdle();
        PumpSmokeMessages();
    }));

    ASSERT_TRUE(RunStage("shutdown DX12 smoke resources", [&]
    {
        swapChain.reset();
        device->WaitIdle();
        device.reset();
        window.reset();
        PumpSmokeMessages();
    }));
}
