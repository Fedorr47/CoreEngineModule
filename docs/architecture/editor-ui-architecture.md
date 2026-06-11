# Editor / debug UI architecture

_Date: 2026-06-01_

This note defines the intended organization for CoreEngine editor/debug UI code. It is a practical review guide for growing ImGui panels, scene inspection, selection tools, renderer settings, and property editing without making editor UI concepts part of the engine runtime architecture.

## Purpose

- Keep editor/debug UI code readable as more panels and tools are added.
- Separate ImGui drawing from user intent and engine mutations.
- Keep ownership of runtime objects in the existing engine systems.
- Prevent editor UI dependencies from leaking into RHI, renderer backend, gameplay, asset runtime, platform, or other core systems.

## Scope and non-goals

This architecture applies only to editor/debug UI organization: current ImGui panels, future editor panels, debug windows, selection tooling, property inspectors, and editor-only commands.

It does not require a broad source move. Existing files may migrate incrementally when touched for feature work.

Non-goals:

- no new UI framework requirement;
- no mandatory ViewModel or Command base class;
- no runtime behavior change;
- no ownership transfer of `Scene`, `LevelInstance`, renderer, RHI, asset, gameplay, or platform objects into editor UI code;
- no application of MVVM terminology to low-level engine modules.

## Architecture roles

- **Model**: editor-facing representation of state or references to engine state. A model may identify selected scene nodes, editable level data, renderer settings, or asset records, but it does not automatically own those runtime objects.
- **ViewModel**: UI-ready projection of state for a panel or tool. It owns editor-only state such as selection presentation, filters, expanded rows, panel state, text buffers, formatted values, pending edits, and validation state. ViewModels may read or project engine state through narrow services, but must not become owners of engine runtime objects.
- **View**: thin ImGui/UI rendering code. Views draw controls, tables, menus, popups, and widgets from ViewModel data. Views should delegate user intent instead of directly mutating deep engine state when a Command boundary is appropriate.
- **Command**: explicit user action boundary. Commands represent actions such as selecting an object, changing a property, importing an asset, toggling a renderer setting, or starting a tool operation. Commands perform mutations by calling existing editor/runtime services through narrow interfaces.
- **Service**: narrow bridge from editor UI to engine systems. Services may wrap access to scene selection, level editing, renderer settings, asset operations, picking, and future undo/redo. Services should keep engine-specific calls out of Views and keep editor-only concepts out of runtime modules.

## ViewModel refresh and caching conventions

Editor ViewModels are presentation caches for editor/debug UI panels. Use them to copy, filter, sort, format, and validate state before ImGui draws so the View remains a thin renderer of already-prepared data. These ViewModel snapshots are only for editor UI presentation. They are not render snapshots, not thread-safety boundaries, and not replacements for future `RenderFramePacket`, scene snapshot, or runtime/render thread split work.

A ViewModel may be a small struct, helper object, or panel-owned cache. There is no required ViewModel base class or framework. Existing panels may continue to read and mutate live state directly until related work justifies a local migration.

### Lifetime categories

Keep three kinds of data separate inside editor/debug UI panels:

1. **Per-frame UI snapshot data** is rebuilt or refreshed before drawing a panel. It is a copied/projected view of runtime/editor state for the current UI frame. It may contain row records, display names, formatted labels, sorted/filterable lists, enabled/disabled flags, validation messages, current numeric values copied into UI units, lightweight IDs, handles, indices, and names needed to draw controls. It should be cheap to discard and safe to become stale after the frame.
2. **Persistent panel state** lives with the panel or tool across frames. It may contain filter strings, search text, selected row IDs, expanded tree state, focused/scroll target IDs, column sort choices, user-visible display preferences, cached lightweight handles, and other editor-only UI memory. It must not become the authoritative owner of engine runtime objects.
3. **Pending edit state** represents user input that has not yet been applied. It may contain text buffers, temporary slider values, dirty flags, parse/validation errors, original values for cancel/revert UI, and the target lightweight ID or handle. Pending edits should be committed by emitting user intent to commands/services or by calling the existing narrow editor/runtime API at the intended mutation phase.

### Refresh, draw, and apply order

Future ViewModel-style panels should use this order unless a local panel has a documented reason to do otherwise:

