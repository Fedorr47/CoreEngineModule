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
