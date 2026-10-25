# Transaction

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:31–39` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 33 | поле | public | `EntityHandle target{kNullEntity};` |
| 34 | поле | public | `mathUtils::Vec3 position{};` |
| 35 | поле | public | `AIWorldFactId result;` |
| 36 | поле | public | `AIWorldIntegerFactId resource;` |
| 37 | поле | public | `std::int32_t price{};` |
| 38 | поле | public | `std::string receiptId;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:31–39`.

```cpp linenums="31"
    struct Transaction
    {
        EntityHandle target{kNullEntity};
        mathUtils::Vec3 position{};
        AIWorldFactId result;
        AIWorldIntegerFactId resource;
        std::int32_t price{};
        std::string receiptId;
    };
```
