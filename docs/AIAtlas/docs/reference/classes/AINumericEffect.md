# AINumericEffect

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:63–68` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 65 | поле | public | `AIWorldIntegerFactId factId{};` |
| 66 | поле | public | `AINumericEffectOperation operation{ AINumericEffectOperation::Set };` |
| 67 | поле | public | `std::int32_t value{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:63–68`.

```cpp linenums="63"
    struct AINumericEffect
    {
        AIWorldIntegerFactId factId{};
        AINumericEffectOperation operation{ AINumericEffectOperation::Set };
        std::int32_t value{};
    };
```
