# AIDebugIntegerFactView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:28–33` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 30 | поле | public | `AIWorldIntegerFactId factId{};` |
| 31 | поле | public | `std::int32_t value{};` |
| 32 | поле | public | `std::string name{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:28–33`.

```cpp linenums="28"
    struct AIDebugIntegerFactView
    {
        AIWorldIntegerFactId factId{};
        std::int32_t value{};
        std::string name{};
    };
```
