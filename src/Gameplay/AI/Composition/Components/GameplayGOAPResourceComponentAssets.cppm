module;

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

export module core:gameplay_goap_resource_component_assets;
export import :gameplay_ai_asset_parsing;

export namespace rendern
{
    struct GameplayAIResourcePickupAsset
    {
        std::string target, fact;
        std::int32_t amount{1};
    };
    struct GameplayAIResourceReceiptAsset
    {
        std::string id, target, fact;
        std::int32_t price{};
    };
    struct GameplayAIResourceLedgerAsset
    {
        std::string fact;
        std::vector<GameplayAIResourcePickupAsset> pickups;
        std::vector<GameplayAIResourceReceiptAsset> receipts;
    };
    struct GameplayAIPurchaseAsset
    {
        std::string receipt;
        float radius{0.6f};
    };
    struct GameplayAIHideOnPurchaseAsset
    {
        std::string receipt, target;
    };

    GameplayAIResourceLedgerAsset ParseGameplayAIResourceLedger(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "fact", "pickups", "receipts"}, source, location);
        GameplayAIResourceLedgerAsset parameters{.fact = String(object, "fact", source, location)};
        for (const auto& item : Array(object, "pickups", source, location))
        {
            const auto& pickup = Object(item, source, location);
            Fields(pickup, {"target", "fact", "amount"}, source, location);
            parameters.pickups.push_back({String(pickup, "target", source, location),
                String(pickup, "fact", source, location), static_cast<std::int32_t>(
                    PositiveInteger(pickup, "amount", INT32_MAX, source, location))});
        }
        for (const auto& item : Array(object, "receipts", source, location))
        {
            const auto& receipt = Object(item, source, location);
            Fields(receipt, {"id", "target", "fact", "price"}, source, location);
            parameters.receipts.push_back({String(receipt, "id", source, location),
                String(receipt, "target", source, location), String(receipt, "fact", source, location),
                static_cast<std::int32_t>(PositiveInteger(receipt, "price", INT32_MAX, source, location))});
        }
        return parameters;
    }
    GameplayAIPurchaseAsset ParseGameplayAIPurchase(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "context", "receipt", "radius"}, source, location);
        return GameplayAIPurchaseAsset{String(object, "receipt", source, location),
            Number(object, "radius", 0.6f, source, location)};
    }
    GameplayAIHideOnPurchaseAsset ParseGameplayAIHideOnPurchase(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        const auto& object = input.object;
        const auto source = input.source, location = input.location;
        Fields(object, {"type", "receipt", "target"}, source, location);
        return {String(object, "receipt", source, location), String(object, "target", source, location)};
    }
}
