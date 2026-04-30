module;

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

export module core:gameplay_graph;

export namespace rendern
{
    enum class GameplayGraphValueType : std::uint8_t
    {
        Bool = 0,
        Int = 1,
        Float = 2,
        Trigger = 3,
        String = 4
    };
    
    enum class GameplayGraphConditionOpcode : std::uint8_t
    {
        Unknown = 0,
        BoolTrue,
        BoolFalse,
        FloatGreater,
        FloatLess
    };
 
    enum class GameplayGraphTaskOpcode : std::uint8_t
    {
        Unknown = 0,
        BeginActionState
    };
 
    struct GameplayGraphTaskDesc
    {
        std::string name{};
        GameplayGraphTaskOpcode opcode{ GameplayGraphTaskOpcode::Unknown };
    };
    
    struct GameplayGraphValue
    {
        GameplayGraphValueType type{ GameplayGraphValueType::Bool };
        bool boolValue{ false };
        int intValue{ 0 };
        float floatValue{ 0.0f };
        std::string stringValue{};
    };

    struct GameplayGraphParameterStore
    {
        std::unordered_map<std::string, GameplayGraphValue> values{};
    };

    struct GameplayGraphEvent
    {
        std::string id{};
    };
    
    struct GameplayGraphConditionDesc
    {
        std::string name{};
        std::string parameter{};
        float threshold{ 0.0f };
        bool boolValue{ false };
        GameplayGraphConditionOpcode opcode{ GameplayGraphConditionOpcode::Unknown };
    };

    struct GameplayGraphTransitionDesc
    {
        std::string toState{};
        std::vector<GameplayGraphConditionDesc> conditions{};
    };

    struct GameplayGraphStateDesc
    {
        std::string name{};
        std::vector<GameplayGraphTaskDesc> onEnter{};
        std::vector<GameplayGraphTaskDesc> onUpdate{};
        std::vector<GameplayGraphTaskDesc> onExit{};
        std::vector<GameplayGraphTransitionDesc> transitions{};
    };

    struct GameplayGraphLayerDesc
    {
        std::string name{};
        std::string defaultState{};
        std::vector<GameplayGraphStateDesc> states{};
    };

    struct GameplayGraphAsset
    {
        std::string id{};
        std::vector<GameplayGraphLayerDesc> layers{};
    };

    struct GameplayGraphLayerRuntimeState
    {
        int activeStateIndex{ -1 };
        int previousStateIndex{ -1 };
        float stateTime{ 0.0f };
        bool enterPending{ true };
    };

    struct GameplayGraphInstance
    {
        const GameplayGraphAsset* asset{ nullptr };
        GameplayGraphParameterStore parameters{};
        GameplayGraphParameterStore blackboard{};
        std::vector<GameplayGraphLayerRuntimeState> layers{};
        std::vector<GameplayGraphEvent> eventsThisFrame{};
        std::vector<std::string> animationTriggersThisFrame{};
    };
    
    std::unordered_map<std::string_view, GameplayGraphConditionOpcode> opcodeStore{
        {"booltrue", GameplayGraphConditionOpcode::BoolTrue},
        {"boolfalse", GameplayGraphConditionOpcode::BoolFalse},
        {"floatgreater", GameplayGraphConditionOpcode::FloatGreater},
        {"floatless", GameplayGraphConditionOpcode::FloatLess}
    };

    [[nodiscard]] inline std::string CanonicalizeGameplayGraphToken(std::string_view value)
    {
        std::string out;
        out.reserve(value.size());
        for (const unsigned char ch : value)
        {
            if (std::isalnum(ch))
            {
                out.push_back(static_cast<char>(std::tolower(ch)));
            }
        }
        return out;
    }

    inline void ClearGameplayGraphFrameState(GameplayGraphInstance& instance)
    {
        for (auto& [name, value] : instance.parameters.values)
        {
            if (value.type == GameplayGraphValueType::Trigger)
            {
                value.boolValue = false;
            }
        }
        instance.eventsThisFrame.clear();
        instance.animationTriggersThisFrame.clear();
    }

    inline void SetGameplayGraphBool(GameplayGraphParameterStore& store, std::string_view name, bool value)
    {
        GameplayGraphValue& entry = store.values[std::string(name)];
        entry.type = GameplayGraphValueType::Bool;
        entry.boolValue = value;
    }

    inline void SetGameplayGraphInt(GameplayGraphParameterStore& store, std::string_view name, int value)
    {
        GameplayGraphValue& entry = store.values[std::string(name)];
        entry.type = GameplayGraphValueType::Int;
        entry.intValue = value;
    }

    inline void SetGameplayGraphFloat(GameplayGraphParameterStore& store, std::string_view name, float value)
    {
        GameplayGraphValue& entry = store.values[std::string(name)];
        entry.type = GameplayGraphValueType::Float;
        entry.floatValue = value;
    }

    inline void SetGameplayGraphString(GameplayGraphParameterStore& store, std::string_view name, std::string value)
    {
        GameplayGraphValue& entry = store.values[std::string(name)];
        entry.type = GameplayGraphValueType::String;
        entry.stringValue = std::move(value);
    }

    [[nodiscard]] inline bool GetGameplayGraphBool(const GameplayGraphParameterStore& store, std::string_view name, bool fallback = false) noexcept
    {
        const auto it = store.values.find(std::string(name));
        if (it == store.values.end())
        {
            return fallback;
        }

        const GameplayGraphValue& value = it->second;
        switch (value.type)
        {
        case GameplayGraphValueType::Bool:
        case GameplayGraphValueType::Trigger:
            return value.boolValue;
        case GameplayGraphValueType::Int:
            return value.intValue != 0;
        case GameplayGraphValueType::Float:
            return value.floatValue > 1e-6f || value.floatValue < -1e-6f;
        default:
            return fallback;
        }
    }