1. **Read/project before drawing**: query the relevant service or existing editor/runtime API and copy only the UI-facing facts the panel needs into per-frame snapshot data. Apply formatting, filtering, sorting, presentation grouping, and validation here rather than inside individual ImGui widgets.
2. **Draw from copied/projected data**: let ImGui Views consume the snapshot and persistent panel state. Drawing code should not rely on long-lived references into `Scene`, `LevelInstance`, renderer, RHI, asset, gameplay, or backend objects.
3. **Collect or emit user intent during UI**: translate clicks, drags, text commits, and menu selections into explicit intent such as select row, rename item, set transform value, toggle renderer option, focus camera, or import asset. Keep incomplete text entry in pending edit state until the user commits or cancels it.
4. **Apply mutations through the intended boundary**: execute editor commands/services when available, or the existing narrow editor/runtime API where no command/service exists yet. Apply at the panel's existing behavior-preserving phase; this convention does not introduce a centralized command queue, undo/redo system, async dispatcher, or thread-safe submission model.

For example, a hierarchy panel can rebuild a vector of row snapshots from the current scene, draw tree rows from those copied labels and IDs, record a selected row intent when the user clicks, and then route selection through the editor selection command/service. A property panel can keep an uncommitted text buffer plus validation error in pending state, then apply the parsed value through the property editing service or existing setter only when the user commits.

### Ownership and handle rules

ViewModels may store lightweight identifiers and presentation values:

- scene node IDs, entity IDs, asset IDs, resource handles, stable row keys, indices valid for the current refresh, and other non-owning handles;
- copied names, display labels, paths, type names, formatted values, filter strings, validation strings, and tooltip text;
- expanded tree state, selected row IDs, focused item IDs, sort/filter settings, column visibility, and other editor-only panel preferences;
- pending edit buffers, parsed candidate values, dirty flags, validation status, and the lightweight target ID for the edit.

ViewModels must not own or extend the lifetime of engine/backend resources, including:

- `Scene`, `LevelInstance`, authoritative ECS registries, gameplay worlds, gameplay runtime objects, or gameplay components;
- renderer instances, renderer backend objects, RenderGraph objects, RHI devices/resources, command lists, descriptors, GPU buffers, textures, or synchronization primitives;
- `AssetManager` resources, imported asset runtime objects, resource manager ownership, streaming state, or file watcher backends;
- platform/windowing handles, input backends, swapchains, or OS resources.

If a panel needs to reach one of these systems, keep the owning object in the existing engine layer and access it through a service, command, short-lived function parameter, or existing narrow API. Do not hide ownership in `shared_ptr`, raw pointer, reference member, or cached backend object fields inside a ViewModel.

### Relationship to render snapshots and thread split work

Editor ViewModel snapshots are allowed to be stale UI presentation data. They do not define render visibility, render resource lifetime, synchronization, frame pacing, or cross-thread ownership. Future scene snapshots, render extraction data, or `RenderFramePacket` work must define their own lifetime, ownership, and synchronization rules independently of these editor UI conventions.

### ViewModel checklist for future panels

Before adding or migrating a ViewModel-style panel, verify that:

- per-frame snapshot data is rebuilt/refreshed before the View draws;
- persistent panel state is editor-only and separate from copied runtime facts;
- pending edits are represented explicitly until commit/cancel;
- user intent is applied through commands/services or an existing narrow API at the intended phase;
- stored references are lightweight IDs/handles/values, not owning engine/runtime/backend objects;
- the document and code comments do not imply thread safety, render snapshot semantics, a command queue, undo/redo, or a mandatory ViewModel framework.

## Transform Inspector ViewModel contract

The Scene Inspector transform section should use a small UI-facing snapshot named `TransformInspectorViewModel` when it is migrated to ViewModel-style drawing. This contract is documentation-first and intentionally behavior-preserving: the current ImGui node inspector may continue to read and mutate the selected node transform directly until a follow-up task projects the values and routes edits through an explicit apply path.

The ViewModel is an editor/debug UI presentation snapshot only. It must not be consumed by renderer, RHI, RenderGraph, gameplay runtime, scene ownership, asset runtime, or threaded render snapshot code. It must not store raw `Scene`, `LevelInstance`, `LevelNode`, transform, or runtime pointers/references.

### Copied display data

`TransformInspectorViewModel` contains the fields the transform section needs to draw the currently selected scene node:

