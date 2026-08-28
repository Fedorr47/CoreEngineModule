module;

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_goap_definition_asset;

import :file_system;
import :json_utils;

export namespace rendern
{
  enum class GameplayGOAPFactType : std::uint8_t { Boolean, Integer };
  struct GameplayGOAPAuthoredFact {
    std::string name{};
    GameplayGOAPFactType type{};
  };
  struct GameplayGOAPAuthoredBoolean {
    std::string fact{};
    bool value{};
  };
  struct GameplayGOAPAuthoredNumeric {
    std::string fact{};
    std::string operation{};
    std::int32_t value{};
  };
  struct GameplayGOAPAuthoredGoal {
    std::string name{};
    float score{};
    std::vector<GameplayGOAPAuthoredBoolean> facts{};
  };
  struct GameplayGOAPAuthoredAction {
    std::string action{};
    std::string context{};
    float cost{};
    std::vector<GameplayGOAPAuthoredBoolean> preconditions{};
    std::vector<GameplayGOAPAuthoredBoolean> effects{};
    std::vector<GameplayGOAPAuthoredNumeric> numericPreconditions{};
    std::vector<GameplayGOAPAuthoredNumeric> numericEffects{};
  };
  struct GameplayGOAPDefinitionAsset {
    std::string id{};
    std::string source{};
    std::vector<GameplayGOAPAuthoredFact> facts{};
    std::vector<GameplayGOAPAuthoredGoal> goals{};
    std::vector<GameplayGOAPAuthoredAction> actions{};
  };

  [[nodiscard]] GameplayGOAPDefinitionAsset
  ParseGameplayGOAPDefinitionAsset(std::string_view json,
                                   std::string_view source);
  [[nodiscard]] GameplayGOAPDefinitionAsset
  LoadGameplayGOAPDefinitionAsset(std::string_view path);
} // namespace rendern

namespace rendern
{
  namespace {
    using jsonUtils::JsonArray;
    using jsonUtils::JsonObject;
    using jsonUtils::JsonValue;
    [[noreturn]] void Invalid(const std::string_view source,
                              const std::string_view section,
                              const std::string_view symbol,
                              const std::string_view reason) {
      throw std::runtime_error(
          "GOAP definition '" + std::string(source) + "', " + std::string(section) +
          (symbol.empty() ? "" : " '" + std::string(symbol) + "'") + ": " +
          std::string(reason));
    }
    
    const JsonValue &Required(const JsonObject &object, const char *key,
                              const std::string_view source,
                              const std::string_view section,
                              const std::string_view symbol) {
      const auto found = object.find(key);
      if (found == object.end()) {
        Invalid(source, section, symbol,
                "missing field '" + std::string(key) + "'");
      }
      return found->second;
    }
    std::string String(const JsonObject &object, const char *key,
                       const std::string_view source,
                       const std::string_view section,
                       const std::string_view symbol) {
      const JsonValue &value = Required(object, key, source, section, symbol);
      if (!value.IsString() || value.AsString().empty()) {
        Invalid(source, section, symbol,
                "field '" + std::string(key) + "' must be a non-empty string");
      }
      return value.AsString();
    }
    const JsonArray &Array(const JsonObject &object, const char *key,
                           const std::string_view source,
                           const std::string_view section,
                           const std::string_view symbol,
                           const bool optional = false) {
      const auto found = object.find(key);
      static const JsonArray empty{};
      if (found == object.end() && optional) {
        return empty;
      }
      if (found == object.end() || !found->second.IsArray()) {
        Invalid(source, section, symbol,
                "field '" + std::string(key) + "' must be an array");
      }
      return found->second.AsArray();
    }
    std::vector<GameplayGOAPAuthoredBoolean>
    Booleans(const JsonObject &object, const char *key,
             const std::string_view source, const std::string_view section,
             const std::string_view symbol) {
      std::vector<GameplayGOAPAuthoredBoolean> result;
      for (const JsonValue &value :
           Array(object, key, source, section, symbol, true)) {
        if (!value.IsObject()) {
          Invalid(source, section, symbol,
                  "entries in '" + std::string(key) + "' must be objects");
        }
        const JsonObject &entry = value.AsObject();
        const JsonValue &expected =
            Required(entry, "value", source, section, symbol);
        if (!expected.IsBool()) {
          Invalid(source, section, symbol, "boolean condition value must be bool");
        }
        result.push_back(
            {String(entry, "fact", source, section, symbol), expected.AsBool()});
           }
      return result;
    }
    std::vector<GameplayGOAPAuthoredNumeric>
    Numerics(const JsonObject &object, const char *key,
             const std::string_view source, const std::string_view section,
             const std::string_view symbol) {
      std::vector<GameplayGOAPAuthoredNumeric> result;
      for (const JsonValue &value :
           Array(object, key, source, section, symbol, true)) {
        if (!value.IsObject()) {
          Invalid(source, section, symbol,
                  "entries in '" + std::string(key) + "' must be objects");
        }
        const JsonObject &entry = value.AsObject();
        const JsonValue &number = Required(entry, "value", source, section, symbol);
        if (!number.IsNumber() || !std::isfinite(number.AsNumber()) ||
            std::floor(number.AsNumber()) != number.AsNumber() ||
            number.AsNumber() < std::numeric_limits<std::int32_t>::min() ||
            number.AsNumber() > std::numeric_limits<std::int32_t>::max()) {
          Invalid(source, section, symbol,
                  "numeric value must be a finite 32-bit integer");
            }
        result.push_back({String(entry, "fact", source, section, symbol),
                          String(entry, "op", source, section, symbol),
                          static_cast<std::int32_t>(number.AsNumber())});
           }
      return result;
    }
  } // namespace

