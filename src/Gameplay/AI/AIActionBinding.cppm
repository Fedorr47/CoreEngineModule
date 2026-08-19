module;

#include <memory>
#include <string_view>
#include <unordered_map>

export module core:ai_action_binding;

export import :ai_action_runtime;

export namespace rendern
{
    class IAIActionBinding
    {
    public:
        virtual ~IAIActionBinding() = default;
        
        [[nodiscard]] virtual std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context) = 0;
    };
    
    // Bindings are non-owning entries and must outlive their registration
    class AIActionBindingRegistry
    {
    public:
        [[nodiscard]] bool Register(const AIActionId actionId, IAIActionBinding& binding) noexcept
        {
            if (!actionId.IsValid())
            {
                return false;
            }
            return bindings_.emplace(actionId.value, &binding).second;
        }
        
        [[nodiscard]] IAIActionBinding* Find(const AIActionId actionId) const noexcept
        {
            const auto it = bindings_.find(actionId.value);
            return it == bindings_.end() ? nullptr : it->second;
        }
        
        [[nodiscard]] bool Contains(const AIActionId actionId) const noexcept
        {
            return Find(actionId) != nullptr;
        }
        
        [[nodiscard]] bool Remove(const AIActionId actionId) noexcept
        {
            return bindings_.erase(actionId.value) != 0u;
        }
        
        void Reset() noexcept
        {
            bindings_.clear();
        }
        
    private:
        std::unordered_map<AIActionId::ValueType, IAIActionBinding*> bindings_{};
    };
}