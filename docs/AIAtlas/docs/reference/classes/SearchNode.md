# SearchNode

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIPlanner.cppm:117–123` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 119 | поле | public | `AIAgentWorldState state{};` |
| 120 | поле | public | `float cost{};` |
| 121 | поле | public | `std::uint64_t sequence{};` |
| 122 | поле | public | `std::vector<AIPlanStep> steps{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanner.cppm:117–123`.

```cpp linenums="117"
        struct SearchNode
        {
            AIAgentWorldState state{};
            float cost{};
            std::uint64_t sequence{};
            std::vector<AIPlanStep> steps{};
        };
```
