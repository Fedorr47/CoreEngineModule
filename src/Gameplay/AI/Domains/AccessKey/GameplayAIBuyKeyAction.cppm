module;

#include <memory>
#include <vector>

export module core:gameplay_ai_buy_key_action;

import :ai_action_binding;
import :gameplay;
import :gameplay_world_event;
import :gameplay_ai_access_key_contracts;
import :ai_agent_world_state;

export namespace rendern::ai_access_key_detail
{
        class BuyKeyActionRuntime final : public IAIActionRuntime
        {
        public:
            BuyKeyActionRuntime(AIAgentWorldState& facts, const GameplayWorld& world,
                std::vector<GameplayWorldEvent>& events, const EntityHandle keyEntity,
                const mathUtils::Vec3 keyPosition, const AIWorldFactId hasAccessKeyFact,
                const AIWorldIntegerFactId coinsFact) noexcept
                : facts_(facts), world_(world), events_(events), keyEntity_(keyEntity),
                    keyPosition_(keyPosition), hasAccessKeyFact_(hasAccessKeyFact),
                    coinsFact_(coinsFact)
            {
            }

            [[nodiscard]] AIActionRuntimeResult Start(
                const AIActionRuntimeContext& context) override
            {
                if (context.actionId != kAIBuyKeyActionId || context.agentEntity == kNullEntity ||
                    keyEntity_ == kNullEntity || facts_.IsFactSet(hasAccessKeyFact_) ||
                    facts_.GetIntegerFact(coinsFact_) < kAccessKeyPrice)
                {
                    return AIActionRuntimeResult::Failed;
                }
                const GameplayTransformComponent* transform =
                    world_.TryGetTransform(context.agentEntity);
                if (transform == nullptr)
                {
                    return AIActionRuntimeResult::Failed;
                }
                const mathUtils::Vec3 delta = transform->position - keyPosition_;
                if (mathUtils::Dot(delta, delta) > 0.36f)
                {
                    return AIActionRuntimeResult::Failed;
                }
                committed_ = true;
                events_.push_back({GameplayWorldEventType::AccessKeyPurchased,
                    context.agentEntity, keyEntity_});
                return AIActionRuntimeResult::Succeeded;
            }

            [[nodiscard]] AIActionRuntimeResult Tick(
                const AIActionRuntimeContext&, float) override
            {
                return committed_ ? AIActionRuntimeResult::Succeeded
                                  : AIActionRuntimeResult::Failed;
            }

            void Cancel(const AIActionRuntimeContext&) noexcept override
            {
                // Start commits atomically; cancellation cannot partially purchase the key.
            }

        private:
            AIAgentWorldState& facts_;
            const GameplayWorld& world_;
            std::vector<GameplayWorldEvent>& events_;
            EntityHandle keyEntity_{kNullEntity};
            mathUtils::Vec3 keyPosition_{};
            AIWorldFactId hasAccessKeyFact_{};
            AIWorldIntegerFactId coinsFact_{};
            bool committed_{};
        };

        class BuyKeyActionBinding final : public IAIActionBinding
        {
        public:
            BuyKeyActionBinding(
                AIAgentWorldState& facts,
                const GameplayWorld& world,
                std::vector<GameplayWorldEvent>& events,
                const EntityHandle keyEntity,
                const mathUtils::Vec3 keyPosition,
                const AIWorldFactId hasAccessKeyFact,
                const AIWorldIntegerFactId coinsFact) noexcept
                : facts_(facts),
                  world_(world),
                  events_(events),
                  keyEntity_(keyEntity),
                  keyPosition_(keyPosition),
                  hasAccessKeyFact_(hasAccessKeyFact),
                  coinsFact_(coinsFact)
            {
            }

            [[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(
                const AIActionRuntimeContext& context) override
            {
                if (context.actionId != kAIBuyKeyActionId)
                {
                    return nullptr;
                }
                return std::make_unique<BuyKeyActionRuntime>(
                    facts_, world_, events_, keyEntity_, keyPosition_,
                    hasAccessKeyFact_, coinsFact_);
            }

        private:
            AIAgentWorldState& facts_;
            const GameplayWorld& world_;
            std::vector<GameplayWorldEvent>& events_;
            EntityHandle keyEntity_{kNullEntity};
            mathUtils::Vec3 keyPosition_{};
            AIWorldFactId hasAccessKeyFact_{};
            AIWorldIntegerFactId coinsFact_{};
        };

}
