# AIDebugBooleanFactView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:21–26` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | поле | public | `AIWorldFactId factId{};` |
| 24 | поле | public | `bool value{};` |
| 25 | поле | public | `std::string name{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:21–26`.

```cpp linenums="21"
    struct AIDebugBooleanFactView
    {
        AIWorldFactId factId{};
        bool value{};
        std::string name{};
    };
```
