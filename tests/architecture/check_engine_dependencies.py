"""Check core engine import boundaries without graphics or compiler dependencies.

Run from any directory: python tests/architecture/check_engine_dependencies.py
"""

from pathlib import Path

from module_dependencies import discover_modules


ROOT = Path(__file__).resolve().parents[2]
RHI_SOURCE_PATHS = {
    Path("src/Render/RHI.cppm"),
    Path("src/Render/DirectX12/DirectX12RHI.cppm"),
    Path("src/Render/OpenGL/OpenGLRHI.cppm"),
}


def _violations(graph, rule, sources, forbidden):
    violations = []
    for source in sorted(sources):
        dependencies = graph.dependencies(source)
        for target in sorted(dependencies & forbidden):
            violations.append((rule, graph.dependency_path(source, target)))
    return violations


def find_violations(graph, root):
    backend_modules = (
        graph.owned_by(root / "src/Render/DirectX12")
        | graph.owned_by(root / "src/Render/OpenGL")
    )
    scene_modules = graph.owned_by(root / "src/Scene")
    gameplay_modules = graph.owned_by(root / "src/Gameplay")
    upward_modules = (
        graph.owned_by(root / "src/App")
        | gameplay_modules
        | graph.owned_by(root / "src/Editor")
    )
    rhi_modules = {
        name
        for name, module in graph.modules.items()
        if module.source.resolve().relative_to(root.resolve()) in RHI_SOURCE_PATHS
    }

    return (
        _violations(graph, "Scene backend dependency", scene_modules, backend_modules)
        + _violations(graph, "Gameplay backend dependency", gameplay_modules, backend_modules)
        + _violations(graph, "RHI upward dependency", rhi_modules, upward_modules)
    )


def main():
    graph = discover_modules(ROOT / "src")
    discovered_rhi_paths = {
        module.source.resolve().relative_to(ROOT)
        for module in graph.modules.values()
        if module.source.resolve().relative_to(ROOT) in RHI_SOURCE_PATHS
    }
    missing_rhi_paths = RHI_SOURCE_PATHS - discovered_rhi_paths
    assert not missing_rhi_paths, (
        "RHI module interfaces were not discovered: "
        f"{sorted(str(path) for path in missing_rhi_paths)}"
    )

    violations = find_violations(graph, ROOT)
    if violations:
        details = "\n".join(
            f"{rule}: core:" + " -> core:".join(path)
            for rule, path in violations
        )
        raise AssertionError(details)

    print("Engine import boundaries: PASS")


if __name__ == "__main__":
    main()
