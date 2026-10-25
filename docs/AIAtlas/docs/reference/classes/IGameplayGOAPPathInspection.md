# IGameplayGOAPPathInspection

**class** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:47–54` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 50 | метод | public | `virtual ~IGameplayGOAPPathInspection() = default;` |
| 53 | метод | public | `BuildPlannedPathDebugView() const = 0;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:47–54`.

```cpp linenums="47"
    class IGameplayGOAPPathInspection
    {
    public:
        virtual ~IGameplayGOAPPathInspection() = default;

        [[nodiscard]] virtual GameplayAIDebugPlannedPathView
            BuildPlannedPathDebugView() const = 0;
    };
```
