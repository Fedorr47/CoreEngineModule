#!/usr/bin/env python3
"""Build compact, deterministic navigation indexes for CoreEngineModule."""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "docs/refactor/index"
SCAN_ROOTS = ("src", "tests", "tools", "docs", "assets/ai")
EXCLUDED_DIRS = {
    ".git", "out", "build", ".idea", ".vs", ".vscode", "extern",
    "third_party", "node_modules", "__pycache__",
}
EXCLUDED_SUFFIXES = {".obj", ".lib", ".dll", ".exe", ".pdb", ".ifc", ".pch"}
BINARY_SUFFIXES = {
    ".fbx", ".gltf", ".glb", ".png", ".jpg", ".jpeg", ".dds", ".tga",
    ".wav", ".mp3", ".mp4", ".gif", ".bmp", ".ico", ".zip", ".bin",
}
CPP_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl", ".ixx", ".cppm"}
TEXT_SUFFIXES = CPP_SUFFIXES | {".md", ".txt", ".tsv", ".cmake", ".py", ".json", ".yml", ".yaml"}
SUBSYSTEMS = (
    "Animation", "App", "Assets", "Core", "ECS", "Editor", "Gameplay", "AI",
    "Input", "Level", "Navigation", "Physics", "Render", "Scene",
)
ENTRY_WORDS = ("runtime", "world", "system", "bootstrap", "renderer", "scene", "lifecycle", "manager")
MAX_LIST = 15


@dataclass(frozen=True)
class FileInfo:
    path: str
    subsystem: str
    kind: str
    lines: int
    size: int
    module: str
    imports: tuple[str, ...]
    test: bool


def excluded(relative: Path) -> bool:
    """Return whether a repository-relative path is outside indexing scope."""
    if relative.suffix.lower() in EXCLUDED_SUFFIXES:
        return True
    return any(part in EXCLUDED_DIRS or part.startswith("cmake-build-") for part in relative.parts)


def classify_subsystem(path: str) -> str:
    """Classify a path using stable directory/name rules; AI takes precedence."""
    lower = path.lower()
    parts = [part.lower() for part in Path(path).parts]
    if lower.startswith("assets/ai/") or lower.startswith("src/gameplay/ai/") or "/ai/" in lower:
        return "AI"
    directory_rules = (
        ("animation", "Animation"), ("app", "App"), ("assets", "Assets"),
        ("core", "Core"), ("ecs", "ECS"), ("editor", "Editor"),
        ("gameplay", "Gameplay"), ("input", "Input"), ("level", "Level"),
        ("navigation", "Navigation"), ("physics", "Physics"),
        ("render", "Render"), ("scene", "Scene"),
    )
    for marker, result in directory_rules:
        if marker in parts[1:] or any(part == marker + "tests" for part in parts):
            return result
    stem = Path(path).stem.lower()
    for marker, result in directory_rules:
        if marker in stem:
            return result
    return "Other"


def strip_cpp_comments(text: str) -> str:
    """Remove C++ comments while preserving strings and newlines."""
    result: list[str] = []
    index = 0
    state = "code"
    while index < len(text):
        char = text[index]
        nxt = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and nxt == "/":
                result.extend("  "); index += 2; state = "line"; continue
            if char == "/" and nxt == "*":
                result.extend("  "); index += 2; state = "block"; continue
            if char in {'"', "'"}:
                state = char
            result.append(char)
        elif state == "line":
            result.append("\n" if char == "\n" else " ")
            if char == "\n": state = "code"
        elif state == "block":
            if char == "*" and nxt == "/":
                result.extend("  "); index += 2; state = "code"; continue
            result.append("\n" if char == "\n" else " ")
        else:
            result.append(char)
            if char == "\\" and nxt:
                result.append(nxt); index += 2; continue
            if char == state: state = "code"
        index += 1
    return "".join(result)


MODULE_RE = re.compile(r"(?m)^\s*(?:export\s+)?module\s+([A-Za-z_][\w.]*?(?::[\w.]+)?)[ \t]*;")
IMPORT_RE = re.compile(r"(?m)^\s*(?:export\s+)?import\s+([:]?[A-Za-z_][\w.]*(?::[\w.]+)?)[ \t]*;")


