# GameplayAIDecisionInstance

**class** · домен «Решение (instance)» · `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:31–38` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 34 | метод | public | `virtual ~GameplayAIDecisionInstance() = default;` |
| 35 | метод | public | `virtual void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) = 0;` |
| 36 | метод | public | `virtual void Cancel(AISystem& aiSystem) noexcept = 0;` |
| 37 | метод | public | `[[nodiscard]] virtual GameplayAIDecisionStatus GetStatus() const noexcept = 0;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:31–38`.

```cpp linenums="31"
    class GameplayAIDecisionInstance
    {
    public:
        virtual ~GameplayAIDecisionInstance() = default;
        virtual void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) = 0;
        virtual void Cancel(AISystem& aiSystem) noexcept = 0;
        [[nodiscard]] virtual GameplayAIDecisionStatus GetStatus() const noexcept = 0;
    };
```
