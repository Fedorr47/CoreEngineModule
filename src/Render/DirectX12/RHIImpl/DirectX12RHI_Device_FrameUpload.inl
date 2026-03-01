        void WaitForFence(UINT64 v)
        {
            if (v == 0)
                return;

            if (fence_->GetCompletedValue() < v)
            {
                ThrowIfFailed(fence_->SetEventOnCompletion(v, fenceEvent_), "DX12: SetEventOnCompletion failed");
                WaitForSingleObject(fenceEvent_, INFINITE);
            }
        }

        void TransitionResource(
            ID3D12GraphicsCommandList* cmdList,
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES& currentState,
            D3D12_RESOURCE_STATES desired)
        {
            if (!cmdList || !resource)
            {
                return;
            }

            if (currentState == desired)
            {
                return;
            }

            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = resource;
            barrier.Transition.StateBefore = currentState;
            barrier.Transition.StateAfter = desired;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            cmdList->ResourceBarrier(1, &barrier);
            currentState = desired;
        }

        void ImmediateUploadBuffer(BufferEntry& dst, std::span<const std::byte> data, std::size_t dstOffsetBytes)
        {
            if (!dst.resource || data.empty())
                return;

            // Temp upload resource
            ComPtr<ID3D12Resource> upload;

            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = std::max<UINT64>(1, static_cast<UINT64>(data.size()));
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            ThrowIfFailed(NativeDevice()->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&upload)),
                "DX12: ImmediateUploadBuffer - Create upload resource failed");

            void* mapped = nullptr;
            ThrowIfFailed(upload->Map(0, nullptr, &mapped), "DX12: ImmediateUploadBuffer - Map upload failed");
            std::memcpy(mapped, data.data(), data.size());
            upload->Unmap(0, nullptr);

            // Record tiny copy list
            ComPtr<ID3D12CommandAllocator> alloc;
            ThrowIfFailed(NativeDevice()->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&alloc)),
                "DX12: ImmediateUploadBuffer - CreateCommandAllocator failed");

            ComPtr<ID3D12GraphicsCommandList> cl;
            ThrowIfFailed(NativeDevice()->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                alloc.Get(),
                nullptr,
                IID_PPV_ARGS(&cl)),
                "DX12: ImmediateUploadBuffer - CreateCommandList failed");

            TransitionResource(
                cl.Get(),
                dst.resource.Get(),
                dst.state,
                D3D12_RESOURCE_STATE_COPY_DEST);

            cl->CopyBufferRegion(
                dst.resource.Get(),
                static_cast<UINT64>(dstOffsetBytes),
                upload.Get(),
                0,
                static_cast<UINT64>(data.size()));

            TransitionResource(
                cl.Get(),
                dst.resource.Get(),
                dst.state,
                D3D12_RESOURCE_STATE_GENERIC_READ);

            ThrowIfFailed(cl->Close(), "DX12: ImmediateUploadBuffer - Close failed");

            ID3D12CommandList* lists[] = { cl.Get() };
            NativeQueue()->ExecuteCommandLists(1, lists);

            const UINT64 v = ++fenceValue_;
            ThrowIfFailed(NativeQueue()->Signal(fence_.Get(), v), "DX12: ImmediateUploadBuffer - Signal failed");
            WaitForFence(v);
        }

        void FlushPendingBufferUpdates()
        {
            if (pendingBufferUpdates_.empty())
                return;

            FrameResource& fr = CurrentFrame();

            for (const PendingBufferUpdate& u : pendingBufferUpdates_)
            {
                auto it = buffers_.find(u.buffer.id);
                if (it == buffers_.end()) continue;

                BufferEntry& dst = it->second;
                if (!dst.resource || u.data.empty()) continue;

                const std::uint32_t size = static_cast<std::uint32_t>(u.data.size());
                const std::uint32_t aligned = AlignUp(size, 16u);

                if (fr.bufCursor + aligned > kPerFrameBufUploadBytes)
                {
                    throw std::runtime_error("DX12: per-frame buffer upload ring overflow (increase kPerFrameBufUploadBytes)");
                }

                std::memcpy(fr.bufMapped + fr.bufCursor, u.data.data(), size);

                TransitionResource(
                    cmdList_.Get(),
                    dst.resource.Get(),
                    dst.state,
                    D3D12_RESOURCE_STATE_COPY_DEST);

                cmdList_->CopyBufferRegion(
                    dst.resource.Get(),
                    static_cast<UINT64>(u.dstOffsetBytes),
                    fr.bufUpload.Get(),
                    static_cast<UINT64>(fr.bufCursor),
                    static_cast<UINT64>(size));

                TransitionResource(
                    cmdList_.Get(),
                    dst.resource.Get(),
                    dst.state,
                    D3D12_RESOURCE_STATE_GENERIC_READ);

                fr.bufCursor += aligned;
            }

            pendingBufferUpdates_.clear();
        }

        FrameResource& CurrentFrame() noexcept
        {
            return frames_[activeFrameIndex_];
        }

        void BeginFrame()
        {
            activeFrameIndex_ = static_cast<std::uint32_t>(submitIndex_ % kFramesInFlight);
            ++submitIndex_;

            FrameResource& fr = frames_[activeFrameIndex_];

            // Wait until GPU is done with this frame resource, then recycle deferred objects/indices.
            WaitForFence(fr.fenceValue);
            fr.ReleaseDeferred(freeSrv_, freeRTV_, freeDSV_);

            ThrowIfFailed(fr.cmdAlloc->Reset(), "DX12: cmdAlloc reset failed");
            ThrowIfFailed(cmdList_->Reset(fr.cmdAlloc.Get(), nullptr), "DX12: cmdList reset failed");

            fr.ResetForRecording();
        }

        void EndFrame()
        {
            ThrowIfFailed(cmdList_->Close(), "DX12: cmdList close failed");

            ID3D12CommandList* lists[] = { cmdList_.Get() };
            NativeQueue()->ExecuteCommandLists(1, lists);

            const UINT64 v = ++fenceValue_;
            ThrowIfFailed(NativeQueue()->Signal(fence_.Get(), v), "DX12: Signal failed");
            frames_[activeFrameIndex_].fenceValue = v;
        }

        void FlushGPU()
        {
            const UINT64 v = ++fenceValue_;
            ThrowIfFailed(NativeQueue()->Signal(fence_.Get(), v), "DX12: Signal failed");
            WaitForFence(v);
        }
