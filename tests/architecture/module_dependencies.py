"""Minimal C++ module graph support shared by architecture checks."""

from collections import deque
from dataclasses import dataclass
from pathlib import Path
import re


DECLARATION = re.compile(r"^export module core:([^;]+);", re.MULTILINE)
IMPORT = re.compile(r"^(?:export )?import\s+:([^;]+);", re.MULTILINE)


@dataclass(frozen=True)
class Module:
    source: Path
    imports: frozenset[str]


class ModuleGraph:
    def __init__(self, modules):
        self.modules = modules

    def dependencies(self, start):
        visited = set()
        pending = [start]
        while pending:
            name = pending.pop()
            if name in visited:
                continue
            visited.add(name)
            if name not in self.modules:
                raise AssertionError(f"Missing partition: {name}")
            pending.extend(self.modules[name].imports)
        return visited

    def dependency_path(self, start, target):
        pending = deque([(start, [start])])
        visited = set()
        while pending:
            name, path = pending.popleft()
            if name in visited:
                continue
            visited.add(name)
            if name not in self.modules:
                raise AssertionError(f"Missing partition: {name}")
            if name == target:
                return path
            pending.extend(
                (dependency, path + [dependency])
                for dependency in sorted(self.modules[name].imports)
            )
        return None

    def owned_by(self, directory):
        directory = directory.resolve()
        return {
            name
            for name, module in self.modules.items()
            if module.source.resolve().is_relative_to(directory)
        }


def discover_modules(source_root):
    modules = {}
    sources = list(source_root.rglob("*.cppm")) + list(source_root.rglob("*.ixx"))
    for path in sources:
        source = path.read_text(encoding="utf-8-sig")
        declaration = DECLARATION.search(source)
        if not declaration:
            continue
        name = declaration.group(1)
        if name in modules:
            raise AssertionError(f"Duplicate partition: {name}")
        modules[name] = Module(path, frozenset(IMPORT.findall(source)))
    return ModuleGraph(modules)
