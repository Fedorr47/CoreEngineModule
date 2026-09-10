#!/usr/bin/env python3
"""Focused standard-library tests for the repository indexer."""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools/refactor/build_repo_index.py"
SPEC = importlib.util.spec_from_file_location("build_repo_index", SCRIPT)
indexer = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
sys.modules[SPEC.name] = indexer
SPEC.loader.exec_module(indexer)


class RepositoryIndexTests(unittest.TestCase):
    def test_module_partition_imports_and_comments(self):
        source = """module;\n// export module wrong;\nexport module core:render;\n/* import core:wrong; */\nimport :rhi;\nexport import core:scene;\nimport std;\n"""
        self.assertEqual(indexer.parse_cpp_module(source),
                         ("core:render", ("core:rhi", "core:scene")))

    def test_subsystem_classification(self):
        self.assertEqual(indexer.classify_subsystem("src/Gameplay/AI/GOAP/X.cppm"), "AI")
        self.assertEqual(indexer.classify_subsystem("tests/unit/PhysicsTests/TestWorld.cpp"), "Physics")
        self.assertEqual(indexer.classify_subsystem("src/Render/RHI.cppm"), "Render")

    def test_exclusions(self):
        self.assertTrue(indexer.excluded(Path("out/cache/file.cppm")))
        self.assertTrue(indexer.excluded(Path("cmake-build-debug/file.cpp")))
        self.assertTrue(indexer.excluded(Path("src/generated.obj")))
        self.assertFalse(indexer.excluded(Path("src/Core/File.cppm")))

    def test_dependency_edges_are_sorted_and_repository_only(self):
        files = [
            indexer.FileInfo("src/B.cppm", "Core", "cppm", 1, 1, "core:b", ("external", "core:a"), False),
            indexer.FileInfo("src/A.cppm", "Core", "cppm", 1, 1, "core:a", (), False),
        ]
        self.assertEqual(indexer.module_edges(files), [("core:b", "core:a", "src/B.cppm")])

    def test_hotspot_fan_out_is_attributed_to_its_source_file(self):
        files = [
            indexer.FileInfo("src/Umbrella.cppm", "Core", "cppm", 250, 1, "core", ("core:a",), False),
            indexer.FileInfo("src/Implementation.cpp", "Core", "cpp", 1, 1, "core", (), False),
            indexer.FileInfo("src/A.cppm", "Core", "cppm", 1, 1, "core:a", (), False),
        ]
        hotspots = indexer.calculate_hotspots(files, indexer.module_edges(files), [])
        signals = {path: detail for _, path, _, detail in hotspots}
        self.assertIn("fan-out 1", signals["src/Umbrella.cppm"])
        self.assertNotIn("src/Implementation.cpp", signals)

    def test_tsv_writing_has_stable_order_and_newlines(self):
        with tempfile.TemporaryDirectory() as directory:
            old_output = indexer.OUTPUT
            try:
                indexer.OUTPUT = Path(directory)
                rows = sorted([("b", "2"), ("a", "1")])
                indexer.write_tsv("sample.tsv", ("key", "value"), rows)
                first = (Path(directory) / "sample.tsv").read_bytes()
                indexer.write_tsv("sample.tsv", ("key", "value"), rows)
                self.assertEqual(first, (Path(directory) / "sample.tsv").read_bytes())
                self.assertEqual(first, b"key\tvalue\na\t1\nb\t2\n")
            finally:
                indexer.OUTPUT = old_output


if __name__ == "__main__":
    unittest.main()
