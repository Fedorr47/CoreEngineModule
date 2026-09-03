module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

export module core:gameplay_goap_resource_components;

import :gameplay_goap_composition_registry;
import :gameplay;
import :ai_action_binding;

export namespace rendern
{
    inline constexpr AIActionId kAIPurchaseActionId{3u};
}

namespace rendern::goap_resource_detail
{
    struct Transaction
    {
        EntityHandle target{kNullEntity};
        mathUtils::Vec3 position{};
        AIWorldFactId result;
        AIWorldIntegerFactId resource;
        std::int32_t price{};
        std::string receiptId;
    };

    Transaction ResolveReceipt(std::string_view name, const GameplayGOAPCompositionContext& context)
    {
        const GameplayAIResourceReceiptAsset* selected = nullptr;
        const GameplayAIResourceLedgerAsset* selectedLedger = nullptr;
        std::set<std::string> names;
        // Receipt IDs are unique across this behavior; subjects may be shared.
        for (const auto& observer : context.observations)
        {
            const auto* ledger = std::get_if<GameplayAIResourceLedgerAsset>(&observer.parameters);
            if (ledger == nullptr)
            {
                continue;
            }
            for (const auto& receipt : ledger->receipts)
            {
                const auto target = context.Role(receipt.target).entity;
                if (receipt.id.empty() || !names.insert(receipt.id).second)
                {
                    context.Fail(receipt.id, "duplicate receipt id");
                }
                if (!context.services.world.IsEntityValid(target) || receipt.price <= 0)
                {
                    context.Fail(receipt.id, "receipt requires a live subject and positive price");
                }
                if (receipt.id == name)
                {
                    selected = &receipt;
                    selectedLedger = ledger;
                }
            }
        }
        if (selected == nullptr)
        {
            context.Fail(name, "unknown resource receipt");
        }
        const auto& target = context.Role(selected->target);
        return {target.entity, target.position, context.BooleanFact(selected->fact),
            context.IntegerFact(selectedLedger->fact), selected->price, selected->id};
    }

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
                    }
                }
                else if (event.type == GameplayWorldEventType::ResourcePurchased)
                {
                    for (const auto& receipt : receipts_)
                    {
                        if (event.subject == receipt.target && event.receiptId == receipt.receiptId
                            && !facts.IsFactSet(receipt.result)
                            && facts.GetIntegerFact(resource_) >= receipt.price)
                        {
                            facts.SetIntegerFact(resource_, facts.GetIntegerFact(resource_) - receipt.price);
                            facts.SetFact(receipt.result, true);
                        }
                    }
                }
            }
        }
        void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            Observe(world, agent, events, facts);
        }
    private:
        struct Pickup { EntityHandle target; AIWorldFactId fact; std::int32_t amount; };
        AIWorldIntegerFactId resource_;
        std::vector<Pickup> pickups_;
        std::vector<Transaction> receipts_;
    };

    // Visibility is an authored reaction, independent of purchase execution.
    class HideOnPurchaseReaction final : public IGameplayGOAPEventReaction
    {
    public:
        HideOnPurchaseReaction(const GameplayAIReactionAsset& asset, const GameplayGOAPCompositionContext& context)
            : agent_(context.services.agent)
        {
            const auto& parameters = context.Parameters<GameplayAIHideOnPurchaseAsset>(asset);
            transaction_ = ResolveReceipt(parameters.receipt, context);
            target_ = context.Role(parameters.target).entity;
            const auto* node = context.services.world.TryGetNodeLink(target_);
            if (!context.services.world.IsEntityValid(target_) || node == nullptr || node->nodeIndex < 0)
            {
                context.Fail(asset.type, "visibility target requires a live node-bound entity");
            }
        }
        void React(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
            const AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& output) override
        {
            if (emitted_ || !world.IsEntityValid(target_) || !facts.IsFactSet(transaction_.result))
            {
                return;
            }
            for (const auto& event : events)
            {
                if (event.type == GameplayWorldEventType::ResourcePurchased && event.instigator == agent_
                    && event.subject == transaction_.target && event.receiptId == transaction_.receiptId)
                {
                    output.push_back({GameplayWorldEventType::HideEntityRequested, agent_, target_});
                    emitted_ = true;
                    return;
                }
            }
        }
    private:
        EntityHandle agent_{kNullEntity};
        EntityHandle target_{kNullEntity};
        Transaction transaction_{};
        bool emitted_{};
    };

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

    struct PurchaseContext { AIActionContextId id; Transaction transaction; float radius; };
    class PurchaseBinding final : public IAIActionBinding
    {
    public:
        PurchaseBinding(AIAgentWorldState& facts, const GameplayWorld& world,
            std::vector<GameplayWorldEvent>& events, std::vector<PurchaseContext> contexts)
            : facts_(facts), world_(world), events_(events), contexts_(std::move(contexts)) {}
        std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext& context) override
        {
            if (context.actionId == kAIPurchaseActionId)
            {
                for (const auto& entry : contexts_)
                {
                    if (entry.id == context.contextId)
                    {
                        return std::make_unique<PurchaseRuntime>(facts_, world_, events_, entry.transaction, entry.radius);
                    }
                }
            }
            return nullptr;
        }
    private:
        AIAgentWorldState& facts_;
        const GameplayWorld& world_;
        std::vector<GameplayWorldEvent>& events_;
        std::vector<PurchaseContext> contexts_;
    };

    class PurchaseCapability final : public IGameplayGOAPCapability
    {
    public:
        PurchaseCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
            : world_(context.services.world)
        {
            for (const auto& asset : assets)
            {
                const auto& parameters = context.Parameters<GameplayAIPurchaseAsset>(asset);
                if (!(parameters.radius > 0) || !std::isfinite(parameters.radius * parameters.radius))
                {
                    context.Fail(asset.context, "purchase radius must be positive and finite");
                }
                const auto transaction = ResolveReceipt(parameters.receipt, context);
                const auto id = context.definition.FindActionContext(asset.context);
                if (!id)
                {
                    context.Fail(asset.context, "unknown purchase context");
                }
                const auto action = std::ranges::find(context.definition.definition.actions, *id, &AIActionDefinition::contextId);
                if (action == context.definition.definition.actions.end()
                    || action->effects.size() != 1 || action->effects.front().factId != transaction.result
                    || !action->effects.front().bValue || action->numericEffects.size() != 1
                    || action->numericEffects.front().factId != transaction.resource
                    || action->numericEffects.front().operation != AINumericEffectOperation::Add
                    || action->numericEffects.front().value != -transaction.price)
                {
                    context.Fail(asset.context, "purchase definition effects do not match the receipt/price");
                }
                const bool fundsGuard = std::ranges::any_of(action->numericPreconditions, [&](const auto& condition)
                {
                    return condition.factId == transaction.resource
                        && condition.comparison == AINumericConditionOperator::GreaterOrEqual
                        && condition.value == transaction.price;
                });
                if (!fundsGuard)
                {
                    context.Fail(asset.context, "purchase requires a matching resource >= price precondition");
                }
                contexts_.push_back({*id, transaction, parameters.radius});
            }
        }
        std::unique_ptr<IAIActionBinding> CreateBinding(AIAgentWorldState& facts,
            std::vector<GameplayWorldEvent>& events) override
        {
            return std::make_unique<PurchaseBinding>(facts, world_, events, contexts_);
        }
    private:
        const GameplayWorld& world_;
        std::vector<PurchaseContext> contexts_;
    };
}

export namespace rendern
{
    void RegisterGameplayGOAPResourceComponents(GameplayGOAPCompositionRegistry& registry)
    {
        const bool ledger = registry.RegisterObservation("resource_ledger", [](const auto& asset, const auto& context)
        {
            return std::make_unique<goap_resource_detail::ResourceLedgerObservation>(asset, context);
        });
        const bool purchase = registry.RegisterCapability("purchase", kAIPurchaseActionId, [](auto assets, const auto& context)
        {
            return std::make_unique<goap_resource_detail::PurchaseCapability>(assets, context);
        });
        const bool hide = registry.RegisterReaction("hide_on_purchase", [](const auto& asset, const auto& context)
        {
            return std::make_unique<goap_resource_detail::HideOnPurchaseReaction>(asset, context);
        });
        if (!ledger || !purchase || !hide)
        {
            throw std::logic_error("Duplicate resource component registration");
        }
    }
}
