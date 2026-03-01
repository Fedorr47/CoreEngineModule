        BufferHandle CreateBuffer(const BufferDesc& desc) override
        {
            BufferHandle handle{ ++nextBufId_ };
            BufferEntry bufferEntry{};
            bufferEntry.desc = desc;

            const UINT64 sz = static_cast<UINT64>(desc.sizeInBytes);

            // GPU-local buffer (DEFAULT heap). Updates happen via per-frame upload ring.
            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = std::max<UINT64>(1, sz);
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            const D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;

            ThrowIfFailed(NativeDevice()->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                initState,
                nullptr,
                IID_PPV_ARGS(&bufferEntry.resource)),
                "DX12: CreateBuffer failed");

            bufferEntry.state = initState;

            if (desc.bindFlag == BufferBindFlag::StructuredBuffer)
            {
                AllocateStructuredBufferSRV(bufferEntry);
            }

            buffers_[handle.id] = std::move(bufferEntry);
            return handle;
        }

        void UpdateBuffer(BufferHandle buffer, std::span<const std::byte> data, std::size_t offsetBytes = 0) override
        {
            if (!buffer || data.empty())
            {
                return;
            }

            auto it = buffers_.find(buffer.id);
            if (it == buffers_.end())
            {
                return;
            }

            BufferEntry& entry = it->second;

            const std::size_t end = offsetBytes + data.size();
            if (end > entry.desc.sizeInBytes)
                throw std::runtime_error("DX12: UpdateBuffer out of bounds");

            // If we haven't submitted anything yet, it's safe to do a blocking upload.
            if (!hasSubmitted_)
            {
                ImmediateUploadBuffer(entry, data, offsetBytes);
                return;
            }

            PendingBufferUpdate u{};
            u.buffer = buffer;
            u.dstOffsetBytes = offsetBytes;
            u.data.assign(data.begin(), data.end());
            pendingBufferUpdates_.push_back(std::move(u));
        }

        void DestroyBuffer(BufferHandle buffer) noexcept override
        {
            if (buffer.id == 0)
            {
                return;
            }

            auto it = buffers_.find(buffer.id);
            if (it == buffers_.end())
            {
                return;
            }

            BufferEntry entry = std::move(it->second);
            buffers_.erase(it);

            // Remove pending updates for this buffer.
            if (!pendingBufferUpdates_.empty())
            {
                pendingBufferUpdates_.erase(
                    std::remove_if(pendingBufferUpdates_.begin(), pendingBufferUpdates_.end(),
                        [&](const PendingBufferUpdate& u) { return u.buffer.id == buffer.id; }),
                    pendingBufferUpdates_.end());
            }

            if (entry.resource && hasSubmitted_)
            {
                CurrentFrame().deferredResources.push_back(std::move(entry.resource));
            }

            if (entry.hasSRV && entry.srvIndex != 0)
            {
                if (hasSubmitted_)
                {
                    CurrentFrame().deferredFreeSrv.push_back(entry.srvIndex);
                }
                else
                {
                    freeSrv_.push_back(entry.srvIndex);
                }
            }

            if (entry.hasSRVArray && entry.srvIndexArray != 0)
            {
                if (hasSubmitted_)
                {
                    CurrentFrame().deferredFreeSrv.push_back(entry.srvIndexArray);
                }
                else
                {
                    freeSrv_.push_back(entry.srvIndexArray);
                }
            }
        }

