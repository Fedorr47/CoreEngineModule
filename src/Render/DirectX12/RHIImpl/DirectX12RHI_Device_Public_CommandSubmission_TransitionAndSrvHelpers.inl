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