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
