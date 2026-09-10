"""Focused synthetic tests for the engine dependency guards."""

from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from check_engine_dependencies import find_violations
from module_dependencies import discover_modules


class EngineDependencyGuardTests(unittest.TestCase):
    def check_graph(self, declarations):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            for relative_path, (module, imports) in declarations.items():
                path = root / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                import_lines = "".join(f"import :{dependency};\n" for dependency in imports)
                path.write_text(
                    f"export module core:{module};\n{import_lines}", encoding="utf-8"
                )
            return find_violations(discover_modules(root / "src"), root)

    def test_detects_direct_gameplay_backend_dependency(self):
        violations = self.check_graph({
            "src/Gameplay/Gameplay.cppm": ("gameplay", ["rhi_dx12"]),
            "src/Render/DirectX12/DirectX12RHI.cppm": ("rhi_dx12", []),
        })
        self.assertEqual(
            violations,
            [("Gameplay backend dependency", ["gameplay", "rhi_dx12"])],
        )

    def test_detects_transitive_gameplay_backend_dependency(self):
        violations = self.check_graph({
            "src/Gameplay/Gameplay.cppm": ("gameplay", ["intermediate"]),
            "src/Core/Intermediate.cppm": ("intermediate", ["rhi_dx12"]),
            "src/Render/DirectX12/DirectX12RHI.cppm": ("rhi_dx12", []),
        })
        self.assertEqual(
            violations,
            [("Gameplay backend dependency", ["gameplay", "intermediate", "rhi_dx12"])],
        )

    def test_allows_gameplay_scene_generic_rhi_dependency(self):
        violations = self.check_graph({
            "src/Gameplay/Gameplay.cppm": ("gameplay", ["scene"]),
            "src/Scene/Scene.cppm": ("scene", ["rhi"]),
            "src/Render/RHI.cppm": ("rhi", []),
        })
        self.assertEqual(violations, [])

    def test_detects_rhi_upward_dependency(self):
        violations = self.check_graph({
            "src/Render/RHI.cppm": ("rhi", ["gameplay"]),
            "src/Gameplay/Gameplay.cppm": ("gameplay", []),
        })
        self.assertEqual(
            violations,
            [("RHI upward dependency", ["rhi", "gameplay"])],
        )

    def test_allows_backend_rhi_support_dependency(self):
        violations = self.check_graph({
            "src/Render/DirectX12/DirectX12RHI.cppm": ("rhi_dx12", ["dx12_core"]),
            "src/Render/DirectX12/DirectX12Core.cppm": ("dx12_core", []),
        })
        self.assertEqual(violations, [])


if __name__ == "__main__":
    unittest.main()
