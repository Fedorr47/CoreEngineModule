#if CORE_DX12_HAS_DXC
        using DxcCreateInstanceProc = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
#endif

        static std::wstring ToWide_(std::string_view s)
        {
            if (s.empty())
            {
                return {};
            }

            int required = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
            if (required <= 0)
            {
                required = MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
                if (required <= 0)
                {
                    return {};
                }

                std::wstring w(static_cast<size_t>(required), L'\0');
                MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), w.data(), required);
                return w;
            }

            std::wstring w(static_cast<size_t>(required), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), required);
            return w;
        }

        void DetectCapabilities_()
        {
            device2_.Reset();
            core_.device.As(&device2_);

            // D3D12 options3: View Instancing tier lives here.
            D3D12_FEATURE_DATA_D3D12_OPTIONS3 opt3{};
            if (SUCCEEDED(NativeDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &opt3, sizeof(opt3))))
            {
                supportsViewInstancing_ = (opt3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED);
            }
            else
            {
                supportsViewInstancing_ = false;
            }

            // Layered rendering (SV_RenderTargetArrayIndex / SV_ViewportArrayIndex) capability.
            // NOTE: Field name differs across Windows SDK versions.
            // Some SDKs expose:
            //   VPAndRTArrayIndexFromAnyShaderFeedingRasterizer
            // Others expose:
            //   VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation
            //
            // We only enable layered point-shadow if it's supported without relying on GS emulation.
            const auto ReadVPAndRTArrayIndexSupport = [](const D3D12_FEATURE_DATA_D3D12_OPTIONS& opt) -> bool
                {
                    if constexpr (requires { opt.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer; })
                    {
                        return opt.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer ? true : false;
                    }
                    else if constexpr (requires { opt.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation; })
                    {
                        return opt.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation ? true : false;
                    }
                    else
                    {
                        return false;
                    }
                };

            // Layered rendering (SV_RenderTargetArrayIndex / SV_ViewportArrayIndex) capability is exposed in
            // D3D12_FEATURE_D3D12_OPTIONS (NOT OPTIONS3). Some Windows SDK versions do not have this field in OPTIONS3.
            D3D12_FEATURE_DATA_D3D12_OPTIONS opt{};
            if (SUCCEEDED(NativeDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opt, sizeof(opt))))
            {
                supportsVPAndRTArrayIndexFromAnyShader_ = ReadVPAndRTArrayIndexSupport(opt);
            }
            else
            {
                supportsVPAndRTArrayIndexFromAnyShader_ = false;
            }

            supportsViewInstancing_ = (device2_ != nullptr) && (viewInstancingTier_ != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED);

            // Shader Model support.
            D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
            shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
            if (FAILED(NativeDevice()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
            {
                shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
            }
            highestShaderModel_ = shaderModel.HighestShaderModel;

#if CORE_DX12_HAS_DXC
            // We only claim SM6.1 support if both hardware and dxcompiler.dll are available.
            supportsSM6_1_ = (highestShaderModel_ >= D3D_SHADER_MODEL_6_1) && EnsureDXC_();
#else
            supportsSM6_1_ = false;
#endif
        }

#if CORE_DX12_HAS_DXC
        bool EnsureDXC_() noexcept
        {
            if (dxcInitTried_)
            {
                return (dxcCompiler_ != nullptr) && (dxcUtils_ != nullptr);
            }

            dxcInitTried_ = true;

            dxcModule_ = LoadLibraryA("dxcompiler.dll");
            if (!dxcModule_)
            {
                return false;
            }

            auto proc = GetProcAddress(dxcModule_, "DxcCreateInstance");
            if (!proc)
            {
                return false;
            }
            dxcCreateInstance_ = reinterpret_cast<DxcCreateInstanceProc>(proc);

            if (FAILED(dxcCreateInstance_(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_))))
            {
                dxcUtils_.Reset();
                return false;
            }

            if (FAILED(dxcCreateInstance_(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_))))
            {
                dxcCompiler_.Reset();
                dxcUtils_.Reset();
                return false;
            }

            if (FAILED(dxcUtils_->CreateDefaultIncludeHandler(&dxcIncludeHandler_)))
            {
                dxcIncludeHandler_.Reset();
                dxcCompiler_.Reset();
                dxcUtils_.Reset();
                return false;
            }

            return true;
        }

        void ShutdownDXC_() noexcept
        {
            dxcIncludeHandler_.Reset();
            dxcCompiler_.Reset();
            dxcUtils_.Reset();
            dxcCreateInstance_ = nullptr;

            if (dxcModule_)
            {
                FreeLibrary(dxcModule_);
                dxcModule_ = nullptr;
            }

            dxcInitTried_ = false;
        }

        bool CompileDXC_(
            std::string_view source,
            const wchar_t* targetProfile,
            std::string_view entryPoint,
            [[maybe_unused]] std::string_view debugName,
            ComPtr<ID3DBlob>& outCode,
            std::string* outErrors) noexcept
        {
            outCode.Reset();

            if (!dxcUtils_ || !dxcCompiler_)
            {
                return false;
            }

            const std::wstring wEntry = ToWide_(entryPoint);

            std::vector<const wchar_t*> args;
            args.reserve(16);

            args.push_back(L"-E");
            args.push_back(wEntry.c_str());
            args.push_back(L"-T");
            args.push_back(targetProfile);

#if defined(_DEBUG)
            args.push_back(L"-Zi");
            args.push_back(L"-Od");
#else
            args.push_back(L"-O3");
#endif

            DxcBuffer buffer{};
            buffer.Ptr = source.data();
            buffer.Size = source.size();
            buffer.Encoding = DXC_CP_UTF8;

            ComPtr<IDxcResult> result;
            HRESULT hr = dxcCompiler_->Compile(
                &buffer,
                args.data(),
                static_cast<uint32_t>(args.size()),
                dxcIncludeHandler_.Get(),
                IID_PPV_ARGS(&result));

            if (FAILED(hr) || !result)
            {
                if (outErrors)
                {
                    *outErrors = "DXC: Compile() call failed";
                }
                return false;
            }

            HRESULT status = S_OK;
            result->GetStatus(&status);
            if (FAILED(status))
            {
                if (outErrors)
                {
                    ComPtr<IDxcBlobUtf8> errs;
                    if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errs), nullptr)) && errs && errs->GetStringLength() > 0)
                    {
                        *outErrors = std::string(errs->GetStringPointer(), errs->GetStringLength());
                    }
                    else
                    {
                        *outErrors = "DXC: compilation failed";
                    }
                }
                return false;
            }

            ComPtr<IDxcBlob> obj;
            hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&obj), nullptr);
            if (FAILED(hr) || !obj)
            {
                if (outErrors)
                {
                    *outErrors = "DXC: missing DXIL output";
                }
                return false;
            }

            ID3DBlob* blob = nullptr;
            hr = D3DCreateBlob(obj->GetBufferSize(), &blob);
            if (FAILED(hr) || !blob)
            {
                if (outErrors)
                {
                    *outErrors = "DXC: D3DCreateBlob failed";
                }
                return false;
            }

            std::memcpy(blob->GetBufferPointer(), obj->GetBufferPointer(), obj->GetBufferSize());
            outCode.Attach(blob);
            return true;
        }
#else
        void ShutdownDXC_() noexcept {}
#endif

