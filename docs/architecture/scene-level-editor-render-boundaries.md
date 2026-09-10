# Scene / Level / Editor / Render boundaries

_Date: 2026-05-10_

This note records the current ownership split at the Scene-to-renderer boundary.

## Intentional ownership model

- **SceneWorld / Scene (`core:scene`)**
  - Authoritative runtime world representation used by gameplay/runtime systems.
  - Holds runtime scene entities, transforms, camera/light state, and runtime debug state.
  - Must not be treated as renderer-owned API surface.

- **LevelAsset (`core:level`)**
  - Serializable authored data (JSON-backed content and authoring-time properties).
  - Input to runtime instantiation.

- **LevelInstance (`core:level`)**
  - Runtime binding/mapping layer between LevelAsset-authored nodes and live scene/runtime entities/resources.

- **EditorSceneState (future extraction)**
  - Transient editor-only state (selection, gizmo interaction, editor overlays).
  - Should move out of Scene as a dedicated structure once extraction risk is reduced.

- **RenderSceneExtractor (`core:render_scene_extractor`)**
  - Conversion boundary that copies the already-computed render state from Scene.

- **RenderFramePacket (`core:render_frame_packet`)**
  - Frame-owned renderer-facing extraction of Scene state, grouped into World, Debug, Editor, and animation-runtime-overlay snapshots.
  - Independent of Scene lifetime and mutable Scene container storage after extraction.
  - Resource handles retain their existing ownership and synchronization semantics; cross-thread resource safety remains future work.

## Dependency direction

```text
LevelAsset
    ↓
LevelInstance
    ↓
Scene
    ↓
RenderSceneExtractor
    ↓
RenderFramePacket
    ↓
Renderer
```

## Constraints to preserve

- Keep module dependency direction acyclic:
  - Scene/Level runtime data -> render extraction -> renderer.
- Keep `core:scene` module name stable unless a dedicated migration patch is planned.
- Do not interpret packet ownership as permission to queue resources across threads without a separate handoff and synchronization contract.
