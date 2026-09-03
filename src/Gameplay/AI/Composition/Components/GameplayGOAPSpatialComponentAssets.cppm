module;

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

export module core:gameplay_goap_spatial_component_assets;
export import :gameplay_ai_asset_parsing;

export namespace rendern
{
    struct GameplayAISpatialObservationAsset
    {
        std::string target, fact;
        float radius{};
        bool requireInteractionPoint{};
        bool requireEntity{true};
        bool latch{};
        std::vector<std::string> requiredFacts;
    };
    struct GameplayAIPickupAvailabilityAsset
    {
        std::string target, fact;
        bool respectReservations{};
    };
    struct GameplayAILocationAsset { std::string target, fact; };
    struct GameplayAINearestLocationAsset
    {
        float radius{};
        std::vector<GameplayAILocationAsset> locations;
    };

    GameplayAISpatialObservationAsset ParseGameplayAISpatialObservation(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "target", "fact", "radius", "requireInteractionPoint", "requireEntity", "latch", "requiredFacts"}, source, location);
        GameplayAISpatialObservationAsset parameters{
            .target = String(object, "target", source, location), .fact = String(object, "fact", source, location),
            .radius = Number(object, "radius", 0, source, location),
            .requireInteractionPoint = Boolean(object, "requireInteractionPoint", false, source, location),
            .requireEntity = Boolean(object, "requireEntity", true, source, location),
            .latch = Boolean(object, "latch", false, source, location)};
        if (object.contains("requiredFacts"))
        {
            parameters.requiredFacts = Strings(object, "requiredFacts", source, location);
        }
        return parameters;
    }
    GameplayAIPickupAvailabilityAsset ParseGameplayAIPickupAvailability(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "target", "fact", "respectReservations"}, source, location);
        return GameplayAIPickupAvailabilityAsset{
            String(object, "target", source, location), String(object, "fact", source, location),
            Boolean(object, "respectReservations", false, source, location)};
    }
    GameplayAINearestLocationAsset ParseGameplayAINearestLocation(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "radius", "locations"}, source, location);
        GameplayAINearestLocationAsset parameters{.radius = Number(object, "radius", 0, source, location)};
        for (const auto& item : Array(object, "locations", source, location))
        {
            const auto& node = Object(item, source, location);
            Fields(node, {"target", "fact"}, source, location);
            parameters.locations.push_back({String(node, "target", source, location), String(node, "fact", source, location)});
        }
        return parameters;
    }
}
