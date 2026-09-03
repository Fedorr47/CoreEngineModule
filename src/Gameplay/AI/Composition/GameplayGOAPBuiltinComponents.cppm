export module core:gameplay_goap_builtin_components;

export import :gameplay_goap_composition_registry;
export import :gameplay_goap_spatial_component_assets;
export import :gameplay_goap_resource_component_assets;
export import :gameplay_goap_move_to_component_assets;
import :gameplay_goap_spatial_components;
import :gameplay_goap_resource_components;
import :gameplay_goap_move_to_component;

export namespace rendern
{
    [[nodiscard]] GameplayGOAPCompositionRegistry MakeDefaultGameplayGOAPComponents()
    {
        GameplayGOAPCompositionRegistry registry;
        RegisterGameplayGOAPSpatialComponents(registry);
        RegisterGameplayGOAPResourceComponents(registry);
        RegisterGameplayGOAPMoveToComponent(registry);
        return registry;
    }
}
