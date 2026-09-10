"""Check AI import boundaries without the graphics/MSVC build dependencies.

Run from any directory: python tests/architecture/check_ai_dependencies.py
"""

import re
from pathlib import Path

from module_dependencies import discover_modules


ROOT = Path(__file__).resolve().parents[2]


def main():
    graph = discover_modules(ROOT / "src")
    dependencies = graph.dependencies

    pure_modules = {
        "ai_action_contracts", "ai_agent_world_state", "ai_decision_contracts",
        "ai_state_operations", "ai_goal_selection_contracts", "gameplay_goap_definition_contracts",
    }
    for name in pure_modules:
        unexpected = dependencies(name) - pure_modules
        assert not unexpected, f"{name} imports runtime dependencies: {sorted(unexpected)}"

    for name in ("gameplay_ai_decision_contracts", "gameplay_ai_decision", "gameplay_goap_composition_registry"):
        forbidden = dependencies(name) & {
            "gameplay", "ai_system", "level", "gameplay_traversal_link_registry",
            "gameplay_traversal_executor_registry", "gameplay_object_reservation_system",
        }
        assert not forbidden, f"{name} imports implementation modules: {sorted(forbidden)}"

    for name in ("gameplay_ai_asset_parsing", "gameplay_ai_decision_asset",
                 "gameplay_goap_decision_template", "gameplay_goap_observation_order"):
        forbidden = dependencies(name) & {"gameplay", "ai_system", "level"}
        assert not forbidden, f"{name} imports world implementation: {sorted(forbidden)}"
        assert not any(dep.endswith(("_components", "_component_assets", "_move_to_component"))
                       for dep in dependencies(name)), f"{name} depends on built-in component definitions"

    domain_prefixes = ("gameplay_ai_access_key", "gameplay_ai_target_recovery", "gameplay_ai_buy_key")
    for name in ("gameplay_goap_decision", "gameplay_goap_decision_setup",
                 "gameplay_goap_decision_instance", "gameplay_runtime"):
        forbidden = {
            dep for dep in dependencies(name)
            if dep.startswith(domain_prefixes) or dep == "gameplay_ai_builtin_decisions"
        }
        assert not forbidden, f"{name} imports concrete composition: {sorted(forbidden)}"

    for path in (ROOT / "src/Gameplay/AI/Domains").rglob("*.cppm"):
        source = path.read_text(encoding="utf-8-sig")
        assert not re.search(r"public\s+(?:GameplayAIDecisionInstance|GameplayGOAPDecisionInstance)\b", source), (
            f"Scenario owns a decision implementation: {path.relative_to(ROOT)}")

    print("AI import boundaries: PASS")


if __name__ == "__main__":
    main()
