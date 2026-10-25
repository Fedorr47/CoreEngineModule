# GameplayAIDecisionFactoryRegistry

**class** · домен «Решение (instance)» · `src/Gameplay/AI/Runtime/GameplayAIDecision.cppm:23–63` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 26 | метод | public | `[[nodiscard]] bool Register(` |
| 36 | метод | public | `[[nodiscard]] bool Contains(const std::string_view definitionId) const noexcept` |
| 41 | метод | public | `[[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> Create(` |
| 62 | поле | private | `std::map<std::string, GameplayAIDecisionFactory, std::less<>> factories_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/GameplayAIDecision.cppm:23–63`.

```cpp linenums="23"
    class GameplayAIDecisionFactoryRegistry
    {
    public:
        [[nodiscard]] bool Register(
            const std::string_view definitionId, GameplayAIDecisionFactory factory)
        {
            if (definitionId.empty() || !factory)
            {
                return false;
            }
            return factories_.emplace(std::string{definitionId}, std::move(factory)).second;
        }

        [[nodiscard]] bool Contains(const std::string_view definitionId) const noexcept
        {
            return factories_.contains(definitionId);
        }

        [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> Create(
            const std::string_view definitionId,
            const GameplayAIDecisionCreationContext& context) const
        {
            const auto found = factories_.find(definitionId);
            if (context.diagnostic != nullptr)
            {
                context.diagnostic->clear();
            }
            if (found == factories_.end())
            {
                if (context.diagnostic != nullptr)
                {
                    *context.diagnostic = "Unknown AI decision '" + std::string(definitionId) + "'";
                }
                return nullptr;
            }
            return found->second(context);
        }

    private:
        std::map<std::string, GameplayAIDecisionFactory, std::less<>> factories_{};
    };
```
