"""Guard the Scene -> RenderFramePacket -> renderer capability boundary."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
FRAME_PACKET = ROOT / "src/Render/Frame/RenderFramePacket.cppm"
RENDERER_FILES = [
    ROOT / "src/Render/OpenGL/OpenGLRenderer.cppm",
    ROOT / "src/Render/DirectX12/DirectX12Renderer.cppm",
    *sorted((ROOT / "src/Render/DirectX12/RendererImpl").glob("*.inl")),
]


def main():
    violations = []
    frame_packet = FRAME_PACKET.read_text(encoding="utf-8")
    checks = {
        "RenderFramePacket exposes GetScene()": r"\bGetScene\s*\(",
        "RenderFramePacket stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
        "RenderFramePacket stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
    }
    for message, pattern in checks.items():
        if re.search(pattern, frame_packet):
            violations.append(f"{FRAME_PACKET.relative_to(ROOT)}: {message}")

    for view_type in ("RenderWorldSnapshot", "RenderDebugSnapshot", "RenderEditorSnapshot"):
        if not re.search(rf"\bstruct\s+{view_type}\b", frame_packet):
            violations.append(
                f"{FRAME_PACKET.relative_to(ROOT)}: missing grouped {view_type} contract"
            )

    if "RenderFrameView" in frame_packet or "RenderEditorView" in frame_packet or "render_frame_view" in frame_packet:
        violations.append(f"{FRAME_PACKET.relative_to(ROOT)}: obsolete frame-view terminology remains")

    world_start = frame_packet.find("struct RenderSkinnedDrawItem")
    world_end = frame_packet.find("struct RenderDebugSnapshot")
    if world_start < 0 or world_end < 0 or world_start >= world_end:
        violations.append(
            f"{FRAME_PACKET.relative_to(ROOT)}: cannot isolate owned World snapshot contract"
        )
    else:
        owned_world_contract = frame_packet[world_start:world_end]
        owned_world_checks = {
            "owned World contract uses std::span": r"\bstd::span\b",
            "owned World contract exposes AnimatorState": r"\bAnimatorState\b",
            "owned World contract exposes AnimationControllerRuntime": r"\bAnimationControllerRuntime\b",
            "owned World contract stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
            "owned World contract stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
        }
        for message, pattern in owned_world_checks.items():
            if re.search(pattern, owned_world_contract):
                violations.append(f"{FRAME_PACKET.relative_to(ROOT)}: {message}")

    debug_start = frame_packet.find("struct RenderDebugSnapshot")
    debug_end = frame_packet.find("struct RenderEditorSnapshot")
    if debug_start < 0 or debug_end < 0 or debug_start >= debug_end:
        violations.append(
            f"{FRAME_PACKET.relative_to(ROOT)}: cannot isolate owned Debug snapshot contract"
        )
    else:
        owned_debug_contract = frame_packet[debug_start:debug_end]
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
                violations.append(f"{FRAME_PACKET.relative_to(ROOT)}: {message}")

    editor_start = frame_packet.find("struct RenderEditorSnapshot")
    editor_end = frame_packet.find("struct RenderFramePacket")
    if editor_start < 0 or editor_end < 0 or editor_start >= editor_end:
        violations.append(
            f"{FRAME_PACKET.relative_to(ROOT)}: cannot isolate owned Editor snapshot contract"
        )
    else:
        owned_editor_contract = frame_packet[editor_start:editor_end]
        owned_editor_checks = {
            "owned Editor contract uses std::span": r"\bstd::span\b",
            "owned Editor contract stores a Scene pointer": r"\bScene\s*(?:const\s*)?\*",
            "owned Editor contract stores a Scene reference": r"\bScene\s*(?:const\s*)?&",
            "owned Editor contract stores a gizmo pointer/reference":
                r"\b(?:Translate|Rotate|Scale)GizmoState\s*(?:const\s*)?[&*]\s*\w+_\s*;",
        }
        for message, pattern in owned_editor_checks.items():
            if re.search(pattern, owned_editor_contract):
                violations.append(f"{FRAME_PACKET.relative_to(ROOT)}: {message}")

    flat_accessors = (
        "GetCamera",
        "GetDrawItems",
        "GetExternalDebugLines",
        "GetEditorSelectedLights",
    )
    frame_packet_body = frame_packet[frame_packet.find("struct RenderFramePacket") :]
    for accessor in flat_accessors:
        if re.search(rf"\b{accessor}\s*\(", frame_packet_body):
            violations.append(
                f"{FRAME_PACKET.relative_to(ROOT)}: RenderFramePacket retains flat {accessor} capability"
            )

    for path in RENDERER_FILES:
        source = path.read_text(encoding="utf-8")
        if re.search(r"\bframePacket\s*\.\s*GetScene\s*\(", source):
            violations.append(
                f"{path.relative_to(ROOT)}: renderer recovers Scene through RenderFramePacket"
            )
        if re.search(r"RenderFrame\s*\([^)]*\bScene\s*(?:const\s*)?[&*]", source, re.DOTALL):
            violations.append(
                f"{path.relative_to(ROOT)}: renderer frame interface directly accepts Scene"
            )

    if violations:
        raise AssertionError("\n".join(violations))
    print("RenderFramePacket Scene capability boundary: PASS")


if __name__ == "__main__":
    main()
