# SearchNodeGreater

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIPlanner.cppm:125–135` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanner.cppm:125–135`.

```cpp linenums="125"
        struct SearchNodeGreater
        {
            [[nodiscard]] bool operator()(const SearchNode& left, const SearchNode& right) const noexcept
            {
                if (left.cost != right.cost)
                {
                    return left.cost > right.cost;
                }
                return left.sequence > right.sequence;
            }
        };
```
