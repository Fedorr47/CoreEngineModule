# IAIActionBinding

**class** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionBinding.cppm:14–21` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Фабрика runtime для шага плана по контексту (позднее связывание семантики и исполнителя).

## Архитектурный разбор

**Владелец и время жизни.** Реализации принадлежат GameplayGOAPDecision (ownedBindings_); реестр хранит сырые указатели по actionId.

**Кто вызывает и в какой фазе.** AIPlanExecutionBridge::StartReadyPlanStep.

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [AB-IFACE](../source-references.md#ab-iface) · [AB-REGISTRY](../source-references.md#ab-registry) · тест `TestAIActionBinding.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 17 | метод | public | `virtual ~IAIActionBinding() = default;` |
| 19 | метод | public | `[[nodiscard]] virtual std::unique_ptr<IAIActionRuntime> CreateRuntime(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionBinding.cppm:14–21`.

```cpp linenums="14"
    class IAIActionBinding
    {
    public:
        virtual ~IAIActionBinding() = default;

        [[nodiscard]] virtual std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context) = 0;
    };
```
