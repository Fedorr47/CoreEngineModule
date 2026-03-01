        TextureDescIndex AllocateTextureDesctiptor(TextureHandle texture) override
        {
            // 0 = invalid
            TextureDescIndex idx{};
            if (!freeTexDesc_.empty())
            {
                idx = freeTexDesc_.back();
                freeTexDesc_.pop_back();
            }
            else
            {
                idx = TextureDescIndex{ nextTexDesc_++ };
            }

            UpdateTextureDescriptor(idx, texture);
            return idx;
        }

        void UpdateTextureDescriptor(TextureDescIndex idx, TextureHandle tex) override
        {
            if (!tex)
            {
                descToTex_[idx] = {};
                return;
            }

            descToTex_[idx] = tex;

            auto it = textures_.find(tex.id);
            if (it == textures_.end())
            {
                throw std::runtime_error("DX12: UpdateTextureDescriptor: texture not found");
            }

            auto& te = it->second;
            if (!te.hasSRV)
            {
                if (te.srvFormat == DXGI_FORMAT_UNKNOWN)
                {
                    throw std::runtime_error("DX12: UpdateTextureDescriptor: texture has no SRV format");
                }

                AllocateSRV(te, te.srvFormat, /*mips*/ 1);
            }
        }

        void FreeTextureDescriptor(TextureDescIndex index) noexcept override
        {
            descToTex_.erase(index);
            freeTexDesc_.push_back(index);
        }

        // ---------------- Fences (minimal impl) ----------------
        FenceHandle CreateFence(bool signaled = false) override
        {
            const auto id = ++nextFenceId_;
            fences_[id] = signaled;
            return FenceHandle{ id };
        }

        void DestroyFence(FenceHandle fence) noexcept override
        {
            fences_.erase(fence.id);
        }

        void SignalFence(FenceHandle fence) override
        {
            fences_[fence.id] = true;
        }

        void WaitFence(FenceHandle) override {}

        bool IsFenceSignaled(FenceHandle fence) override
        {
            auto it = fences_.find(fence.id);
            return it != fences_.end() && it->second;
        }

        ID3D12Device* NativeDevice() const
        {
            return core_.device.Get();
        }

        ID3D12CommandQueue* NativeQueue() const
        {
            return core_.cmdQueue.Get();
        }

        ID3D12DescriptorHeap* NativeSRVHeap() const
        {
            return srvHeap_.Get();
        }

        UINT NativeSRVInc() const
        {
            return srvInc_;
        }
