        InputLayoutHandle CreateInputLayout(const InputLayoutDesc& desc) override
        {
            InputLayoutHandle handle{ ++nextLayoutId_ };
            InputLayoutEntry inputLayoutEntry{};
            inputLayoutEntry.strideBytes = desc.strideBytes;

            inputLayoutEntry.semanticStorage.reserve(desc.attributes.size());
            inputLayoutEntry.elems.reserve(desc.attributes.size());

            for (const auto& attribute : desc.attributes)
            {
                inputLayoutEntry.semanticStorage.emplace_back(SemanticName(attribute.semantic));

                const bool instanced = (attribute.inputSlot != 0);

                D3D12_INPUT_ELEMENT_DESC elemDesc{};
                elemDesc.SemanticName = inputLayoutEntry.semanticStorage.back().c_str();
                elemDesc.SemanticIndex = attribute.semanticIndex;
                elemDesc.Format = ToDXGIVertexFormat(attribute.format);
                elemDesc.InputSlot = attribute.inputSlot;
                elemDesc.AlignedByteOffset = attribute.offsetBytes;
                elemDesc.InputSlotClass = instanced
                    ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
                    : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                elemDesc.InstanceDataStepRate = instanced ? 1 : 0;

                inputLayoutEntry.elems.push_back(elemDesc);
            }

            layouts_[handle.id] = std::move(inputLayoutEntry);
            return handle;
        }

        void DestroyInputLayout(InputLayoutHandle layout) noexcept override
        {
            layouts_.erase(layout.id);
        }

        // ---------------- Shaders / Pipelines ----------------
        ShaderHandle CreateShader(ShaderStage stage, std::string_view debugName, std::string_view sourceOrBytecode) override
        {
            ShaderHandle handle{ ++nextShaderId_ };
            ShaderEntry shaderEntry{};
            shaderEntry.stage = stage;
            shaderEntry.name = std::string(debugName);

            const char* target = (stage == ShaderStage::Vertex) ? "vs_5_1" : "ps_5_1";

            ComPtr<ID3DBlob> code;
            ComPtr<ID3DBlob> errors;

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            auto TryCompile = [&](const char* entry) -> bool
                {
                    code.Reset();
                    errors.Reset();

                    HRESULT hr = D3DCompile(
                        sourceOrBytecode.data(),
                        sourceOrBytecode.size(),
                        shaderEntry.name.c_str(),
                        nullptr, nullptr,
                        entry, target,
                        flags, 0,
                        &code, &errors);

                    return SUCCEEDED(hr);
                };

            if (!TryCompile(shaderEntry.name.c_str()))
            {
                const char* fallback = (stage == ShaderStage::Vertex) ? "VSMain" : "PSMain";
                if (!TryCompile(fallback))
                {
                    std::string err = "DX12: shader compile failed: ";
                    if (errors)
                    {
                        err += std::string((const char*)errors->GetBufferPointer(), errors->GetBufferSize());
                    }
                    throw std::runtime_error(err);
                }
            }

            shaderEntry.blob = code;
            shaders_[handle.id] = std::move(shaderEntry);
            return handle;
        }

        void DestroyShader(ShaderHandle shader) noexcept override
        {
            shaders_.erase(shader.id);
        }

        void DestroyPipeline(PipelineHandle pso) noexcept override
        {
            pipelines_.erase(pso.id);
            // TODO: PSO cache entries - it can be cleared indpendtly - but right here it is ok
        }

        // ---------------- Submission ----------------
