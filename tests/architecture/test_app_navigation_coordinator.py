#!/usr/bin/env python3
"""Characterize the App navigation progression ownership boundary."""

from pathlib import Path
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]


class AppNavigationCoordinatorTests(unittest.TestCase):
    def test_navigation_progression_has_a_focused_owner(self) -> None:
        header = (REPO_ROOT / "src/App/AppNavigationCoordinator.h").read_text(encoding="utf-8")
        implementation = (
            REPO_ROOT / "src/App/AppNavigationCoordinator.cpp"
        ).read_text(encoding="utf-8")
        lifecycle = (REPO_ROOT / "src/App/AppLifecycle.cpp").read_text(encoding="utf-8")

        self.assertIn("void Advance(appLifecycle::AppState& app);", header)
        self.assertIn("void Advance(appLifecycle::AppState& app)", implementation)
        self.assertIn("GeometryStatus::WaitingForMeshes", implementation)
        self.assertIn("NavigationState::Ready", implementation)
        self.assertIn("NavigationState::Failed", implementation)
        self.assertIn("BuildAgentSettings", implementation)
        self.assertNotIn("UpdateNavigationRuntime", lifecycle)

    def test_lifecycle_replaces_the_call_in_place(self) -> None:
        lifecycle = (REPO_ROOT / "src/App/AppLifecycle.cpp").read_text(encoding="utf-8")

        streaming = lifecycle.index("appRuntime::DriveAssetStreaming(")
        navigation = lifecycle.index("appNavigationCoordinator::Advance(app);")
        scenario = lifecycle.index("app.developmentScenarioRuntime.Update(developmentContext);")
        self.assertLess(streaming, navigation)
        self.assertLess(navigation, scenario)


if __name__ == "__main__":
    unittest.main()
