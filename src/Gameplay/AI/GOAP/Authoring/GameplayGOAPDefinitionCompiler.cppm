module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

export module core:gameplay_goap_definition_compiler;

export import :ai_action_contracts;
export import :gameplay_goap_definition_contracts;
import :gameplay_goap_definition_asset;

export namespace rendern {
struct GameplayGOAPSemanticAction {
  std::string name{};
  AIActionId actionId{};
};
struct GameplayGOAPActionCostOverride {
  std::string action{};
  std::string context{};
  float cost{};
};

class GameplayGOAPCompiledDefinition {
public:
  GameplayGOAPDecisionDefinition definition{};
  [[nodiscard]] std::optional<AIWorldFactId>
  FindBooleanFact(const std::string_view name) const {
    const auto found = booleanFacts_.find(std::string(name));
    return found == booleanFacts_.end() ? std::nullopt
                                        : std::optional{found->second};
  }
  [[nodiscard]] std::optional<AIWorldIntegerFactId>
  FindIntegerFact(const std::string_view name) const {
    const auto found = integerFacts_.find(std::string(name));
    return found == integerFacts_.end() ? std::nullopt
                                        : std::optional{found->second};
  }
  [[nodiscard]] std::optional<AIActionContextId>
  FindActionContext(const std::string_view name) const {
    const auto found = contexts_.find(std::string(name));
    return found == contexts_.end() ? std::nullopt
                                    : std::optional{found->second};
  }

private:
  friend GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(
      const GameplayGOAPDefinitionAsset &,
      std::span<const GameplayGOAPSemanticAction>,
      std::span<const GameplayGOAPActionCostOverride>);
  std::unordered_map<std::string, AIWorldFactId> booleanFacts_{};
  std::unordered_map<std::string, AIWorldIntegerFactId> integerFacts_{};
  std::unordered_map<std::string, AIActionContextId> contexts_{};
};

[[nodiscard]] GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(
    const GameplayGOAPDefinitionAsset &asset,
    std::span<const GameplayGOAPSemanticAction> semanticActions,
    std::span<const GameplayGOAPActionCostOverride> costOverrides = {});
} // namespace rendern

namespace rendern
{
  namespace {
    [[noreturn]] void Invalid(const std::string_view source,
                              const std::string_view section,
                              const std::string_view symbol,
                              const std::string_view reason) {
      throw std::runtime_error(
          "GOAP definition '" + std::string(source) + "', " + std::string(section) +
          (symbol.empty() ? "" : " '" + std::string(symbol) + "'") + ": " +
          std::string(reason));
    }

    [[nodiscard]] bool IsSymbolValid(const std::string_view symbol) noexcept {
      const auto isLetter = [](const char value) {
        return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
      };
      if (symbol.empty() || !(isLetter(symbol.front()) || symbol.front() == '_')) {
        return false;
      }
      return std::ranges::all_of(symbol.substr(1), [&](const char character) {
        return isLetter(character) || (character >= '0' && character <= '9') ||
               character == '_';
      });
    }

    void ValidateSymbol(const std::string_view source,
                        const std::string_view section,
                        const std::string_view symbol) {
      if (!IsSymbolValid(symbol)) {
        Invalid(source, section, symbol,
                "symbol must begin with a letter or '_' and contain only letters, "
                "digits, or '_'");
      }
    }
    AINumericConditionOperator
    ConditionOperator(const std::string_view op,
                      const GameplayGOAPDefinitionAsset &asset,
                      const std::string_view symbol) {
      if (op == "==") {
        return AINumericConditionOperator::Equal;
      }
      if (op == "!=") {
        return AINumericConditionOperator::NotEqual;
      }
      if (op == "<") {
        return AINumericConditionOperator::Less;
      }
      if (op == "<=") {
        return AINumericConditionOperator::LessOrEqual;
      }
      if (op == ">") {
        return AINumericConditionOperator::Greater;
      }
      if (op == ">=") {
        return AINumericConditionOperator::GreaterOrEqual;
      }
      Invalid(asset.source, "actions", symbol,
              "unknown numeric condition operator '" + std::string(op) + "'");
    }
    AINumericEffectOperation
    EffectOperation(const std::string_view op,
                    const GameplayGOAPDefinitionAsset &asset,
                    const std::string_view symbol) {
      if (op == "set") {
        return AINumericEffectOperation::Set;
      }
      if (op == "add") {
        return AINumericEffectOperation::Add;
      }
      Invalid(asset.source, "actions", symbol,
              "unknown numeric effect operation '" + std::string(op) + "'");
    }
  } // namespace

