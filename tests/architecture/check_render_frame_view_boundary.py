"""Guard the Scene -> RenderFrameView -> renderer capability boundary."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
FRAME_VIEW = ROOT / "src/Render/Frame/RenderFrameView.cppm"
RENDERER_FILES = [
    ROOT / "src/Render/OpenGL/OpenGLRenderer.cppm",
    ROOT / "src/Render/DirectX12/DirectX12Renderer.cppm",
    *sorted((ROOT / "src/Render/DirectX12/RendererImpl").glob("*.inl")),
]


def main():
    violations = []
    frame_view = FRAME_VIEW.read_text(encoding="utf-8")
    checks = {
        "RenderFrameView exposes GetScene()": r"\bGetScene\s*\(",
        "RenderFrameView stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
        "RenderFrameView stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
    }
    for message, pattern in checks.items():
        if re.search(pattern, frame_view):
            violations.append(f"{FRAME_VIEW.relative_to(ROOT)}: {message}")

    for view_type in ("RenderWorldSnapshot", "RenderDebugSnapshot", "RenderEditorView"):
        if not re.search(rf"\bstruct\s+{view_type}\b", frame_view):
            violations.append(
                f"{FRAME_VIEW.relative_to(ROOT)}: missing grouped {view_type} contract"
            )

    world_start = frame_view.find("struct RenderSkinnedDrawItem")
    world_end = frame_view.find("struct RenderDebugSnapshot")
    if world_start < 0 or world_end < 0 or world_start >= world_end:
        violations.append(
            f"{FRAME_VIEW.relative_to(ROOT)}: cannot isolate owned World snapshot contract"
        )
    else:
        owned_world_contract = frame_view[world_start:world_end]
        owned_world_checks = {
            "owned World contract uses std::span": r"\bstd::span\b",
            "owned World contract exposes AnimatorState": r"\bAnimatorState\b",
            "owned World contract exposes AnimationControllerRuntime": r"\bAnimationControllerRuntime\b",
            "owned World contract stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
            "owned World contract stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
        }
        for message, pattern in owned_world_checks.items():
            if re.search(pattern, owned_world_contract):
                violations.append(f"{FRAME_VIEW.relative_to(ROOT)}: {message}")

    debug_start = frame_view.find("struct RenderDebugSnapshot")
    debug_end = frame_view.find("struct RenderEditorView")
    if debug_start < 0 or debug_end < 0 or debug_start >= debug_end:
        violations.append(
            f"{FRAME_VIEW.relative_to(ROOT)}: cannot isolate owned Debug snapshot contract"
        )
    else:
        owned_debug_contract = frame_view[debug_start:debug_end]
        owned_debug_checks = {
            "owned Debug contract uses std::span": r"\bstd::span\b",
            "owned Debug contract stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
            "owned Debug contract stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
            "owned Debug contract stores a DebugRay pointer/reference":
                r"\bDebugRay\s*(?:const\s*)?[&*]\s*\w+_\s*;",
            "owned Debug contract stores a GameplayMovementDebugState pointer/reference":
                r"\bGameplayMovementDebugState\s*(?:const\s*)?[&*]\s*\w+_\s*;",
        }
        for message, pattern in owned_debug_checks.items():
            if re.search(pattern, owned_debug_contract):
                violations.append(f"{FRAME_VIEW.relative_to(ROOT)}: {message}")

    flat_accessors = (
        "GetCamera",
        "GetDrawItems",
        "GetExternalDebugLines",
        "GetEditorSelectedLights",
    )
    frame_view_body = frame_view[frame_view.find("struct RenderFrameView") :]
    for accessor in flat_accessors:
        if re.search(rf"\b{accessor}\s*\(", frame_view_body):
            violations.append(
                f"{FRAME_VIEW.relative_to(ROOT)}: RenderFrameView retains flat {accessor} capability"
            )

    for path in RENDERER_FILES:
        source = path.read_text(encoding="utf-8")
        if re.search(r"\bframeView\s*\.\s*GetScene\s*\(", source):
            violations.append(
                f"{path.relative_to(ROOT)}: renderer recovers Scene through RenderFrameView"
            )
        if re.search(r"RenderFrame\s*\([^)]*\bScene\s*(?:const\s*)?[&*]", source, re.DOTALL):
            violations.append(
                f"{path.relative_to(ROOT)}: renderer frame interface directly accepts Scene"
            )

    if violations:
        raise AssertionError("\n".join(violations))
    print("RenderFrameView Scene capability boundary: PASS")


if __name__ == "__main__":
    main()