- `selectedSceneNodeId`: lightweight current-level node index for the selected scene node; `-1` means no selected node.
- `displayName`: copied node name or UI fallback such as an unnamed-node label.
- `position`: copied position value from the selected node transform.
- `rotationDegrees`: copied rotation value from the selected node transform, in the same degrees convention used by the existing inspector.
- `scale`: copied scale value from the selected node transform.
- `hasSelection`: true when the selected scene node id resolves to a live editable node for the current refresh.
- `canEdit`: true when the selected node resolves to an editable level node and the current editor mode allows transform editing.
- `isDirty`: true when `LevelEditorUIState::transformInspectorPendingEdit` contains unapplied local edits for the same selected scene node.
- `validationWarning`: copied warning text to display near the transform controls, normally sourced from pending edit validation.

These fields are copied/projected for UI drawing. They are allowed to become stale after the editor frame and should be rebuilt from the selected node and local pending edit state before the panel draws again.

### Pending local edit state

Pending transform edits live in `LevelEditorUIState::transformInspectorPendingEdit` as `TransformInspectorPendingEditState`. This keeps temporary input, dirty state, and validation warnings owned by editor/debug UI state instead of the read-only ViewModel or runtime model.

The pending edit state owns:

- `targetSceneNodeId`: lightweight current-level node index that the pending values apply to.
- `position`, `rotationDegrees`, and `scale`: local candidate values entered by the UI before they are applied.
- `isDirty`: whether the pending values differ from the copied selected node values and still need commit/cancel/apply handling.
- `validationWarning`: local validation text such as an invalid scale warning.

Follow-up migration work should copy selected node data into `TransformInspectorViewModel`, draw the section from that snapshot plus `LevelEditorUIState::transformInspectorPendingEdit`, and then apply committed edits through the existing narrow `LevelInstance`/scene editing path. This contract does not introduce a command queue, undo/redo, scene hierarchy redesign, thread-safety boundary, or render snapshot semantics.

## Animation Runtime panel ViewModel contract

The Animation Runtime debug panel is planned to migrate to ViewModel-style rendering after the current documentation-first contract is in place. This section defines the UI-facing data contract only. It does not implement the full Animation Runtime ViewModel, change animation runtime behavior, refactor animation systems, add a command queue, or create a thread-safe runtime/render snapshot.

The current ImGui panel still reads live `LevelAsset`, `LevelInstance`, `Scene`, `SkinnedDrawItem`, `AnimatorState`, and `AnimationControllerRuntime` data during the editor/debug UI phase. That is acceptable as transitional behavior while the application is single-thread sequenced, but the migrated panel should project the facts it needs into copied UI data before drawing and should keep panel state and pending edits separate from those copied facts.

### Current sources to project

When the panel is migrated, the projection step may read from the existing narrow call path that locates the observed skinned target and animation runtime state. Relevant current sources include:

- the observed or selected level node index/name and skinned mesh identifier from `LevelAsset`;
- the node-to-skinned-draw mapping from `LevelInstance`;
- the live skinned draw item, animator, and controller runtime data owned by `Scene`/runtime systems;
- `Scene::animationRuntimeDebug.samples` when a panel wants row-oriented runtime debug samples instead of, or in addition to, the direct selected-target view;
- controller asset metadata needed only to produce copied UI rows or lightweight identifiers.

These sources remain owned by their existing systems. The ViewModel may copy values out of them for one UI refresh, but it must not retain owning references or mutable access to them.

### Data buckets

Keep Animation Runtime panel data in the same three buckets used by the general ViewModel convention:

1. **Per-frame UI-facing snapshot data** is rebuilt before drawing the Animation Runtime panel. It should contain copied/projected display facts for the currently observed target and any table rows shown by the panel. Examples include selected or observed animated entity/node ID, node name, skinned mesh ID, animation controller/graph name, controller asset ID, current state name, requested/previous state names, active clip names, playback time, normalized time, blend weights, blend parameter names/values, transition state text, transition alpha, playback speed, looping/paused/autoplay/force-bind-pose/debug flags, sample/debug rows, recent notify rows, routed gameplay event rows, transition diagnostic rows, warning/error text, and lightweight asset or node identifiers.
2. **Persistent panel UI state** lives across frames with the panel/tool. Examples include whether the panel is open, the pinned observed node ID/index, target selection mode, selected row key, filter/search text, collapsed/expanded sections, table sort options, graph/blend preview pan and zoom, scroll/focus requests, and display preferences. This state is editor-only memory and is not authoritative runtime animation state.
3. **Pending UI edits / user intent** represents incomplete or just-emitted user actions. Examples include a requested target change, select-in-editor intent, open-graph intent, pending state request, pending playback speed text/slider value, pending debug flag toggle, validation text for a typed value, dirty flags, and the lightweight target ID the edit applies to. Pending edits remain separate from read-only display facts and should be committed through explicit editor/runtime command paths or existing narrow APIs when those commands/APIs exist.

