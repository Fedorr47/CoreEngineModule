# AIFactEffect

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:32–38` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 34 | поле | public | `AIWorldFactId factId{};` |
| 35 | поле | public | `bool bValue{ true };` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:32–38`.

```cpp linenums="32"
    struct AIFactEffect
    {
        AIWorldFactId factId{};
        bool bValue{ true };

        friend constexpr bool operator==(const AIFactEffect&, const AIFactEffect&) noexcept = default;
    };
```
