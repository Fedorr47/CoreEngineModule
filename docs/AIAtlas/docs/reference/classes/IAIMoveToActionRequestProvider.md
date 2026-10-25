# IAIMoveToActionRequestProvider

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:20–27` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | метод | public | `virtual ~IAIMoveToActionRequestProvider() = default;` |
| 25 | метод | public | `[[nodiscard]] virtual std::optional<AIMoveToActionRequest> ResolveRequest(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:20–27`.

```cpp linenums="20"
    class IAIMoveToActionRequestProvider
    {
    public:
        virtual ~IAIMoveToActionRequestProvider() = default;

        [[nodiscard]] virtual std::optional<AIMoveToActionRequest> ResolveRequest(
            const AIActionRuntimeContext& context) = 0;
    };
```