  GameplayGOAPCompiledDefinition CompileGameplayGOAPDefinition(
      const GameplayGOAPDefinitionAsset &asset,
      const std::span<const GameplayGOAPSemanticAction> semanticActions,
      const std::span<const GameplayGOAPActionCostOverride> costOverrides) {
    if (asset.id.empty()) {
      Invalid(asset.source, "root", {}, "missing id");
    }
    ValidateSymbol(asset.source, "root", asset.id);
    if (asset.goals.empty()) {
      Invalid(asset.source, "goals", {}, "at least one goal is required");
    }
    GameplayGOAPCompiledDefinition compiled{};
    std::unordered_set<std::string> symbols;
    std::uint16_t booleanIndex = 0, integerIndex = 0;
    for (const GameplayGOAPAuthoredFact &fact : asset.facts) {
      ValidateSymbol(asset.source, "facts", fact.name);
      if (fact.name.empty() || !symbols.insert(fact.name).second) {
        Invalid(asset.source, "facts", fact.name, "empty or duplicate fact name");
      }
      if (fact.type == GameplayGOAPFactType::Boolean) {
        if (booleanIndex >= AIAgentWorldState::FactCapacity) {
          Invalid(asset.source, "facts", fact.name,
                  "boolean fact capacity exceeded");
        }
        compiled.booleanFacts_.emplace(fact.name, AIWorldFactId{booleanIndex++});
        compiled.definition.metadata.booleanFacts.push_back(
            {AIWorldFactId{static_cast<AIWorldFactId::ValueType>(booleanIndex - 1u)},
                fact.name});
      } else {
        if (integerIndex >= AIAgentWorldState::IntegerFactCapacity) {
          Invalid(asset.source, "facts", fact.name,
                  "integer fact capacity exceeded");
        }
        compiled.integerFacts_.emplace(fact.name,
                                       AIWorldIntegerFactId{integerIndex++});
        compiled.definition.metadata.integerFacts.push_back(
            {AIWorldIntegerFactId{
                static_cast<AIWorldIntegerFactId::ValueType>(integerIndex - 1u)},
                fact.name});
      }
    }
    const auto booleanId = [&](const std::string &name,
                               const std::string_view section,
                               const std::string_view symbol) {
      const auto found = compiled.booleanFacts_.find(name);
      if (found != compiled.booleanFacts_.end()) {
        return found->second;
      }
      if (compiled.integerFacts_.contains(name)) {
        Invalid(asset.source, section, symbol,
                "fact '" + name + "' has wrong type; expected bool");
      }
      Invalid(asset.source, section, symbol, "unknown fact '" + name + "'");
    };
    const auto integerId = [&](const std::string &name,
                               const std::string_view symbol) {
      const auto found = compiled.integerFacts_.find(name);
      if (found != compiled.integerFacts_.end()) {
        return found->second;
      }
      if (compiled.booleanFacts_.contains(name)) {
        Invalid(asset.source, "actions", symbol,
                "fact '" + name + "' has wrong type; expected int");
      }
      Invalid(asset.source, "actions", symbol, "unknown fact '" + name + "'");
    };
    std::unordered_set<std::string> goalNames;
    for (std::size_t index = 0; index < asset.goals.size(); ++index) {
      const auto &authored = asset.goals[index];
      ValidateSymbol(asset.source, "goals", authored.name);
      if (authored.name.empty() || !goalNames.insert(authored.name).second) {
        Invalid(asset.source, "goals", authored.name,
                "empty or duplicate goal name");
      }
      if (index >= AIGoalId::InvalidValue) {
        Invalid(asset.source, "goals", authored.name,
                "goal id capacity exceeded");
      }
      if (!std::isfinite(authored.score)) {
        Invalid(asset.source, "goals", authored.name,
                "score must be finite and representable as float");
      }
      AIGoalSelectionCandidate goal{
        .goal =
            AIGoalDefinition{AIGoalId{static_cast<AIGoalId::ValueType>(index)},
                             {}},
        .baseScore = authored.score};
      for (const auto &condition : authored.facts) {
        goal.goal.desiredFacts.push_back(
            {booleanId(condition.fact, "goals", authored.name), condition.value});
      }
      compiled.definition.goals.push_back(std::move(goal));
      compiled.definition.metadata.goals.push_back(
          {AIGoalId{static_cast<AIGoalId::ValueType>(index)}, authored.name});
    }
    std::unordered_map<std::string, AIActionId> actions;
    std::unordered_set<AIActionId::ValueType> actionIds;
    for (const auto &semantic : semanticActions)
    {
      ValidateSymbol(asset.source, "semanticActions", semantic.name);
      if (!semantic.actionId.IsValid()) {
        Invalid(asset.source, "semanticActions", semantic.name,
        "semantic action id must be valid");
      }
      if (!actions.emplace(semantic.name, semantic.actionId).second) {
        Invalid(asset.source, "semanticActions", semantic.name,
                 "duplicate semantic action name");
      }
      if (!actionIds.insert(semantic.actionId.value).second) {
        Invalid(asset.source, "semanticActions", semantic.name,
                "duplicate semantic action id");
      }
    }
    std::vector<GameplayGOAPActionCostOverride> overrides;
    for (const auto &overrideValue : costOverrides) {
      ValidateSymbol(asset.source, "costOverrides", overrideValue.action);
      ValidateSymbol(asset.source, "costOverrides", overrideValue.context);
      if (!std::isfinite(overrideValue.cost) || overrideValue.cost < 0.0f) {
        Invalid(asset.source, "costOverrides", overrideValue.context,
                "cost must be finite and non-negative");
      }
      if (std::ranges::any_of(overrides, [&](const auto &candidate) {
            return candidate.action == overrideValue.action &&
                   candidate.context == overrideValue.context;
          })) {
        Invalid(asset.source, "costOverrides", overrideValue.context,
                "duplicate override key");
          }
      overrides.push_back(overrideValue);
    }
    std::unordered_set<std::uint32_t> actionKeys;
    std::unordered_set<std::string> contextNames;
    std::uint16_t nextContext = 0;
    for (const auto &authored : asset.actions) {
      ValidateSymbol(asset.source, "actions", authored.action);
      ValidateSymbol(asset.source, "actions", authored.context);
      if (!std::isfinite(authored.cost) || authored.cost < 0.0f) {
        Invalid(asset.source, "actions", authored.context,
                "cost must be finite and non-negative");
      }
      const auto semantic = actions.find(authored.action);
      if (semantic == actions.end()) {
        Invalid(asset.source, "actions", authored.context,
                "unknown semantic action '" + authored.action + "'");
      }
      if (!contextNames.insert(authored.context).second) {
        Invalid(asset.source, "actions", authored.context,
                "duplicate action context");
      }
      auto [context, inserted] = compiled.contexts_.emplace(
          authored.context, AIActionContextId{nextContext});
      if (inserted) {
        if (nextContext == AIActionContextId::InvalidValue) {
          Invalid(asset.source, "actions", authored.context,
                  "context id capacity exceeded");
        }
        ++nextContext;
      }
      const std::uint32_t key =
          (static_cast<std::uint32_t>(semantic->second.value) << 16u) |
          context->second.value;
      if (!actionKeys.insert(key).second) {
        Invalid(asset.source, "actions", authored.context,
                "duplicate effective (actionId, contextId) key");
      }
      const auto overrideValue =
          std::ranges::find_if(overrides, [&](const auto &candidate) {
            return candidate.action == authored.action &&
                   candidate.context == authored.context;
          });
      AIActionDefinition action{.actionId = semantic->second,
                                .contextId = context->second,
                                .baseCost = overrideValue == overrides.end()
                                                ? authored.cost
                                                : overrideValue->cost};
      for (const auto &value : authored.preconditions) {
        action.preconditions.push_back(
            {booleanId(value.fact, "actions", authored.context), value.value});
      }
      for (const auto &value : authored.effects) {
        action.effects.push_back(
            {booleanId(value.fact, "actions", authored.context), value.value});
      }
      for (const auto &value : authored.numericPreconditions) {
        action.numericPreconditions.push_back(
            {integerId(value.fact, authored.context),
             ConditionOperator(value.operation, asset, authored.context),
             value.value});
      }
      for (const auto &value : authored.numericEffects) {
        action.numericEffects.push_back(
            {integerId(value.fact, authored.context),
             EffectOperation(value.operation, asset, authored.context),
             value.value});
      }
      for (const auto &value : authored.continuationConditions) {
        action.continuationConditions.push_back(
            {booleanId(value.fact, "continuationConditions", authored.context), value.value});
      }
      for (const auto &value : authored.numericContinuationConditions) {
        action.numericContinuationConditions.push_back(
            {integerId(value.fact, authored.context),
             ConditionOperator(value.operation, asset, authored.context), value.value});
      }
      compiled.definition.actions.push_back(std::move(action));
      compiled.definition.metadata.actions.push_back(
          {semantic->second, context->second, authored.action, authored.context});
    }
    for (const auto &overrideValue : overrides) {
      const bool matched =
          std::ranges::any_of(asset.actions, [&](const auto &candidate) {
            return candidate.action == overrideValue.action &&
                   candidate.context == overrideValue.context;
          });
      if (!matched) {
        Invalid(asset.source, "costOverrides", overrideValue.context,
                "override does not match an authored action");
      }
    }
    return compiled;
  }
} // namespace rendern