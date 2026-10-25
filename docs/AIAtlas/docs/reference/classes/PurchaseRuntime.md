# PurchaseRuntime

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:212–253` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionRuntime`

> Мгновенная покупка: проверки и событие ResourcePurchased; факты не меняет.

## Архитектурный разбор

**Ссылки.** [теория: 17-resources-multiagent](../../chapters/17-resources-multiagent.md) · [схема d17-purchase-sequence](../diagrams.md#d17-purchase-sequence) · [CP-PURCHASE-RT](../source-references.md#cp-purchase-rt) · [CP-PURCHASE-CAP](../source-references.md#cp-purchase-cap)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 215 | конструктор | public | `PurchaseRuntime(AIAgentWorldState& facts, const GameplayWorld& world,` |
| 217 | метод | public | `: facts_(facts), world_(world), events_(events), transaction_(transaction), radius_(radius) {}` |
| 218 | метод | public | `AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override` |
| 241 | метод | public | `AIActionRuntimeResult Tick(const AIActionRuntimeContext&, float) override` |
| 245 | метод | public | `void Cancel(const AIActionRuntimeContext&) noexcept override {}` |
| 247 | поле | private | `AIAgentWorldState& facts_;` |
| 248 | поле | private | `const GameplayWorld& world_;` |
| 249 | поле | private | `std::vector<GameplayWorldEvent>& events_;` |
| 250 | поле | private | `Transaction transaction_;` |
| 251 | поле | private | `float radius_{};` |
| 252 | поле | private | `bool committed_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:212–253`.

```cpp linenums="212"
    class PurchaseRuntime final : public IAIActionRuntime
    {
    public:
        PurchaseRuntime(AIAgentWorldState& facts, const GameplayWorld& world,
            std::vector<GameplayWorldEvent>& events, Transaction transaction, float radius)
            : facts_(facts), world_(world), events_(events), transaction_(transaction), radius_(radius) {}
        AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            if (committed_ || context.actionId != kAIPurchaseActionId
                || !world_.IsEntityValid(transaction_.target) || facts_.IsFactSet(transaction_.result)
                || facts_.GetIntegerFact(transaction_.resource) < transaction_.price)
            {
                return AIActionRuntimeResult::Failed;
            }
            const auto* transform = world_.TryGetTransform(context.agentEntity);
            if (transform == nullptr)
            {
                return AIActionRuntimeResult::Failed;
            }
            const auto delta = transform->position - transaction_.position;
            if (!(mathUtils::Dot(delta, delta) <= radius_ * radius_))
            {
                return AIActionRuntimeResult::Failed;
            }
            events_.push_back({GameplayWorldEventType::ResourcePurchased, context.agentEntity,
                transaction_.target, transaction_.receiptId});
            committed_ = true;
            return AIActionRuntimeResult::Succeeded;
        }
        AIActionRuntimeResult Tick(const AIActionRuntimeContext&, float) override
        {
            return committed_ ? AIActionRuntimeResult::Succeeded : AIActionRuntimeResult::Failed;
        }
        void Cancel(const AIActionRuntimeContext&) noexcept override {}
    private:
        AIAgentWorldState& facts_;
        const GameplayWorld& world_;
        std::vector<GameplayWorldEvent>& events_;
        Transaction transaction_;
        float radius_{};
        bool committed_{};
    };
```
