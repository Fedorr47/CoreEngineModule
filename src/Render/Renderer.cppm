module;

#include <memory>
#include <utility>

#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"

export module core:render_renderer;

// Re-export shared settings so external code can keep using
// `rendern::RendererSettings` via `import core:render`.
export import :renderer_settings;

import :rhi;
import :render_frame_view;
import :thread_affinity;

#if defined(CORE_USE_GL)
import :renderer_mesh_gl;
#endif

#if defined(CORE_USE_DX12)
import :renderer_dx12;
#endif

namespace rendern
{
    namespace detail
    {
        // ---------------------------------------------------------------------
        // Facade implementation
        // ---------------------------------------------------------------------
        struct IRendererImpl
        {
            virtual ~IRendererImpl() = default;
            virtual void RenderFrame(rhi::IRHISwapChain& swapChain, const RenderFrameView& frameView, const void* imguiDrawData) = 0;
            virtual void SetSettings(const RendererSettings& settings) = 0;
            virtual RendererCpuTimingSnapshot GetLastCpuTimings() const = 0;
            virtual void Shutdown() = 0;
        };

        class NullRendererImpl final : public IRendererImpl
        {
        public:
            void RenderFrame(rhi::IRHISwapChain& swapChain, const RenderFrameView&, const void*) override
            {
                swapChain.Present();
            }
            void SetSettings(const RendererSettings&) override {}
            RendererCpuTimingSnapshot GetLastCpuTimings() const override { return {}; }
            void Shutdown() override {}
        };

        #if defined(CORE_USE_GL)
        class GLRendererImpl final : public IRendererImpl
        {
        public:
            GLRendererImpl(rhi::IRHIDevice& device, RendererSettings settings)
                : impl_(device, std::move(settings))
            {}

            void RenderFrame(rhi::IRHISwapChain& swapChain, const RenderFrameView& frameView, const void* imguiDrawData) override
            {
                impl_.RenderFrame(swapChain, frameView);
            }
            void SetSettings(const RendererSettings& settings) override
            {
                impl_.SetSettings(settings);
            }

            RendererCpuTimingSnapshot GetLastCpuTimings() const override
            {
                return {};
            }

            void Shutdown() override
            {
                impl_.Shutdown();
            }

        private:
            GLMeshRenderer impl_;
        };
        #endif

#if defined(CORE_USE_DX12)
        class DX12RendererImpl final : public IRendererImpl
        {
        public:
            DX12RendererImpl(rhi::IRHIDevice& device, RendererSettings settings)
                : impl_(device, std::move(settings))
            {}

            void RenderFrame(rhi::IRHISwapChain& swapChain, const RenderFrameView& frameView, const void* imguiDrawData) override
            {
                impl_.RenderFrame(swapChain, frameView, imguiDrawData);
            }

            void SetSettings(const RendererSettings& settings) override
            {
                impl_.SetSettings(settings);
            }

            RendererCpuTimingSnapshot GetLastCpuTimings() const override
            {
                return impl_.GetLastCpuTimings();
            }

            void Shutdown() override
            {
                impl_.Shutdown();
            }

        private:
            DX12Renderer impl_;
        };
#endif
    }


    // -------------------------------------------------------------------------
    // Public facade
    // -------------------------------------------------------------------------
    export class Renderer
    {
    public:
        Renderer(rhi::IRHIDevice& device, RendererSettings settings = {})
            : device_(device)
            , settings_(settings)
        {
            switch (device_.GetBackend())
            {
            case rhi::Backend::OpenGL:
                #if defined(CORE_USE_GL)
                impl_ = std::make_unique<detail::GLRendererImpl>(device_, std::move(settings));
                break;
                #else
                impl_ = std::make_unique<detail::NullRendererImpl>();
                break;
                #endif

            case rhi::Backend::DirectX12:
#if defined(CORE_USE_DX12)
                impl_ = std::make_unique<detail::DX12RendererImpl>(device_, std::move(settings));
                break;
#else
                impl_ = std::make_unique<detail::NullRendererImpl>();
                break;
#endif

            default:
                impl_ = std::make_unique<detail::NullRendererImpl>();
                break;
            }
        }

        void RenderFrame(rhi::IRHISwapChain& swapChain, const RenderFrameView& frameView, const void* imguiDrawData = nullptr)
        {
            CORE_ASSERT_RENDER_THREAD();

            swapChain.SetVSyncEnabled(settings_.enableVSync);
            impl_->RenderFrame(swapChain, frameView, imguiDrawData);
            lastPresentDiagnostics_ = swapChain.GetPresentDiagnostics();
        }

        void SetSettings(const RendererSettings& settings)
        {
            CORE_ASSERT_RENDER_THREAD();

            settings_ = settings;
            impl_->SetSettings(settings);
        }

        RendererCpuTimingSnapshot GetLastCpuTimings() const
        {
            RendererCpuTimingSnapshot snapshot = impl_->GetLastCpuTimings();
            snapshot.presentDiagnostics = lastPresentDiagnostics_;
            return snapshot;
        }

        void Shutdown()
        {
            CORE_ASSERT_RENDER_THREAD();

            impl_->Shutdown();
        }

    private:
        rhi::IRHIDevice& device_;
        RendererSettings settings_{};
        rhi::PresentDiagnostics lastPresentDiagnostics_{};
        std::unique_ptr<detail::IRendererImpl> impl_;
    };
}