Read-only display data should be copied into the per-frame snapshot and consumed by drawing code without mutating runtime objects. Persistent UI state may influence what the snapshot projects, such as the pinned target or filters. Pending edits may be produced while drawing, but applying them is a separate step and must not be hidden inside formatting, filtering, row-building, or low-level ImGui widget helpers.

### Allowed copied fields

Animation Runtime ViewModel structs may contain copied values and UI-ready rows such as:

- lightweight entity IDs, level node IDs/indices, asset IDs, controller IDs, graph/node/state IDs, row keys, and handles that do not own runtime objects;
- copied names for the observed node, animation controller/graph, current state, requested state, previous state, active clips, blend parameters, notifies, gameplay events, and diagnostics;
- numeric presentation values such as playback time, normalized time, playback speed, blend weights, transition alpha, sample counters, sequence numbers, and current parameter values;
- enum or simple mode states such as controller mode, clip/blend1D/blend2D mode, transition active/inactive, enabled flags, looping, paused, autoplay, controlled, and debug toggles;
- preformatted labels, warning/error strings, tooltip text, validation messages, table rows, sample/debug rows, and display colors or severity categories when those are purely UI presentation values.

### Data that must stay out of the ViewModel

Animation Runtime ViewModels must not store or own:

- owning pointers/references to `Scene`, `LevelInstance`, `LevelAsset`, ECS registries, gameplay worlds, or scene/runtime containers;
- mutable `SkinnedDrawItem`, `AnimatorState`, `AnimationControllerRuntime`, animation controller assets, skeletons, clips, or other live animation runtime objects;
- renderer, RHI, RenderGraph, swapchain, command-list, descriptor, GPU resource, or renderer backend objects;
- `AssetManager` ownership, asset runtime ownership, streaming state, file watchers, or live containers whose lifetime is controlled by runtime/asset systems;
- `const` pointers/references to live runtime state as a claimed safety boundary. A `const AnimationControllerRuntime*` is still a live runtime reference, not a UI snapshot, synchronization primitive, or lifetime guarantee.

Short-lived function parameters may still be used during projection while the current code remains in `src/Render/ImGui/`, but the stored ViewModel data that drawing consumes should be copied values, lightweight IDs/handles, enum states, counters, and UI-ready display rows.

### Lifetime and refresh rules

An Animation Runtime per-frame snapshot is valid only for the UI refresh/draw that built it. It may become stale immediately after the frame and must not be used as authoritative animation state by runtime, gameplay, renderer, asset, or RHI code. Rebuild or refresh it before each panel draw from the current selected/pinned target and current runtime debug samples.

The contract is intentionally not a thread-safety boundary. It does not make live runtime reads safe across threads, does not define ownership transfer between runtime and render code, and does not replace future render frame packets, scene snapshots, render extraction data, or animation/runtime debug packet work. Future thread-ownership or render-snapshot work must define its own synchronization and lifetime rules independently.

### Future panel migration note

A future Animation Runtime panel migration should follow this sequence:

1. read/project the observed target and animation runtime facts into an Animation Runtime panel ViewModel before drawing;
2. draw target selection, live-state text, active-clip tables, parameter tables, blend previews, recent notify rows, diagnostics, and gameplay event rows from the copied ViewModel data plus persistent panel UI state;
3. emit explicit user intent or pending edits for target pinning, select-in-editor, open-graph, requested state changes, playback/debug toggles, or editable numeric values;
4. apply mutations through explicit editor/runtime command paths when available, or through the existing narrow behavior-preserving API at the intended phase until a command exists.

This migration should stay local to the panel. It should not introduce a central ViewModel framework, centralized command queue, undo/redo system, animation runtime ownership changes, renderer/RHI dependencies, or broad editor UI architecture rewrite.

