    // NOTE:
    // Pipeline State Stream parsing requires each subobject to be aligned to sizeof(void*)
    // and for the stream layout to be well-formed. A common source of E_INVALIDARG is a
    // custom subobject wrapper that doesn't add trailing padding so the next subobject's
    // Type field starts at a pointer-aligned offset.
    
    template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename T>
    struct alignas(void*) PSOSubobject
    {
        static constexpr std::size_t kAlign = sizeof(void*);
        static constexpr std::size_t kBaseSize = sizeof(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE) + sizeof(T);
        static constexpr std::size_t kPaddedSize = ((kBaseSize + (kAlign - 1)) / kAlign) * kAlign;
        static constexpr std::size_t kPadSize = kPaddedSize - kBaseSize;
    
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ Type };
        T data{};
    };

    class DX12Device final : public IRHIDevice
    {
    public:
#include "DirectX12RHI_Device_Public.inl"

    private:
#include "DirectX12RHI_Device_PrivateHelpers.inl"
#include "DirectX12RHI_Device_State.inl"
    };
