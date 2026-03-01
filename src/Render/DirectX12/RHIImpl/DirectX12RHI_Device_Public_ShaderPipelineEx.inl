        ShaderHandle CreateShaderEx(ShaderStage stage, std::string_view debugName, std::string_view sourceOrBytecode, ShaderModel shaderModel) override
        {
            if (shaderModel == ShaderModel::SM5_1)
            {
                return CreateShader(stage, debugName, sourceOrBytecode);
            }

#if CORE_DX12_HAS_DXC
            // Shader Model 6.1 (DXIL) via DXC.
            if (!supportsSM6_1_ || !EnsureDXC_())
            {
                return {};
            }

            const wchar_t* target = (stage == ShaderStage::Vertex) ? L"vs_6_1" : L"ps_6_1";

            auto TryCompile = [&](std::string_view entry) -> ComPtr<ID3DBlob>
                {
                    ComPtr<ID3DBlob> out;
                    std::string err;
                    if (!CompileDXC_(sourceOrBytecode, target, entry, debugName, out, &err))
                    {
                        return {};
                    }
                    return out;
                };

            ComPtr<ID3DBlob> code = TryCompile(std::string_view(debugName));
            if (!code)
            {
                code = TryCompile("main");
            }
            if (!code)
            {
                const char* fallback = (stage == ShaderStage::Vertex) ? "VSMain" : "PSMain";
                code = TryCompile(fallback);
            }

            if (!code)
            {
                return {};
            }

            ShaderHandle handle{ ++nextShaderId_ };
            ShaderEntry shaderEntry{};
            shaderEntry.stage = stage;
            shaderEntry.name = std::string(debugName);
            shaderEntry.blob = code;

            shaders_[handle.id] = std::move(shaderEntry);
            return handle;
#else
            // Built without dxcapi.h; cannot compile SM6 shaders.
            return {};
#endif
        }

        PipelineHandle CreatePipelineEx(std::string_view debugName, ShaderHandle vertexShader, ShaderHandle pixelShader, PrimitiveTopologyType topologyType, std::uint32_t viewInstanceCount) override
        {
            if (viewInstanceCount > 1)
            {
                // View instancing PSOs require ID3D12Device2 + a supported ViewInstancingTier.
                if (!supportsViewInstancing_ || !device2_)
                {
                    return {};
                }
            }

            PipelineHandle handle{ ++nextPsoId_ };
            PipelineEntry pipelineEntry{};
            pipelineEntry.debugName = std::string(debugName);
            pipelineEntry.vs = vertexShader;
            pipelineEntry.ps = pixelShader;
            pipelineEntry.topologyType = topologyType;
            pipelineEntry.viewInstanceCount = viewInstanceCount;
            pipelines_[handle.id] = std::move(pipelineEntry);
            return handle;
        }

        PipelineHandle CreatePipeline(std::string_view debugName, ShaderHandle vertexShader, ShaderHandle pixelShader, PrimitiveTopologyType topologyType) override
        {
            return CreatePipelineEx(debugName, vertexShader, pixelShader, topologyType, 1);
        }

