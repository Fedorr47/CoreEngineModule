# IAIActionReservationTargetProvider

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:29–35` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 32 | метод | public | `virtual ~IAIActionReservationTargetProvider() = default;` |
| 33 | метод | public | `[[nodiscard]] virtual EntityHandle ResolveReservationTarget(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:29–35`.

```cpp linenums="29"
     class IAIActionReservationTargetProvider
    {
    public:
        virtual ~IAIActionReservationTargetProvider() = default;
        [[nodiscard]] virtual EntityHandle ResolveReservationTarget(
            const AIActionRuntimeContext& context) = 0;
    };
```
