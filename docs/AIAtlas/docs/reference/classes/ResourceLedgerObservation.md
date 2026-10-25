# ResourceLedgerObservation

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:81–169` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPObservation`

> Бухгалтерия ресурса по подтверждённым событиям сбора и покупки своего агента.

## Архитектурный разбор

**Ссылки.** [теория: 17-resources-multiagent](../../chapters/17-resources-multiagent.md) · [теория: 07-observations-events](../../chapters/07-observations-events.md) · [схема d17-purchase-sequence](../diagrams.md#d17-purchase-sequence) · [CP-LEDGER](../source-references.md#cp-ledger) · [CP-LEDGER-OBSERVE](../source-references.md#cp-ledger-observe) · тест `TestGameplayGOAPResourceComponents.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 84 | конструктор | public | `ResourceLedgerObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)` |
| 103 | метод | public | `std::vector<AIWorldFactId> BooleanOutputs() const override` |
| 116 | метод | public | `std::vector<AIWorldIntegerFactId> IntegerOutputs() const override { return {resource_}; }` |
| 117 | метод | public | `void Observe(const GameplayWorld&, EntityHandle agent,` |
| 159 | метод | public | `void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,` |
| 166 | поле | private | `AIWorldIntegerFactId resource_;` |
| 167 | поле | private | `std::vector<Pickup> pickups_;` |
| 168 | поле | private | `std::vector<Transaction> receipts_;` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponents.cppm:81–141`.

```cpp linenums="81"
    class ResourceLedgerObservation final : public IGameplayGOAPObservation
    {
    public:
        ResourceLedgerObservation(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
        {
            const auto& ledger = context.Parameters<GameplayAIResourceLedgerAsset>(asset);
            resource_ = context.IntegerFact(ledger.fact);
            std::set<EntityHandle> targets;
            for (const auto& authored : ledger.pickups)
            {
                const auto target = context.Role(authored.target).entity;
                if (!context.services.world.HasPickup(target) || authored.amount <= 0 || !targets.insert(target).second)
                {
                    context.Fail(asset.type, "pickup credit requires a unique pickup subject and positive amount");
                }
                pickups_.push_back({target, context.BooleanFact(authored.fact), authored.amount});
            }
            for (const auto& receipt : ledger.receipts)
            {
                receipts_.push_back(ResolveReceipt(receipt.id, context));
            }
        }
        std::vector<AIWorldFactId> BooleanOutputs() const override
        {
            std::vector<AIWorldFactId> result;
            for (const auto& pickup : pickups_)
            {
                result.push_back(pickup.fact);
            }
            for (const auto& receipt : receipts_)
            {
                result.push_back(receipt.result);
            }
            return result;
        }
        std::vector<AIWorldIntegerFactId> IntegerOutputs() const override { return {resource_}; }
        void Observe(const GameplayWorld&, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            for (const auto& event : events)
            {
                if (event.instigator != agent)
                {
                    continue;
                }
                if (event.type == GameplayWorldEventType::PickupCollected)
                {
                    for (const auto& pickup : pickups_)
                    {
                        if (event.subject != pickup.target || facts.IsFactSet(pickup.fact))
                        {
                            continue;
                        }
                        const auto next = static_cast<std::int64_t>(facts.GetIntegerFact(resource_)) + pickup.amount;
                        // Reject an unrepresentable credit without partially acknowledging it.
                        if (next < 0 || next > std::numeric_limits<std::int32_t>::max())
                        {
                            continue;
                        }
                        facts.SetIntegerFact(resource_, static_cast<std::int32_t>(next));
                        facts.SetFact(pickup.fact, true);
```
