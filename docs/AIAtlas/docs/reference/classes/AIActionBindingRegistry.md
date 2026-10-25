# AIActionBindingRegistry

**class** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionBinding.cppm:24–59` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Action runtime».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 27 | метод | public | `[[nodiscard]] bool Register(const AIActionId actionId, IAIActionBinding& binding) noexcept` |
| 36 | метод | public | `[[nodiscard]] IAIActionBinding* Find(const AIActionId actionId) const noexcept` |
| 42 | метод | public | `[[nodiscard]] bool Contains(const AIActionId actionId) const noexcept` |
| 47 | метод | public | `[[nodiscard]] bool Remove(const AIActionId actionId) noexcept` |
| 52 | метод | public | `void Reset() noexcept` |
| 58 | поле | private | `std::unordered_map<AIActionId::ValueType, IAIActionBinding*> bindings_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionBinding.cppm:24–59`.

```cpp linenums="24"
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
```
