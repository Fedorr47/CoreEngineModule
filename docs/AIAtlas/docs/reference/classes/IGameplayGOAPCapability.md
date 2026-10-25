# IGameplayGOAPCapability

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:52–59` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 55 | метод | public | `virtual ~IGameplayGOAPCapability() = default;` |
| 56 | метод | public | `[[nodiscard]] virtual std::vector<GameplayGOAPActionCostOverride> CostOverrides() const { return {}; }` |
| 57 | метод | public | `[[nodiscard]] virtual std::unique_ptr<IAIActionBinding> CreateBinding(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm:52–59`.

```cpp linenums="52"
    class IGameplayGOAPCapability
    {
    public:
        virtual ~IGameplayGOAPCapability() = default;
        [[nodiscard]] virtual std::vector<GameplayGOAPActionCostOverride> CostOverrides() const { return {}; }
        [[nodiscard]] virtual std::unique_ptr<IAIActionBinding> CreateBinding(
            AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& events) = 0;
    };
```
