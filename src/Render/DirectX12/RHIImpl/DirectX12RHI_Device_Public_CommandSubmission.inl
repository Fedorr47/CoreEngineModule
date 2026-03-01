        void SubmitCommandList(CommandList&& commandList) override
        {
            // Begin frame: wait/recycle per-frame stuff + reset allocator/list
            BeginFrame();
            hasSubmitted_ = true;

            hasSubmitted_ = true;

            // Set descriptor heaps (SRV)
            ID3D12DescriptorHeap* heaps[] = { NativeSRVHeap() };
            cmdList_->SetDescriptorHeaps(1, heaps);

            FlushPendingBufferUpdates();

            // State while parsing high-level commands
            GraphicsState curState{};
            PipelineHandle curPipe{};

            InputLayoutHandle curLayout{};
            D3D_PRIMITIVE_TOPOLOGY currentTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            static constexpr std::uint32_t kMaxVBSlots = 2;
            std::array<BufferHandle, kMaxVBSlots> vertexBuffers{};
            std::array<std::uint32_t, kMaxVBSlots> vbStrides{};
            std::array<std::uint32_t, kMaxVBSlots> vbOffsets{};

            BufferHandle indexBuffer{};
            IndexType ibType = IndexType::UINT16;
            std::uint32_t ibOffset = 0;

            // Bound textures by slot (we actuallu use only slot 0)
            std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxSRVSlots> boundTex{};
            for (auto& t : boundTex)
            {
                t = srvHeap_->GetGPUDescriptorHandleForHeapStart(); // null SRV slot0
            }

            // slot 2 (t2) expects StructuredBuffer SRV; point it to null-buffer descriptor (SRV heap index 1).
            if (boundTex.size() > 2)
            {
                boundTex[2].ptr += static_cast<UINT64>(srvInc_);
            }

            // Per-draw constants (raw bytes).
            // The renderer is responsible for packing the layout expected by HLSL.
            static constexpr std::uint32_t kMaxPerDrawConstantsBytes = 512;
            std::array<std::byte, kMaxPerDrawConstantsBytes> perDrawBytes{};
            std::uint32_t perDrawSize = 0;
            std::uint32_t perDrawSlot = 0;

            auto WriteCBAndBind = [&]()
                {
                    FrameResource& fr = CurrentFrame();

                    const std::uint32_t used = (perDrawSize == 0) ? 1u : perDrawSize;
                    const std::uint32_t cbSize = AlignUp(used, 256);

                    if (fr.cbCursor + cbSize > kPerFrameCBUploadBytes)
                    {
                        throw std::runtime_error("DX12: per-frame constant upload ring overflow (increase kPerFrameCBUploadBytes)");
                    }

                    if (perDrawSize != 0)
                    {
                        std::memcpy(fr.cbMapped + fr.cbCursor, perDrawBytes.data(), perDrawSize);
                    }

                    const D3D12_GPU_VIRTUAL_ADDRESS gpuVA = fr.cbUpload->GetGPUVirtualAddress() + fr.cbCursor;
                    cmdList_->SetGraphicsRootConstantBufferView(perDrawSlot, gpuVA);

                    fr.cbCursor += cbSize;
                };

            auto ResolveTextureHandleFromDesc = [&](TextureDescIndex idx) -> TextureHandle
                {
                    if (idx == 0) // 0 = null SRV
                    {
                        return {};
                    }

                    auto it = descToTex_.find(idx);
                    if (it == descToTex_.end())
                    {
                        throw std::runtime_error("DX12: TextureDescIndex not mapped");
                    }
                    return it->second;
                };

            auto GetTextureSRV = [&](TextureHandle textureHandle) -> D3D12_GPU_DESCRIPTOR_HANDLE
                {
                    if (!textureHandle)
                    {
                        return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    }
                    auto it = textures_.find(textureHandle.id);
                    if (it == textures_.end())
                    {
                        return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    }
                    if (!it->second.hasSRV)
                    {
                        return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    }
                    return it->second.srvGpu;
                };

            auto NullBufferSRV = [&]() -> D3D12_GPU_DESCRIPTOR_HANDLE
                {
                    D3D12_GPU_DESCRIPTOR_HANDLE h = srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    h.ptr += static_cast<UINT64>(srvInc_); // SRV heap index 1
                    return h;
                };

            auto GetBufferSRV = [&](BufferHandle bufferHandle) -> D3D12_GPU_DESCRIPTOR_HANDLE
                {
                    if (!bufferHandle)
                    {
                        return NullBufferSRV();
                    }

                    auto it = buffers_.find(bufferHandle.id);
                    if (it == buffers_.end())
                    {
                        return NullBufferSRV();
                    }

                    if (!it->second.hasSRV)
                    {
                        return NullBufferSRV();
                    }

                    return it->second.srvGpu;
                };



            UINT curNumRT = 0;
            std::array<DXGI_FORMAT, 8> curRTVFormats{};
            std::fill(curRTVFormats.begin(), curRTVFormats.end(), DXGI_FORMAT_UNKNOWN);
            DXGI_FORMAT curDSVFormat = DXGI_FORMAT_UNKNOWN;
            bool curPassIsSwapChain = false;
            DX12SwapChain* curSwapChain = nullptr;

            auto TransitionTexture = [&](TextureHandle tex, D3D12_RESOURCE_STATES desired)
                {
                    if (!tex)
                    {
                        return;
                    }

                    auto it = textures_.find(tex.id);
                    if (it == textures_.end())
                    {
                        return;
                    }

                    TransitionResource(
                        cmdList_.Get(),
                        it->second.resource.Get(),
                        it->second.state,
                        desired);
                };

            auto TransitionBackBuffer = [&](DX12SwapChain& sc, D3D12_RESOURCE_STATES desired)
                {
                    TransitionResource(
                        cmdList_.Get(),
                        sc.CurrentBackBuffer(),
                        sc.CurrentBackBufferState(),
                        desired);
                };

            auto EnsurePSO = [&](PipelineHandle pipelineHandle, InputLayoutHandle layout) -> ID3D12PipelineState*
                {
                    auto PackState = [&](const GraphicsState& s) -> std::uint64_t
                        {
                            std::uint64_t v = 0;
                            std::uint32_t bit = 0;
                            auto PackBits = [&](std::uint64_t value, std::uint32_t width)
                                {
                                    const std::uint64_t mask = (width >= 64u) ? ~0ull : ((1ull << width) - 1ull);
                                    v |= (value & mask) << bit;
                                    bit += width;
                                };

                            PackBits(static_cast<std::uint32_t>(s.rasterizer.cullMode), 2);             // 0..2
                            PackBits(static_cast<std::uint32_t>(s.rasterizer.frontFace), 1);            // 2
                            PackBits(s.depth.testEnable ? 1u : 0u, 1);                                  // 3
                            PackBits(s.depth.writeEnable ? 1u : 0u, 1);                                 // 4
                            PackBits(static_cast<std::uint32_t>(s.depth.depthCompareOp), 3);            // 5..7
                            PackBits(s.blend.enable ? 1u : 0u, 1);                                      // 8

                            PackBits(s.depth.stencil.enable ? 1u : 0u, 1);                              // 9
                            PackBits(static_cast<std::uint32_t>(s.depth.stencil.readMask), 8);          // 10..17
                            PackBits(static_cast<std::uint32_t>(s.depth.stencil.writeMask), 8);         // 18..25

                            auto PackStencilFace = [&](const StencilFaceState& face)
                                {
                                    PackBits(static_cast<std::uint32_t>(face.failOp), 3);
                                    PackBits(static_cast<std::uint32_t>(face.depthFailOp), 3);
                                    PackBits(static_cast<std::uint32_t>(face.passOp), 3);
                                    PackBits(static_cast<std::uint32_t>(face.compareOp), 3);
                                };
                            PackStencilFace(s.depth.stencil.front);                                     // 26..37
                            PackStencilFace(s.depth.stencil.back);                                      // 38..49
                            return v;
                        };

                    auto Fnv1a64 = [](std::uint64_t h, std::uint64_t v) -> std::uint64_t
                        {
                            constexpr std::uint64_t kPrime = 1099511628211ull;
                            for (int i = 0; i < 8; ++i)
                            {
                                const std::uint8_t byte = static_cast<std::uint8_t>((v >> (i * 8)) & 0xffu);
                                h ^= byte;
                                h *= kPrime;
                            }
                            return h;
                        };

                    // PSO cache key MUST include: shaders, state, layout, and render-target formats.
                    std::uint64_t key = 1469598103934665603ull; // FNV-1a offset basis
                    key = Fnv1a64(key, static_cast<std::uint64_t>(pipelineHandle.id));
                    key = Fnv1a64(key, static_cast<std::uint64_t>(layout.id));
                    key = Fnv1a64(key, static_cast<std::uint64_t>(PackState(curState)));
                    key = Fnv1a64(key, static_cast<std::uint64_t>(curNumRT));
                    key = Fnv1a64(key, static_cast<std::uint64_t>(curDSVFormat));
                    for (std::size_t i = 0; i < curRTVFormats.size(); ++i)
                    {
                        key = Fnv1a64(key, static_cast<std::uint64_t>(curRTVFormats[i]));
                    }

                    if (auto it = psoCache_.find(key); it != psoCache_.end())
                    {
                        return it->second.Get();
                    }

                    auto pit = pipelines_.find(pipelineHandle.id);
                    if (pit == pipelines_.end())
                    {
                        throw std::runtime_error("DX12: pipeline handle not found");
                    }

                    auto vsIt = shaders_.find(pit->second.vs.id);
                    auto psIt = shaders_.find(pit->second.ps.id);
                    if (vsIt == shaders_.end() || psIt == shaders_.end())
                    {
                        throw std::runtime_error("DX12: shader handle not found");
                    }

                    auto layIt = layouts_.find(layout.id);
                    if (layIt == layouts_.end())
                    {
                        throw std::runtime_error("DX12: input layout handle not found");
                    }

                    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
                    pipelineDesc.pRootSignature = rootSig_.Get();

                    pipelineDesc.VS = { vsIt->second.blob->GetBufferPointer(), vsIt->second.blob->GetBufferSize() };
                    pipelineDesc.PS = { psIt->second.blob->GetBufferPointer(), psIt->second.blob->GetBufferSize() };

                    pipelineDesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
                    pipelineDesc.SampleMask = UINT_MAX;

                    // Blend
                    if (curState.blend.enable)
                    {
                        D3D12_BLEND_DESC blendDesc = CD3D12_BLEND_DESC(D3D12_DEFAULT);
                        blendDesc.AlphaToCoverageEnable = FALSE;
                        blendDesc.IndependentBlendEnable = FALSE;

                        D3D12_RENDER_TARGET_BLEND_DESC renderTartget{};
                        renderTartget.BlendEnable = TRUE;
                        renderTartget.LogicOpEnable = FALSE;
                        renderTartget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                        renderTartget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                        renderTartget.BlendOp = D3D12_BLEND_OP_ADD;
                        renderTartget.SrcBlendAlpha = D3D12_BLEND_ONE;
                        renderTartget.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                        renderTartget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                        renderTartget.LogicOp = D3D12_LOGIC_OP_NOOP;
                        renderTartget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

                        for (UINT i = 0; i < 8; ++i)
                        {
                            blendDesc.RenderTarget[i] = renderTartget;
                        }

                        pipelineDesc.BlendState = blendDesc;
                    }

                    // Rasterizer from current state
                    pipelineDesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
                    pipelineDesc.RasterizerState.CullMode = ToD3DCull(curState.rasterizer.cullMode);
                    pipelineDesc.RasterizerState.FrontCounterClockwise = (curState.rasterizer.frontFace == FrontFace::CounterClockwise) ? TRUE : FALSE;

                    // Depth / Stencil
                    pipelineDesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                    pipelineDesc.DepthStencilState.DepthEnable = curState.depth.testEnable ? TRUE : FALSE;
                    pipelineDesc.DepthStencilState.DepthWriteMask = curState.depth.writeEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
                    pipelineDesc.DepthStencilState.DepthFunc = ToD3DCompare(curState.depth.depthCompareOp);
                    pipelineDesc.DepthStencilState.StencilEnable = curState.depth.stencil.enable ? TRUE : FALSE;
                    pipelineDesc.DepthStencilState.StencilReadMask = curState.depth.stencil.readMask;
                    pipelineDesc.DepthStencilState.StencilWriteMask = curState.depth.stencil.writeMask;
                    pipelineDesc.DepthStencilState.FrontFace.StencilFailOp = ToD3DStencilOp(curState.depth.stencil.front.failOp);
                    pipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = ToD3DStencilOp(curState.depth.stencil.front.depthFailOp);
                    pipelineDesc.DepthStencilState.FrontFace.StencilPassOp = ToD3DStencilOp(curState.depth.stencil.front.passOp);
                    pipelineDesc.DepthStencilState.FrontFace.StencilFunc = ToD3DCompare(curState.depth.stencil.front.compareOp);
                    pipelineDesc.DepthStencilState.BackFace.StencilFailOp = ToD3DStencilOp(curState.depth.stencil.back.failOp);
                    pipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = ToD3DStencilOp(curState.depth.stencil.back.depthFailOp);
                    pipelineDesc.DepthStencilState.BackFace.StencilPassOp = ToD3DStencilOp(curState.depth.stencil.back.passOp);
                    pipelineDesc.DepthStencilState.BackFace.StencilFunc = ToD3DCompare(curState.depth.stencil.back.compareOp);

                    pipelineDesc.InputLayout = { layIt->second.elems.data(), static_cast<UINT>(layIt->second.elems.size()) };
                    pipelineDesc.PrimitiveTopologyType = ToD3DTopologyType(pit->second.topologyType);

                    pipelineDesc.NumRenderTargets = curNumRT;
                    for (UINT i = 0; i < curNumRT; ++i)
                    {
                        pipelineDesc.RTVFormats[i] = curRTVFormats[i];
                    }
                    pipelineDesc.DSVFormat = curDSVFormat;

                    pipelineDesc.SampleDesc.Count = 1;

                    ComPtr<ID3D12PipelineState> pso;
                    
                    if (pit->second.viewInstanceCount > 1)
                    {
                        if (!device2_)
                        {
                            // View instancing is optional; fail softly so the renderer can fallback to 6-pass.
                            return nullptr;
                        }
                        // Build PSO via Pipeline State Stream to enable View Instancing.

                        using SO_RootSig = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
                        using SO_VS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;
                        using SO_PS = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
                        using SO_Blend = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
                        using SO_SampleMask = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT>;
                        using SO_Raster = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
                        using SO_Depth = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
                        using SO_Input = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>;
                        using SO_Topo = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>;
                        using SO_RTVFmts = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
                        using SO_DSVFmt = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
                        using SO_SampleDesc = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;
                        using SO_ViewInst = PSOSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC>;

                        // Each stream subobject must be pointer-aligned, and its size should be a multiple of sizeof(void*)
                        // so the next Type is correctly aligned in the byte stream.
                        static_assert(sizeof(SO_RootSig) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_VS) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_PS) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_Blend) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_SampleMask) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_Raster) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_Depth) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_Input) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_Topo) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_RTVFmts) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_DSVFmt) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_SampleDesc) % sizeof(void*) == 0);
                        static_assert(sizeof(SO_ViewInst) % sizeof(void*) == 0);

                        const std::uint32_t viewCount = pit->second.viewInstanceCount;
                        std::array<D3D12_VIEW_INSTANCE_LOCATION, 8> locations{};
                        if (viewCount > locations.size())
                        {
                            // View instancing is optional; fail softly so the renderer can fallback to 6-pass.
                            return nullptr;
                        }
                        for (std::uint32_t i = 0; i < viewCount; ++i)
                        {
                            locations[i].RenderTargetArrayIndex = i;
                            locations[i].ViewportArrayIndex = 0;
                        }

                        D3D12_VIEW_INSTANCING_DESC viDesc{};
                        viDesc.ViewInstanceCount = viewCount;
                        viDesc.pViewInstanceLocations = locations.data();
                        viDesc.Flags = D3D12_VIEW_INSTANCING_FLAG_NONE;

                        D3D12_RT_FORMAT_ARRAY rtFmts{};
                        rtFmts.NumRenderTargets = curNumRT;
                        for (UINT i = 0; i < curNumRT; ++i)
                        {
                            rtFmts.RTFormats[i] = curRTVFormats[i];
                        }

                        struct alignas(void*) PSOStream
                        {
                            SO_RootSig    rootSig;
                            SO_VS         vs;
                            SO_PS         ps;
                            SO_Blend      blend;
                            SO_SampleMask sampleMask;
                            SO_Raster     raster;
                            SO_Depth      depth;
                            SO_Input      input;
                            SO_Topo       topo;
                            SO_RTVFmts    rtvFmts;
                            SO_DSVFmt     dsvFmt;
                            SO_SampleDesc sampleDesc;
                            SO_ViewInst   viewInst;
                        } stream{};

                        stream.rootSig.data = rootSig_.Get();
                        stream.vs.data = pipelineDesc.VS;
                        stream.ps.data = pipelineDesc.PS;
                        stream.blend.data = pipelineDesc.BlendState;
                        stream.sampleMask.data = pipelineDesc.SampleMask;
                        stream.raster.data = pipelineDesc.RasterizerState;
                        stream.depth.data = pipelineDesc.DepthStencilState;
                        stream.input.data = pipelineDesc.InputLayout;
                        stream.topo.data = pipelineDesc.PrimitiveTopologyType;
                        stream.rtvFmts.data = rtFmts;
                        stream.dsvFmt.data = pipelineDesc.DSVFormat;
                        stream.sampleDesc.data = pipelineDesc.SampleDesc;
                        stream.viewInst.data = viDesc;

                        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
                        streamDesc.SizeInBytes = sizeof(stream);
                        streamDesc.pPipelineStateSubobjectStream = &stream;

                        const HRESULT hr = device2_->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
                        if (FAILED(hr))
                        {
                            // View instancing is optional; fail softly so the renderer can fallback to 6-pass.
                            ThrowIfFailed(NativeDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pso)),
                                "DX12: CreateGraphicsPipelineState failed");
                        }
                    }
                    else
                    {
                        ThrowIfFailed(NativeDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pso)),
                            "DX12: CreateGraphicsPipelineState failed");
                    }

                    if (!pso)
                    {
                        return nullptr;
                    }

                    psoCache_[key] = pso;
                    return pso.Get();
                };

            // Parse high-level commands and record native D3D12
            for (auto& command : commandList.commands)
            {
                std::visit([&](auto&& cmd)
                    {
                        using T = std::decay_t<decltype(cmd)>;

                        if constexpr (std::is_same_v<T, CommandBeginPass>)
                        {
                            const BeginPassDesc& pass = cmd.desc;
                            const ClearDesc& c = pass.clearDesc;

                            std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtvs{};
                            UINT numRT = 0;

                            D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
                            bool hasDSV = false;

                            curNumRT = 0;
                            std::fill(curRTVFormats.begin(), curRTVFormats.end(), DXGI_FORMAT_UNKNOWN);
                            curDSVFormat = DXGI_FORMAT_UNKNOWN;

                            if (pass.frameBuffer.id == 0)
                            {
                                if (!pass.swapChain)
                                {
                                    throw std::runtime_error("DX12: CommandBeginPass: pass.swapChain is null (frameBuffer.id == 0)");
                                }

                                auto* sc = dynamic_cast<DX12SwapChain*>(pass.swapChain);
                                if (!sc)
                                {
                                    throw std::runtime_error("DX12: CommandBeginPass: pass.swapChain is not DX12SwapChain");
                                }

                                curSwapChain = sc;
                                TransitionBackBuffer(*sc, D3D12_RESOURCE_STATE_RENDER_TARGET);

                                rtvs[0] = sc->CurrentRTV();
                                numRT = 1;

                                curNumRT = numRT;
                                curRTVFormats[0] = sc->BackBufferFormat();

                                dsv = sc->DSV();
                                hasDSV = (dsv.ptr != 0);
                                curDSVFormat = sc->DepthFormat();

                                const D3D12_CPU_DESCRIPTOR_HANDLE* rtvPtr = rtvs.data();
                                const D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = hasDSV ? &dsv : nullptr;
                                cmdList_->OMSetRenderTargets(numRT, rtvPtr, FALSE, dsvPtr);

                                if (c.clearColor)
                                {
                                    const float* col = c.color.data();
                                    cmdList_->ClearRenderTargetView(rtvs[0], col, 0, nullptr);
                                }
                                const D3D12_CLEAR_FLAGS dsClearFlags =
                                    (c.clearDepth ? D3D12_CLEAR_FLAG_DEPTH : static_cast<D3D12_CLEAR_FLAGS>(0)) |
                                    (c.clearStencil ? D3D12_CLEAR_FLAG_STENCIL : static_cast<D3D12_CLEAR_FLAGS>(0));
                                if (dsClearFlags != 0 && hasDSV)
                                {
                                    cmdList_->ClearDepthStencilView(dsv, dsClearFlags, c.depth, c.stencil, 0, nullptr);
                                }

                                curPassIsSwapChain = true;
                            }
                            else
                            {
                                // ----- Offscreen framebuffer pass -----
                                auto fbIt = framebuffers_.find(pass.frameBuffer.id);
                                if (fbIt == framebuffers_.end())
                                {
                                    throw std::runtime_error("DX12: CommandBeginPass: framebuffer not found");
                                }

                                const FramebufferEntry& fb = fbIt->second;

                                // Color (0 or 1 RT)
                                if (fb.color)
                                {
                                    auto it = textures_.find(fb.color.id);
                                    if (it == textures_.end())
                                    {
                                        throw std::runtime_error("DX12: CommandBeginPass: framebuffer color texture not found");
                                    }

                                    auto& te = it->second;

                                    if (fb.colorCubeAllFaces)
                                    {
                                        if (!te.hasRTVAllFaces)
                                        {
                                            throw std::runtime_error("DX12: CommandBeginPass: cubemap color texture has no RTV (all faces)");
                                        }
                                    
                                        TransitionResource(
                                            cmdList_.Get(),
                                            te.resource.Get(),
                                            te.state,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
                                    
                                        rtvs[0] = te.rtvAllFaces;
                                        numRT = 1;
                                        curRTVFormats[0] = te.rtvFormat;
                                    }
                                    else if (fb.colorCubeFace != 0xFFFFFFFFu)
                                    {
                                        if (!te.hasRTVFaces)
                                        {
                                            throw std::runtime_error("DX12: CommandBeginPass: cubemap color texture has no RTV faces");
                                        }
                                        if (fb.colorCubeFace >= 6)
                                        {
                                            throw std::runtime_error("DX12: CommandBeginPass: cubemap face index out of range");
                                        }
                                    
                                        TransitionResource(
                                            cmdList_.Get(),
                                            te.resource.Get(),
                                            te.state,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
                                    
                                        rtvs[0] = te.rtvFaces[fb.colorCubeFace];
                                        numRT = 1;
                                        curRTVFormats[0] = te.rtvFormat;
                                    }
                                    else
                                    {
                                        if (!te.hasRTV)
                                        {
                                            throw std::runtime_error("DX12: CommandBeginPass: color texture has no RTV");
                                        }
                                    
                                        TransitionResource(
                                            cmdList_.Get(),
                                            te.resource.Get(),
                                            te.state,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
                                    
                                        rtvs[0] = te.rtv;
                                        numRT = 1;
                                        curRTVFormats[0] = te.rtvFormat;
                                    }
                                }
                                // Depth
                                if (fb.depth)
                                {
                                    auto it = textures_.find(fb.depth.id);
                                    if (it == textures_.end())
                                    {
                                        throw std::runtime_error("DX12: CommandBeginPass: framebuffer depth texture not found");
                                    }
                                
                                    auto& te = it->second;
                                    
                                    if (fb.colorCubeAllFaces && te.hasDSVAllFaces)
                                    {
                                        TransitionResource(
                                            cmdList_.Get(),
                                            te.resource.Get(),
                                            te.state,
                                            D3D12_RESOURCE_STATE_DEPTH_WRITE);
                                    
                                        dsv = te.dsvAllFaces;
                                        hasDSV = true;
                                        curDSVFormat = te.dsvFormat;
                                    }
                                    else
                                    {
                                        if (!te.hasDSV)
                                        {
                                            throw std::runtime_error("DX12: CommandBeginPass: depth texture has no DSV");
                                        }
                                    
                                        TransitionResource(
                                            cmdList_.Get(),
                                            te.resource.Get(),
                                            te.state,
                                            D3D12_RESOURCE_STATE_DEPTH_WRITE);
                                    
                                        dsv = te.dsv;
                                        hasDSV = true;
                                        curDSVFormat = te.dsvFormat;
                                    }
                                }

                                curPassIsSwapChain = false;
                                curSwapChain = nullptr;

                                curNumRT = numRT;

                                // Bind RT/DSV
                                const D3D12_CPU_DESCRIPTOR_HANDLE* rtvPtr = (numRT > 0) ? rtvs.data() : nullptr;
                                const D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = (hasDSV) ? &dsv : nullptr;
                                cmdList_->OMSetRenderTargets(numRT, rtvPtr, FALSE, dsvPtr);

                                // Clear
                                if (c.clearColor && numRT > 0)
                                {
                                    const float* col = c.color.data();
                                    for (UINT i = 0; i < numRT; ++i)
                                    {
                                        cmdList_->ClearRenderTargetView(rtvs[i], col, 0, nullptr);
                                    }
                                }

                                const D3D12_CLEAR_FLAGS dsClearFlags =
                                    (c.clearDepth ? D3D12_CLEAR_FLAG_DEPTH : static_cast<D3D12_CLEAR_FLAGS>(0)) |
                                    (c.clearStencil ? D3D12_CLEAR_FLAG_STENCIL : static_cast<D3D12_CLEAR_FLAGS>(0));
                                if (dsClearFlags != 0 && hasDSV)
                                {
                                    cmdList_->ClearDepthStencilView(dsv, dsClearFlags, c.depth, c.stencil, 0, nullptr);
                                }
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandEndPass>)
                        {
                            if (curPassIsSwapChain)
                            {
                                if (!curSwapChain)
                                {
                                    throw std::runtime_error("DX12: CommandEndPass: curSwapChain is null");
                                }
                                TransitionBackBuffer(*curSwapChain, D3D12_RESOURCE_STATE_PRESENT);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandSetViewport>)
                        {
                            D3D12_VIEWPORT viewport{};
                            viewport.TopLeftX = static_cast<float>(cmd.x);
                            viewport.TopLeftY = static_cast<float>(cmd.y);
                            viewport.Width = static_cast<float>(cmd.width);
                            viewport.Height = static_cast<float>(cmd.height);
                            viewport.MinDepth = 0.0f;
                            viewport.MaxDepth = 1.0f;
                            cmdList_->RSSetViewports(1, &viewport);

                            D3D12_RECT scissor{};
                            scissor.left = cmd.x;
                            scissor.top = cmd.y;
                            scissor.right = cmd.x + cmd.width;
                            scissor.bottom = cmd.y + cmd.height;
                            cmdList_->RSSetScissorRects(1, &scissor);
                        }
                        else if constexpr (std::is_same_v<T, CommandSetState>)
                        {
                            curState = cmd.state;
                        }
                        else if constexpr (std::is_same_v<T, CommandSetStencilRef>)
                        {
                            cmdList_->OMSetStencilRef(static_cast<UINT>(cmd.ref & 0xFFu));
                         }
                        else if constexpr (std::is_same_v<T, CommandSetPrimitiveTopology>)
                        {
                            currentTopology = ToD3DTopology(cmd.topology);
                        }
                        else if constexpr (std::is_same_v<T, CommandBindPipeline>)
                        {
                            curPipe = cmd.pso;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindInputLayout>)
                        {
                            curLayout = cmd.layout;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindVertexBuffer>)
                        {
                            if (cmd.slot >= kMaxVBSlots)
                            {
                                throw std::runtime_error("DX12: BindVertexBuffer: slot out of range");
                            }
                            vertexBuffers[cmd.slot] = cmd.buffer;
                            vbStrides[cmd.slot] = cmd.strideBytes;
                            vbOffsets[cmd.slot] = cmd.offsetBytes;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindIndexBuffer>)
                        {
                            indexBuffer = cmd.buffer;
                            ibType = cmd.indexType;
                            ibOffset = cmd.offsetBytes;
                        }
                        else if constexpr (std::is_same_v<T, CommnadBindTexture2D>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                auto it = textures_.find(cmd.texture.id);
                                if (it == textures_.end())
                                {
                                    throw std::runtime_error("DX12: BindTexture2D: texture not found in textures_ map");
                                }

                                if (!it->second.hasSRV)
                                {
                                    throw std::runtime_error("DX12: BindTexture2D: texture has no SRV");
                                }

                                TransitionTexture(cmd.texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                boundTex[cmd.slot] = GetTextureSRV(cmd.texture);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandBindTextureCube>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                auto it = textures_.find(cmd.texture.id);
                                if (it == textures_.end())
                                {
                                    throw std::runtime_error("DX12: BindTextureCube: texture not found in textures_ map");
                                }

                                if (!it->second.hasSRV)
                                {
                                    throw std::runtime_error("DX12: BindTextureCube: texture has no SRV");
                                }

                                TransitionTexture(cmd.texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                boundTex[cmd.slot] = GetTextureSRV(cmd.texture);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandTextureDesc>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                TextureHandle handle = ResolveTextureHandleFromDesc(cmd.texture);
                                if (!handle)
                                {
                                    // null SRV
                                    boundTex[cmd.slot] = srvHeap_->GetGPUDescriptorHandleForHeapStart();
                                    return;
                                }

                                TransitionTexture(handle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                boundTex[cmd.slot] = GetTextureSRV(handle);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandBindStructuredBufferSRV>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                boundTex[cmd.slot] = GetBufferSRV(cmd.buffer);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandSetUniformInt> ||
                            std::is_same_v<T, CommandUniformFloat4> ||
                            std::is_same_v<T, CommandUniformMat4>)
                        {
                            // DX12 backend does not interpret the name-based uniform commands.
                            // Use CommandSetConstants instead.
                        }
                        else if constexpr (std::is_same_v<T, CommandSetConstants>)
                        {
                            perDrawSlot = cmd.slot;
                            perDrawSize = cmd.size;
                            if (perDrawSize > kMaxPerDrawConstantsBytes)
                            {
                                perDrawSize = kMaxPerDrawConstantsBytes;
                            }
                            if (perDrawSize != 0)
                            {
                                std::memcpy(perDrawBytes.data(), cmd.data.data(), perDrawSize);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandDrawIndexed>)
                        {
                            // PSO + RootSig
                            ID3D12PipelineState* pso = EnsurePSO(curPipe, curLayout);
                            cmdList_->SetPipelineState(pso);
                            cmdList_->SetGraphicsRootSignature(rootSig_.Get());

                            // IA bindings (slot0..slotN based on input layout)
                            auto layIt = layouts_.find(curLayout.id);
                            if (layIt == layouts_.end())
                            {
                                throw std::runtime_error("DX12: input layout handle not found");
                            }

                            std::uint32_t maxSlot = 0;
                            for (const auto& e : layIt->second.elems)
                            {
                                maxSlot = std::max(maxSlot, static_cast<std::uint32_t>(e.InputSlot));
                            }
                            const std::uint32_t numVB = maxSlot + 1;
                            if (numVB > kMaxVBSlots)
                            {
                                throw std::runtime_error("DX12: input layout uses more VB slots than supported");
                            }

                            std::array<D3D12_VERTEX_BUFFER_VIEW, kMaxVBSlots> vbv{};
                            for (std::uint32_t s = 0; s < numVB; ++s)
                            {
                                if (!vertexBuffers[s])
                                {
                                    throw std::runtime_error("DX12: missing vertex buffer binding for required slot");
                                }
                                auto vbIt = buffers_.find(vertexBuffers[s].id);
                                if (vbIt == buffers_.end())
                                {
                                    throw std::runtime_error("DX12: vertex buffer not found");
                                }

                                const std::uint32_t off = vbOffsets[s];
                                vbv[s].BufferLocation = vbIt->second.resource->GetGPUVirtualAddress() + off;
                                vbv[s].SizeInBytes = (UINT)(vbIt->second.desc.sizeInBytes - off);
                                vbv[s].StrideInBytes = vbStrides[s];
                            }
                            cmdList_->IASetVertexBuffers(0, numVB, vbv.data());
                            cmdList_->IASetPrimitiveTopology(currentTopology);

                            if (indexBuffer)
                            {
                                auto ibIt = buffers_.find(indexBuffer.id);
                                if (ibIt == buffers_.end())
                                {
                                    throw std::runtime_error("DX12: index buffer not found");
                                }
                                D3D12_INDEX_BUFFER_VIEW ibv{};
                                ibv.BufferLocation = ibIt->second.resource->GetGPUVirtualAddress() + ibOffset
                                    + static_cast<UINT64>(cmd.firstIndex) * static_cast<UINT64>(IndexSizeBytes(cmd.indexType));
                                ibv.SizeInBytes = static_cast<UINT>(ibIt->second.desc.sizeInBytes - ibOffset);
                                ibv.Format = (cmd.indexType == IndexType::UINT16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

                                cmdList_->IASetIndexBuffer(&ibv);
                            }

                            // Root bindings: CBV (0) + SRV table (1)
                            WriteCBAndBind();

                            for (UINT i = 0; i < kMaxSRVSlots; ++i)
                            {
                                cmdList_->SetGraphicsRootDescriptorTable(1 + i, boundTex[i]);
                            }

                            cmdList_->DrawIndexedInstanced(cmd.indexCount, cmd.instanceCount, 0, cmd.baseVertex, cmd.firstInstance);
                        }
                        else if constexpr (std::is_same_v<T, CommandDraw>)
                        {
                            ID3D12PipelineState* pso = EnsurePSO(curPipe, curLayout);
                            cmdList_->SetPipelineState(pso);
                            cmdList_->SetGraphicsRootSignature(rootSig_.Get());

                            // IA bindings (slot0..slotN based on input layout)
                            auto layIt = layouts_.find(curLayout.id);
                            if (layIt == layouts_.end())
                            {
                                throw std::runtime_error("DX12: input layout handle not found");
                            }

                            std::uint32_t maxSlot = 0;
                            for (const auto& e : layIt->second.elems)
                            {
                                maxSlot = std::max(maxSlot, static_cast<std::uint32_t>(e.InputSlot));
                            }
                            const std::uint32_t numVB = maxSlot + 1;
                            if (numVB > kMaxVBSlots)
                            {
                                throw std::runtime_error("DX12: input layout uses more VB slots than supported");
                            }

                            std::array<D3D12_VERTEX_BUFFER_VIEW, kMaxVBSlots> vbv{};
                            for (std::uint32_t s = 0; s < numVB; ++s)
                            {
                                if (!vertexBuffers[s])
                                {
                                    throw std::runtime_error("DX12: missing vertex buffer binding for required slot");
                                }
                                auto vbIt = buffers_.find(vertexBuffers[s].id);
                                if (vbIt == buffers_.end())
                                {
                                    throw std::runtime_error("DX12: vertex buffer not found");
                                }

                                const std::uint32_t off = vbOffsets[s];
                                vbv[s].BufferLocation = vbIt->second.resource->GetGPUVirtualAddress() + off;
                                vbv[s].SizeInBytes = (UINT)(vbIt->second.desc.sizeInBytes - off);
                                vbv[s].StrideInBytes = vbStrides[s];
                            }
                            cmdList_->IASetVertexBuffers(0, numVB, vbv.data());
                            cmdList_->IASetPrimitiveTopology(currentTopology);

                            WriteCBAndBind();

                            for (UINT i = 0; i < kMaxSRVSlots; ++i)
                            {
                                cmdList_->SetGraphicsRootDescriptorTable(1 + i, boundTex[i]);
                            }

                            cmdList_->DrawInstanced(cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance);
                        }
                        else if constexpr (std::is_same_v<T, CommandDX12ImGuiRender>)
                        {
                            if (!imguiInitialized_ || !cmd.drawData)
                            {
                                return;
                            }

                            // Ensure ImGui sees the same shader-visible heap.
                            ID3D12DescriptorHeap* heaps[] = { srvHeap_.Get() };
                            cmdList_->SetDescriptorHeaps(1, heaps);

                            ImGui_ImplDX12_RenderDrawData(reinterpret_cast<ImDrawData*>(const_cast<void*>(cmd.drawData)), cmdList_.Get());
                        }
                        else if constexpr (std::is_same_v<T, CommandBindTexture2DArray>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                auto it = textures_.find(cmd.texture.id);
                                if (it == textures_.end())
                                {
                                    throw std::runtime_error("DX12: BindTexture2DArray: texture not found in textures_ map");
                                }

                                // Ensure an Array SRV exists for cube textures.
                                if (!it->second.hasSRVArray)
                                {
                                    const auto desc = it->second.resource->GetDesc();
                                    AllocateSRV_CubeAsArray(it->second, it->second.srvFormat, desc.MipLevels);
                                }

                                TransitionTexture(cmd.texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                boundTex[cmd.slot] = it->second.srvGpuArray;
                            }
                        }
                        else
                        {
                            // other commands ignored
                        }

                    }, command);
            }

            // Close + execute + signal fence for the current frame resource
            EndFrame();
        }