def parse_cpp_module(text: str) -> tuple[str, tuple[str, ...]]:
    cleaned = strip_cpp_comments(text)
    declaration = MODULE_RE.search(cleaned)
    module = declaration.group(1) if declaration else ""
    imports: set[str] = set()
    for match in IMPORT_RE.finditer(cleaned):
        imported = match.group(1)
        if imported.startswith(":"):
            if module:
                imported = module.split(":", 1)[0] + imported
            else:
                continue
        if imported != "std" and not imported.startswith("std."):
            imports.add(imported)
    return module, tuple(sorted(imports))


def discover_files() -> list[Path]:
    paths: list[Path] = []
    for root_name in SCAN_ROOTS:
        scan_root = ROOT / root_name
        if not scan_root.exists():
            continue
        for path in scan_root.rglob("*"):
            relative = path.relative_to(ROOT)
            if not path.is_file() or excluded(relative):
                continue
            # Generated output cannot be input to its own next run.
            if relative.parts[:3] == ("docs", "refactor", "index"):
                continue
            paths.append(path)
    return sorted(set(paths), key=lambda item: item.relative_to(ROOT).as_posix())


def collect_file_metrics(paths: list[Path]) -> tuple[list[FileInfo], list[str]]:
    files: list[FileInfo] = []
    warnings: list[str] = []
    for path in paths:
        relative = path.relative_to(ROOT).as_posix()
        suffix = path.suffix.lower()
        size = path.stat().st_size
        lines = 0
        module = ""
        imports: tuple[str, ...] = ()
        if suffix not in BINARY_SUFFIXES and (suffix in TEXT_SUFFIXES or path.name == "CMakeLists.txt"):
            try:
                text = path.read_text(encoding="utf-8-sig")
                lines = len(text.splitlines())
                if suffix in CPP_SUFFIXES:
                    module, imports = parse_cpp_module(text)
            except UnicodeDecodeError:
                warnings.append(f"non-UTF-8 text omitted: {relative}")
        kind = suffix[1:] if suffix else ("cmake" if path.name == "CMakeLists.txt" else "file")
        files.append(FileInfo(relative, classify_subsystem(relative), kind, lines, size,
                              module, imports, relative.startswith("tests/")))
    return files, warnings


def module_edges(files: list[FileInfo]) -> list[tuple[str, str, str]]:
    repository_modules = {info.module for info in files if info.module}
    return sorted({(info.module, imported, info.path) for info in files if info.module
                   for imported in info.imports if imported in repository_modules})


def build_test_map(files: list[FileInfo]) -> list[tuple[str, str, str]]:
    declarations = {info.module: info.path for info in files if info.module and not info.test}
    production = [info for info in files if not info.test and info.path.startswith("src/")]
    mappings: set[tuple[str, str, str]] = set()
    for test in (info for info in files if info.test):
        for imported in test.imports:
            if imported in declarations:
                mappings.add((declarations[imported], test.path, "imports"))
        test_key = re.sub(r"^(test|tests)", "", Path(test.path).stem.lower())
        if len(test_key) >= 5:
            candidates = [item for item in production
                          if re.sub(r"^(test|tests)", "", Path(item.path).stem.lower()) == test_key]
            for item in candidates:
                mappings.add((item.path, test.path, "name-match"))
    return sorted(mappings)


def write_tsv(name: str, header: tuple[str, ...], rows) -> None:
    content = ["\t".join(header)] + ["\t".join(map(str, row)) for row in rows]
    (OUTPUT / name).write_text("\n".join(content) + "\n", encoding="utf-8", newline="\n")


def generated_header(title: str) -> list[str]:
    return [f"# {title}", "", "Generated by `tools/refactor/build_repo_index.py`.",
            "Do not edit manually.", "",
            "> Navigation aid only; source code and architecture checks remain authoritative.", ""]


def primary_paths(subsystem: str, files: list[FileInfo]) -> list[str]:
    prefixes = Counter()
    for info in files:
        if info.subsystem == subsystem:
            parts = Path(info.path).parts
            depth = 3 if subsystem == "AI" and parts[:3] == ("src", "Gameplay", "AI") else min(2, len(parts))
            prefixes["/".join(parts[:depth]) + "/"] += 1
    return [path for path, _ in sorted(prefixes.items(),
                                       key=lambda item: (not item[0].startswith("src/"), -item[1], item[0]))[:4]]