    [[nodiscard]] inline int GetGameplayGraphInt(const GameplayGraphParameterStore& store, std::string_view name, int fallback = 0) noexcept
    {
        const auto it = store.values.find(std::string(name));
        if (it == store.values.end())
        {
            return fallback;
        }

        const GameplayGraphValue& value = it->second;
        switch (value.type)
        {
        case GameplayGraphValueType::Bool:
        case GameplayGraphValueType::Trigger:
            return value.boolValue ? 1 : 0;
        case GameplayGraphValueType::Int:
            return value.intValue;
        case GameplayGraphValueType::Float:
            return static_cast<int>(value.floatValue);
        default:
            return fallback;
        }
    }

    [[nodiscard]] inline float GetGameplayGraphFloat(const GameplayGraphParameterStore& store, std::string_view name, float fallback = 0.0f) noexcept
    {
        const auto it = store.values.find(std::string(name));
        if (it == store.values.end())
        {
            return fallback;
        }

        const GameplayGraphValue& value = it->second;
        switch (value.type)
        {
        case GameplayGraphValueType::Bool:
        case GameplayGraphValueType::Trigger:
            return value.boolValue ? 1.0f : 0.0f;
        case GameplayGraphValueType::Int:
            return static_cast<float>(value.intValue);
        case GameplayGraphValueType::Float:
            return value.floatValue;
        default:
            return fallback;
        }
    }

    [[nodiscard]] inline std::string GetGameplayGraphString(const GameplayGraphParameterStore& store, std::string_view name, std::string fallback = {})
    {
        const auto it = store.values.find(std::string(name));
        if (it == store.values.end() || it->second.type != GameplayGraphValueType::String)
        {
            return fallback;
        }
        return it->second.stringValue;
    }

    inline void SetGameplayGraphTrigger(GameplayGraphParameterStore& store, std::string_view name)
    {
        GameplayGraphValue& entry = store.values[std::string(name)];
        entry.type = GameplayGraphValueType::Trigger;
        entry.boolValue = true;
    }

    [[nodiscard]] inline bool ConsumeGameplayGraphTrigger(GameplayGraphParameterStore& store, std::string_view name) noexcept
    {
        const auto it = store.values.find(std::string(name));
        if (it == store.values.end())
        {
            return false;
        }

        GameplayGraphValue& value = it->second;
        if (value.type != GameplayGraphValueType::Trigger || !value.boolValue)
        {
            return false;
        }
        value.boolValue = false;
        return true;
    }

    inline void PushGameplayGraphEvent(GameplayGraphInstance& instance, std::string eventId)
    {
        instance.eventsThisFrame.push_back(GameplayGraphEvent{ .id = std::move(eventId) });
    }

    [[nodiscard]] inline bool GameplayGraphHasEvent(
        const GameplayGraphInstance& instance, 
        const std::string_view canonicalEventId) noexcept
    {
        const std::string expectedEventId = CanonicalizeGameplayGraphToken(canonicalEventId);

        for (const GameplayGraphEvent& eventDesc : instance.eventsThisFrame)
        {
            if (CanonicalizeGameplayGraphToken(eventDesc.id) == expectedEventId)
            {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] inline int FindGameplayGraphStateIndex(const GameplayGraphLayerDesc& layer, std::string_view stateName) noexcept
    {
        for (std::size_t i = 0; i < layer.states.size(); ++i)
        {
            if (layer.states[i].name == stateName)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    [[nodiscard]] inline GameplayGraphConditionOpcode CompileGameplayGraphConditionOpcode(std::string_view name)
    {
        const std::string canonical = CanonicalizeGameplayGraphToken(name);
        if (opcodeStore.contains(canonical))
        {
            return opcodeStore[canonical];
        }
        return GameplayGraphConditionOpcode::Unknown;
    }
    
    [[nodiscard]] inline GameplayGraphTaskOpcode CompileGameplayGraphTaskOpcode(std::string_view name)
    {
        const std::string canonical = CanonicalizeGameplayGraphToken(name);
        if (canonical == "beginactionstate")
        {
            return GameplayGraphTaskOpcode::BeginActionState;
        }
        return GameplayGraphTaskOpcode::Unknown;
    }

    void PrecompileGameplayGraphTasks(std::vector<GameplayGraphTaskDesc>& tasks)
    {
        for (GameplayGraphTaskDesc& task : tasks)
        {
            task.opcode = CompileGameplayGraphTaskOpcode(task.name);
            if (task.opcode == GameplayGraphTaskOpcode::Unknown)
            {
                std::cerr << "Unknown GameplayGraphTaskOpcode: name=" << task.name << '\n';
            }
        }
    }
    
    void PrecompileGameplayGraphAsset(GameplayGraphAsset& asset)
    {
        for (GameplayGraphLayerDesc& layer : asset.layers)
        {
            for (GameplayGraphStateDesc& state : layer.states)
            {
                PrecompileGameplayGraphTasks(state.onEnter);
                PrecompileGameplayGraphTasks(state.onUpdate);
                PrecompileGameplayGraphTasks(state.onExit);
                
                for (GameplayGraphTransitionDesc& transition : state.transitions)
                {
                    for (GameplayGraphConditionDesc& condition : transition.conditions)
                    {
                        condition.opcode = CompileGameplayGraphConditionOpcode(condition.name);
                        if (condition.opcode == GameplayGraphConditionOpcode::Unknown)
                        {
                            std::cerr << "Unknown GameplayGraphConditionOpcode: name=" << condition.name
                                << ", parameter=" << condition.parameter << '\n';
                        }
                    }
                }
            }
        }
    }
}