  GameplayGOAPDefinitionAsset
  ParseGameplayGOAPDefinitionAsset(const std::string_view json,
                                   const std::string_view source) {
    const JsonValue rootValue = jsonUtils::JsonParser(json).Parse();
    if (!rootValue.IsObject()) {
      Invalid(source, "root", {}, "root must be an object");
    }
    const JsonObject &root = rootValue.AsObject();
    GameplayGOAPDefinitionAsset asset{};
    asset.source = source;
    asset.id = String(root, "id", source, "root", {});
    for (const JsonValue &value : Array(root, "facts", source, "facts", {})) {
      if (!value.IsObject()) {
        Invalid(source, "facts", {}, "fact must be an object");
      }
      const JsonObject &object = value.AsObject();
      const std::string name = String(object, "name", source, "facts", {});
      const std::string type = String(object, "type", source, "facts", name);
      if (type != "bool" && type != "int") {
        Invalid(source, "facts", name, "type must be 'bool' or 'int'");
      }
      asset.facts.push_back({name, type == "bool"
                                       ? GameplayGOAPFactType::Boolean
                                       : GameplayGOAPFactType::Integer});
    }
    for (const JsonValue &value : Array(root, "goals", source, "goals", {})) {
      if (!value.IsObject()) {
        Invalid(source, "goals", {}, "goal must be an object");
      }
      const JsonObject &object = value.AsObject();
      GameplayGOAPAuthoredGoal goal{};
      goal.name = String(object, "name", source, "goals", {});
      const JsonValue &score =
          Required(object, "score", source, "goals", goal.name);
      if (!score.IsNumber() || !std::isfinite(score.AsNumber()) ||
          std::abs(score.AsNumber()) > std::numeric_limits<float>::max()) {
        Invalid(source, "goals", goal.name,
                "score must be finite and representable as float");
          }
      goal.score = static_cast<float>(score.AsNumber());
      goal.facts = Booleans(object, "facts", source, "goals", goal.name);
      asset.goals.push_back(std::move(goal));
    }
    for (const JsonValue &value : Array(root, "actions", source, "actions", {})) {
      if (!value.IsObject()) {
        Invalid(source, "actions", {}, "action must be an object");
      }
      const JsonObject &object = value.AsObject();
      GameplayGOAPAuthoredAction action{};
      action.action = String(object, "action", source, "actions", {});
      action.context =
          String(object, "context", source, "actions", action.action);
      const JsonValue &cost =
          Required(object, "cost", source, "actions", action.context);
      if (!cost.IsNumber() || !std::isfinite(cost.AsNumber()) ||
          cost.AsNumber() < 0.0 ||
          cost.AsNumber() > std::numeric_limits<float>::max()) {
        Invalid(source, "actions", action.context,
                "cost must be finite and non-negative");
          }
      action.cost = static_cast<float>(cost.AsNumber());
      action.preconditions =
          Booleans(object, "preconditions", source, "actions", action.context);
      action.effects =
          Booleans(object, "effects", source, "actions", action.context);
      action.numericPreconditions = Numerics(object, "numericPreconditions",
                                             source, "actions", action.context);
      action.numericEffects =
          Numerics(object, "numericEffects", source, "actions", action.context);
      asset.actions.push_back(std::move(action));
    }
    return asset;
  }
  GameplayGOAPDefinitionAsset
  LoadGameplayGOAPDefinitionAsset(const std::string_view path) {
    return ParseGameplayGOAPDefinitionAsset(
        FILE_UTILS::ReadAllText(
            corefs::ResolveAsset(std::filesystem::path(path))),
        path);
  }
} // namespace rendern