# Scene / Level / Editor / Render boundaries (first-step refactor)

_Date: 2026-05-10_

This note records the intended ownership split and the **safe first step** taken in this patch.

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

- **RenderScene / RenderFrameView (future extraction)**
  - Renderer-facing extracted view of scene data.
  - Renderer should consume this view rather than the full Scene object.

## What changed in this patch

- Introduced top-level `src/Scene/` physical area.
- Moved `core:scene` module implementation from `src/Render/Scene/Scene.cppm` to `src/Scene/Scene.cppm`.
- Kept the exported module name (`core:scene`) and public API stable to avoid behavior and dependency churn.
- Updated build and docs references accordingly.

## Why this is intentionally limited

This patch is boundary preparation only. It avoids renderer/gameplay semantic rewrites so runtime behavior remains unchanged.

## Follow-up sequence

1. Extract editor-only state into `EditorSceneState`.
2. Move `LevelAsset` / `LevelInstance` / `LevelECS` into a top-level `src/Level/` area.
3. Introduce `RenderScene` / `RenderFrameView` extraction path.
4. Make `Renderer::RenderFrame` consume `RenderFrameView` instead of full `Scene`.
5. Stop mutating `LevelAsset` directly from gameplay runtime; use runtime-owned scene/world state.

## Temporary constraints to preserve during follow-ups

- Keep module dependency direction acyclic:
  - Scene/Level runtime data -> render extraction -> renderer.
- Keep `core:scene` module name stable unless a dedicated migration patch is planned.
- Prefer behavior-preserving moves before semantic ownership rewrites.