## Minimal editor command boundary

The current command boundary is intentionally minimal. A command may be a named free function, helper object, or local call path that captures an explicit editor/debug UI intent and performs the mutation synchronously through an editor service or existing narrow runtime/editor API. It is not a centralized command queue or framework.

Conventions:

- Name commands after user intent, such as `SelectSceneNode`, `ClearEditorSelection`, `SetObservedRuntimeEntity`, `SetTransform`, or `FocusCameraOnSelection`.
- Keep command execution synchronous and behavior-preserving until undo/redo, deferred edit application, command recording, or thread-split dispatch is required.
- Delegate selection writes to `EditorSelectionService` when the selection service is available, then mirror to legacy `Scene` selection state only as transitional compatibility.
- Do not introduce renderer/RHI/RenderGraph dependencies on editor commands.
- Do not add a shared command base class, queue, undo stack, async dispatcher, recording/replay layer, or thread-safe submission model for simple panel actions.

The first concrete integration point is scene-node selection from the Level Editor hierarchy: the ImGui row click delegates to `rendern::editor_commands::SelectSceneNode` or `rendern::editor_commands::ToggleSceneNodeSelection`, and those commands route through `EditorSelectionService` before updating the existing `Scene` selection mirrors.
## Dependency direction

Preferred dependency flow:

```text
Views -> ViewModels -> Commands -> Services -> Engine/runtime systems
                     \-> Models ----/
```

Rules:

- Editor UI may observe engine state and request changes through stable interfaces.
- Engine/runtime systems own runtime state; editor ViewModels cache only UI/editor state or lightweight references/IDs.
- Renderer backend, RHI, RenderGraph, gameplay, asset runtime, and platform/windowing code must not depend on editor Views or ViewModels.
- Services are the boundary for mutations that touch scene, level, renderer settings, assets, selection, or future undo/redo.
- Rendering ImGui draw data to the swapchain is an integration concern, not permission for renderer backend code to know about editor panel state.

## Folder layout

Intended layout for new or migrated editor/debug UI code:

```text
src/Editor/
  Model/        # editor-facing IDs, handles, and lightweight state projections
  ViewModels/   # panel/tool state, filters, formatting, validation, selected item projections
  Views/        # thin ImGui windows, panels, menus, and inspectors
  Commands/     # user-intent actions that call services to mutate engine/editor state
  Services/     # narrow bridges to Scene, Level, Assets, renderer settings, picking, undo/redo later
  Widgets/      # reusable editor-only ImGui widgets and small UI helpers
```

Notes:

- This is a proposed target structure. The current ImGui/debug UI files can remain where they are until related work justifies moving them.
- `src/Render/ImGui/` may continue to host existing debug UI during migration, but new editor-facing code should prefer the `src/Editor/` structure when practical.
- Renderer-owned debug drawing and text rendering remain renderer/debug functionality, not editor MVVM code.

## What MVVM does not apply to

MVVM is an editor/debug UI organization pattern only. It must not be applied as a global CoreEngine architecture rule.

Do not introduce Model/ViewModel/View/Command layering into:

- RHI and graphics device abstractions;
- RenderGraph resource/pass scheduling;
- DirectX 12, OpenGL, or other renderer backend implementation;
- renderer frame execution, GPU resource lifetime, descriptor, command list, or synchronization code;
- gameplay core and runtime gameplay systems;
- asset/runtime loading, resource management, import, or streaming systems;
- low-level platform, windowing, input capture, or swapchain plumbing;
- core math, JSON, string, hash, and IO utility modules.

Those systems should keep their existing backend/resource/pass/runtime abstractions. Editor UI can inspect or request changes through services, but it must not invert ownership or make runtime code depend on editor UI concepts.

## Review checklist

When reviewing editor/debug UI changes, check that:

- Views remain mostly ImGui drawing and small UI composition.
- ViewModels hold editor-only panel state and projections, not authoritative runtime ownership.
- Commands capture user intent before mutating scene, level, asset, renderer, or tool state.
- Services provide narrow, testable boundaries to engine systems.
- RHI, RenderGraph, renderer backend, gameplay, asset runtime, platform, and other core modules do not include or depend on editor ViewModels or Views.
- New folders are introduced incrementally and only when code moves or new code needs them.
