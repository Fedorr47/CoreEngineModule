import core;
import std;

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "AppBootstrap.h"

constexpr std::string_view FLAGS_LITERAL{"flags"};
constexpr std::string_view COMMON_ARGUMENTS{"--"};
constexpr std::array<std::string_view, 1> ValidArgumentNames{MAP_LITERAL};

namespace appBootstrap
{
    bool CheckNamedArgument(std::string_view argumentName)
    {
        return std::ranges::find(ValidArgumentNames, argumentName) != ValidArgumentNames.end();
    }
    
    void ParseArgument(const std::string& argument, std::map<std::string, std::vector<std::string>>& args)
    {
        std::string_view value = argument;
        if (value.find(COMMON_ARGUMENTS) == 0)
        {
            value = value.substr(COMMON_ARGUMENTS.length());
        }

        const size_t index = value.find('=');
        if (index != std::string::npos)
        {
            const std::string_view key = value.substr(0, index);
            if (!CheckNamedArgument(key))
            {
                std::cerr << "Invalid key: " << key << std::endl;
                return;
            }

            const std::string_view parsedValue = value.substr(index + 1);
            if (parsedValue.empty())
            {
                std::cerr << "Empty value for key: " << key << std::endl;
                return;
            }

            args[std::string(key)].emplace_back(parsedValue);
        }
        else
        {
            std::cerr << "Invalid argument: " << argument << std::endl;
        }
    }
    
    rhi::Backend ParseAppArguments(int argc, char** argv, std::map<std::string, std::vector<std::string>>& args)
    {
        rhi::Backend backendType = rhi::Backend::DirectX12;
        for (int argIndex = 1; argIndex < argc; ++argIndex)
        {
            const std::string_view argValue = argv[argIndex];
            if (argValue == "--null")
            {
                backendType = rhi::Backend::Null;
                continue;
            }
            ParseArgument(argv[argIndex], args);
        }

        return backendType;
    }

    bool CanUseDebugWindow([[maybe_unused]] rhi::Backend backend)
    {
#if defined(CORE_USE_DX12)
        return backend == rhi::Backend::DirectX12;
#else
        return false;
#endif
    }

    void CreatePrimaryWindowSet(
        int mainWidth,
        int mainHeight,
        const std::wstring& mainTitle,
        bool canUseDebugWindow,
        appWin32::Win32Window& outMainWindow
#if defined(CORE_USE_DX12)
        , appWin32::Win32Window* outDebugWindow
#endif
    )
    {
#if defined(CORE_USE_DX12)
        appWin32::g_showDebugWindow = canUseDebugWindow;
#endif

        appWin32::g_mainMenu = appWin32::CreateMainMenu(canUseDebugWindow, canUseDebugWindow);
        outMainWindow = appWin32::CreateWindowWin32(mainWidth, mainHeight, mainTitle, /*show=*/true, appWin32::g_mainMenu);
        appWin32::g_window = &outMainWindow;

#if defined(CORE_USE_DX12)
        if (outDebugWindow)
        {
            *outDebugWindow = {};
        }

        appWin32::g_debugWindow = nullptr;
        if (canUseDebugWindow && outDebugWindow)
        {
            *outDebugWindow = appWin32::CreateWindowWin32(900, 900, L"CoreEngineModule - Debug UI", /*show=*/appWin32::g_showDebugWindow);
            appWin32::g_debugWindow = outDebugWindow;
        }

        appWin32::UpdateMainMenuDebugWindowCheck();
#endif
    }

    void BindWin32Input(rendern::Win32Input& input)
    {
        appWin32::g_input = &input;
    }

    void CreateDeviceAndSwapChain(
        rhi::Backend backend,
        HWND hwnd,
        int initialWidth,
        int initialHeight,
        std::unique_ptr<rhi::IRHIDevice>& outDevice,
        std::unique_ptr<rhi::IRHISwapChain>& outSwapChain)
    {
        if (backend == rhi::Backend::DirectX12)
        {
#if defined(CORE_USE_DX12)
            outDevice = rhi::CreateDX12Device();

            rhi::DX12SwapChainDesc swapChainDesc{};
            swapChainDesc.hwnd = hwnd;
            swapChainDesc.bufferCount = 2;
            swapChainDesc.base.extent = rhi::Extent2D{
                static_cast<std::uint32_t>(initialWidth),
                static_cast<std::uint32_t>(initialHeight)
            };
            swapChainDesc.base.backbufferFormat = rhi::Format::BGRA8_UNORM;
            swapChainDesc.base.vsync = false;

            outSwapChain = rhi::CreateDX12SwapChain(*outDevice, swapChainDesc);
#else
            outDevice = rhi::CreateNullDevice();
            rhi::SwapChainDesc swapChainDesc{};
            swapChainDesc.extent = rhi::Extent2D{
                static_cast<std::uint32_t>(initialWidth),
                static_cast<std::uint32_t>(initialHeight)
            };
            outSwapChain = rhi::CreateNullSwapChain(*outDevice, swapChainDesc);
#endif
            return;
        }

        outDevice = rhi::CreateNullDevice();
        rhi::SwapChainDesc swapChainDesc{};
        swapChainDesc.extent = rhi::Extent2D{
            static_cast<std::uint32_t>(initialWidth),
            static_cast<std::uint32_t>(initialHeight)
        };
        outSwapChain = rhi::CreateNullSwapChain(*outDevice, swapChainDesc);
    }

#if defined(CORE_USE_DX12)
    void CreateDebugSwapChainIfNeeded(
        rhi::Backend backend,
        rhi::IRHIDevice& device,
        const appWin32::Win32Window& debugWindow,
        std::unique_ptr<rhi::IRHISwapChain>& outDebugSwapChain)
    {
        outDebugSwapChain.reset();

        if (backend != rhi::Backend::DirectX12 || !debugWindow.hwnd)
        {
            return;
        }

        rhi::DX12SwapChainDesc debugSwapChainDesc{};
        debugSwapChainDesc.hwnd = debugWindow.hwnd;
        debugSwapChainDesc.bufferCount = 2;
        debugSwapChainDesc.base.extent = rhi::Extent2D{ 900u, 900u };
        debugSwapChainDesc.base.backbufferFormat = rhi::Format::BGRA8_UNORM;
        debugSwapChainDesc.base.vsync = false;

        outDebugSwapChain = rhi::CreateDX12SwapChain(device, debugSwapChainDesc);
    }
#endif

    std::unique_ptr<ITextureUploader> CreateTextureUploader(rhi::Backend backend, rhi::IRHIDevice& device)
    {
        switch (backend)
        {
        case rhi::Backend::DirectX12:
#if defined(CORE_USE_DX12)
            return std::make_unique<rendern::DX12TextureUploader>(device);
#else
            return std::make_unique<rendern::NullTextureUploader>(device);
#endif
        default:
            return std::make_unique<rendern::NullTextureUploader>(device);
        }
    }
}