def calculate_hotspots(files: list[FileInfo], edges, mappings):
    fan_in = Counter(imported for _, imported, _ in edges)
    source_fan_out = Counter(source for _, _, source in edges)
    tested = Counter(production for production, _, _ in mappings)
    module_subsystem = {info.module: info.subsystem for info in files if info.module}
    cross = Counter(source for module, imported, source in edges
                    if module_subsystem.get(module) != module_subsystem.get(imported))
    ranked = []
    for info in files:
        if info.test or not info.path.startswith("src/") or info.kind not in {"cpp", "cppm", "ixx", "h", "hpp"}:
            continue
        score = min(info.lines // 250, 8) + min(info.size // 20000, 5) + min(len(info.imports) // 4, 5)
        score += min(fan_in[info.module] // 3, 6) + min(source_fan_out[info.path] // 3, 5)
        score += min(cross[info.path] // 2, 4) + (1 if tested[info.path] else 0)
        if score:
            signals = f"{info.lines} LOC; {len(info.imports)} imports"
            if info.module: signals += f"; fan-in {fan_in[info.module]}; fan-out {source_fan_out[info.path]}"
            ranked.append((score, info.path, info.subsystem, signals))
    return sorted(ranked, key=lambda row: (-row[0], row[1]))[:40]


def write_repo(files, edges, mappings, hotspots) -> None:
    modules = {info.module for info in files if info.module}
    lines = generated_header("Repository Index")
    lines += ["## Summary", "", f"- Indexed files: **{len(files)}**",
              f"- Source-tree files: **{sum(info.path.startswith('src/') for info in files)}**",
              f"- Test files: **{sum(info.test for info in files)}**", f"- Repository modules: **{len(modules)}**",
              f"- Repository dependency edges: **{len(edges)}**", f"- Test mappings: **{len(mappings)}**", "",
              "Regenerate with `python tools/refactor/build_repo_index.py`.", "", "## Subsystems", "",
              "| Subsystem | Primary path | Files | Tests | Index |", "|---|---|---:|---:|---|"]
    for subsystem in SUBSYSTEMS:
        members = [info for info in files if info.subsystem == subsystem]
        primary = primary_paths(subsystem, files)
        lines.append(f"| {subsystem} | `{primary[0] if primary else '—'}` | {len(members)} | {sum(i.test for i in members)} | [open](subsystems/{subsystem.lower()}.md) |")
    architecture_docs = sorted(info.path for info in files if info.path.startswith("docs/architecture/") and info.kind == "md")
    lines += ["", "## Navigation", "", "1. Open the relevant subsystem page above.",
              "2. Use [`module-deps.tsv`](module-deps.tsv) or [`test-map.tsv`](test-map.tsv) for exact relationships.",
              "3. Inspect only the exact source and test files needed.", "",
              f"See the bounded [structural hotspots](hotspots.md) ({len(hotspots)} candidates) and [file metrics](file-metrics.tsv).", "",
              "## Architecture documentation", ""]
    lines += [f"- [`{path}`](../../architecture/{Path(path).name})" for path in architecture_docs] or ["- None discovered."]
    (OUTPUT / "repo.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def write_hotspots(hotspots) -> None:
    lines = generated_header("Structural Hotspots")
    lines += ["These are investigation candidates, not a refactoring priority list or a design-quality judgment.", "",
              "## Mechanical scoring", "", "Score = capped points for each 250 LOC, 20 KB, four imports, module fan-in/fan-out, cross-subsystem edges, and a mapped test. Ties sort by path.", "",
              "| Score | File | Subsystem | Signals |", "|---:|---|---|---|"]
    lines += [f"| {score} | `{path}` | {subsystem} | {signals} |" for score, path, subsystem, signals in hotspots]
    lines += ["", "Each row means **structural hotspot — inspect before planning refactoring**."]
    (OUTPUT / "hotspots.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def capped(items, empty="None discovered.") -> list[str]:
    values = list(items)
    result = [f"- `{item}`" for item in values[:MAX_LIST]] or [f"- {empty}"]
    if len(values) > MAX_LIST: result.append(f"- _{len(values) - MAX_LIST} more; see the TSV indexes._")
    return result


def write_subsystems(files, edges, mappings, hotspots) -> None:
    module_owner = {info.module: info.subsystem for info in files if info.module}
    tests_by_subsystem = defaultdict(set)
    for production, test, _ in mappings:
        tests_by_subsystem[classify_subsystem(production)].add(test)
    architecture_docs = sorted(info.path for info in files if info.path.startswith("docs/architecture/") and info.kind == "md")
    for subsystem in SUBSYSTEMS:
        members = [info for info in files if info.subsystem == subsystem]
        modules = sorted({info.module for info in members if info.module})
        direct = Counter(imported for module, imported, _ in edges if module in modules and module_owner.get(imported) != subsystem)
        reverse = Counter(module for module, imported, _ in edges if imported in modules and module_owner.get(module) != subsystem)
        entries = sorted((info for info in members if info.module), key=lambda info: (
            -sum(1 for _, imported, _ in edges if imported == info.module),
            -sum(word in Path(info.path).stem.lower() for word in ENTRY_WORDS), info.path))[:10]
        tests = sorted({info.path for info in members if info.test} | tests_by_subsystem[subsystem])
        docs = [path for path in architecture_docs if subsystem.lower() in Path(path).stem.lower()]
        if subsystem in {"Scene", "Level", "Editor", "Render"}:
            docs += [path for path in architecture_docs if "scene-level-editor-render" in path]
        docs = sorted(set(docs))
        local_hotspots = [path for _, path, owner, _ in hotspots if owner == subsystem]
        lines = generated_header(f"{subsystem} Subsystem Index")
        lines += ["## Primary paths", ""] + capped(primary_paths(subsystem, files))
        lines += ["", "## Counts", "", f"- Files: **{len(members)}**", f"- Declared modules: **{len(modules)}**", f"- Test files: **{len(tests)}**"]
        lines += ["", "## Modules", ""] + capped(modules)
        lines += ["", "## Likely entry points", "", "Inferred mechanically from fan-in and common entry-point names."] + capped(info.path for info in entries)
        lines += ["", "## Dependencies", "", "Top direct repository-module dependencies:"] + capped(name for name, _ in direct.most_common(MAX_LIST))
        lines += ["", "Top reverse repository-module dependencies:"] + capped(name for name, _ in reverse.most_common(MAX_LIST))
        lines += ["", "For the complete machine-readable graph, see [`../module-deps.tsv`](../module-deps.tsv)."]
        lines += ["", "## Tests", ""] + capped(tests)
        lines += ["", "## Relevant docs", ""] + capped(docs)
        if subsystem == "AI":
            ai_assets = sorted({"/".join(Path(info.path).parts[:3]) + "/" for info in members if info.path.startswith("assets/ai/")})
            lines += ["", "## Authored AI data", "", "Paths are indexed by metadata only; asset contents are not summarized."] + capped(ai_assets)
        lines += ["", "## Hotspots", ""] + capped(local_hotspots)
        (OUTPUT / "subsystems" / f"{subsystem.lower()}.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def self_validate() -> None:
    sample = "// export module fake;\nexport module core:demo;\n/* import core:no; */\nimport :part;\nimport std;\nimport core:real;\n"
    assert parse_cpp_module(sample) == ("core:demo", ("core:part", "core:real"))
    assert classify_subsystem("src/Gameplay/AI/Runtime/X.cppm") == "AI"
    assert classify_subsystem("tests/unit/RenderTests/TestX.cpp") == "Render"
    assert excluded(Path("out/generated.obj")) and excluded(Path("extern/x.cpp"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run parser/classifier checks and exit")
    args = parser.parse_args()
    self_validate()
    if args.self_test:
        print("Repository index self-test: PASS")
        return 0
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / "subsystems").mkdir(parents=True, exist_ok=True)
    files, warnings = collect_file_metrics(discover_files())
    edges = module_edges(files)
    mappings = build_test_map(files)
    hotspots = calculate_hotspots(files, edges, mappings)
    write_tsv("file-metrics.tsv", ("path", "subsystem", "kind", "lines", "bytes", "module", "import_count", "test"),
              ((i.path, i.subsystem, i.kind, i.lines, i.size, i.module, len(i.imports), int(i.test)) for i in files))
    write_tsv("module-deps.tsv", ("module", "imports", "source"), edges)
    write_tsv("test-map.tsv", ("production", "test", "relation"), mappings)
    write_repo(files, edges, mappings, hotspots)
    write_hotspots(hotspots)
    write_subsystems(files, edges, mappings, hotspots)
    print(f"Indexed {len(files)} files, {len({i.module for i in files if i.module})} modules, "
          f"{len(edges)} dependency edges, {len(mappings)} test mappings, {len(hotspots)} hotspots.")
    if warnings:
        print(f"Warnings ({len(warnings)}):", file=sys.stderr)
        for warning in warnings[:10]: print(f"  {warning}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
