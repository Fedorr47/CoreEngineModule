# CoreEngineModule Agent Guide

This file is the entry point for AI coding agents working in this repository.

Keep this file short. Do not turn it into an architecture encyclopedia.
Detailed architecture, refactoring plans, and generated repository maps live under `docs/`.

## Repository

CoreEngineModule is a custom modular real-time engine written in C++23.

Major areas include:

* App / application lifecycle
* Core / ECS
* Scene / Level
* Renderer / RenderGraph / RHI
* Assets
* Animation
* Physics
* Navigation
* Gameplay
* AI / GOAP / Steering
* Editor and debug tooling

The project uses C++ modules extensively.

The primary rendering backend is DirectX 12.
OpenGL is maintained as a secondary backend.

The project is Windows-first and uses CMake, Ninja, and MSVC.

## Read First

Before making non-trivial changes, read only the documentation relevant to the task.

Repository overview:

* `README.md`
* `CLAUDE.md`

Refactoring:

* `docs/refactor/master-plan.md`
* `docs/refactor/debt-register.md`

Repository navigation:

* `docs/refactor/index/repo.md`
* `docs/refactor/index/hotspots.md`
* `docs/refactor/index/module-deps.tsv`
* `docs/refactor/index/test-map.tsv`

Subsystem indexes:

* `docs/refactor/index/subsystems/`

Architecture:

* `docs/architecture/`

Do not recursively read the whole repository before starting a task.

Use the repository index to narrow the search first.

## Navigation Protocol

For refactoring tasks, use this order:

1. Read this `AGENTS.md`.
2. Read the specific task.
3. Read `docs/refactor/master-plan.md` only when architectural context is required.
4. Open the relevant subsystem index under `docs/refactor/index/subsystems/`.
5. Inspect dependency and test maps when useful.
6. Search for the exact symbols involved.
7. Read only the implementation files and tests needed for the task.

Do not dump the complete source tree, complete module graph, large grep results, or full build logs into context.

Redirect large command output to a file and inspect only the relevant sections.

## Refactoring Rules

Prefer small, independently reviewable patches.

Do not combine unrelated cleanup with an architectural change.

Unless explicitly required by the task, do not simultaneously:

* move files,
* redesign public APIs,
* change runtime behavior,
* rename large groups of symbols,
* reformat unrelated code.

Preserve behavior unless the task explicitly requests a behavior change.

Architecture changes should normally follow this sequence:

1. establish or test the new boundary,
2. introduce the replacement path,
3. migrate consumers,
4. remove the old path,
5. add an architectural regression guard where appropriate.

Do not perform repository-wide mechanical modernization merely because newer C++23 syntax exists.

Correctness and ownership clarity take priority over stylistic modernization.

## Architecture Boundaries

Respect existing subsystem ownership.

Important current boundaries include:

* `LevelAsset` represents authored/serialized level data.
* `LevelInstance` represents runtime bindings for a loaded level.
* `Scene` is the authoritative runtime world.
* Renderer-facing state should move toward extracted frame-local render data rather than unrestricted live `Scene` access.
* App should act primarily as composition/lifecycle orchestration rather than accumulating subsystem behavior.
* Generic GOAP infrastructure must remain domain-neutral.
* Domain-specific GOAP observations/actions/decisions belong outside the generic planner/runtime layer.
* Gameplay graph code should orchestrate gameplay rather than absorb domain locomotion or interaction implementation.
* Renderer facade, RenderGraph, RHI, and backend implementations are distinct layers.

Do not solve ownership problems by introducing broad shared locking.

Thread-safety changes require explicit ownership and handoff semantics.

## Frame Ordering

Runtime frame ordering is behavior.

Do not reorder gameplay, animation, scene synchronization, rendering, or event-processing stages as cleanup.

If a refactor touches frame orchestration, identify the existing order before modifying it and verify that the resulting order is equivalent unless the task explicitly requests a change.

## C++ Modules

Most engine code uses C++ modules.

When adding or moving `.cppm` files:

* check their module declaration,
* check imports,
* update the relevant CMake module source list,
* avoid introducing dependency cycles,
* avoid leaking backend-specific implementation through generic module interfaces.

Do not rename existing modules as part of unrelated refactoring.

## Build

Primary development configuration:

`out/build/x64-Debug-DX12`

Typical application build:

`cmake --build out/build/x64-Debug-DX12 --target app`

Use the narrowest useful build target during development.

Do not perform a clean full rebuild for documentation-only or indexing changes unless necessary.

## Tests

Use the narrowest relevant tests first.

For architectural dependency changes, run the architecture checks under:

`tests/architecture/`

Then run affected unit/integration tests.

Before declaring a code task complete, run the relevant validation required by the touched subsystem.

If validation cannot be run, state exactly what was not run and why.

Do not claim that a change builds or passes tests without having run the corresponding command.

## Repository Index

The generated files under:

`docs/refactor/index/`

are navigation aids, not architectural authority.

If generated index data disagrees with source code, source code wins.

Do not manually edit generated index output when it can be regenerated.

Index generation tooling lives under:

`tools/refactor/`

When changing the generator, regenerate the index and verify deterministic output.

## Refactoring Campaign

The repository-wide refactoring roadmap is:

`docs/refactor/master-plan.md`

The master plan is a roadmap, not permission to refactor adjacent systems.

Each Codex task should identify a narrow `Rxx` scope from that roadmap.

Do not start later refactoring stages merely because related code was encountered.

Record newly discovered architectural debt rather than opportunistically fixing unrelated debt.

## Assets and External Code

Do not recursively inspect binary assets unless the task requires them.

Do not modify third-party code under `extern/` as part of normal engine refactoring.

For global indexing, record large/binary asset paths and categories rather than reading their contents.

## Generated and Build Files

Do not treat build output as source.

Avoid indexing or modifying:

* `.git/`
* `out/`
* generated compiler/module artifacts
* IDE caches
* binaries
* imported models
* textures

unless explicitly required.

## Definition of Done

A refactoring task is complete only when:

* the requested scope is implemented,
* behavior outside the requested scope is preserved,
* relevant tests/checks have been run,
* architectural boundaries have not regressed,
* unrelated files have not been modified,
* generated indexes are updated if the repository structure changed,
* documentation is updated when an architectural contract changed,
* remaining risks or unverified behavior are reported explicitly.

Keep the final report concise:

* what changed,
* important design decisions,
* files or subsystems affected,
* validation performed,
* remaining risks.

## Optional Agent Workflows

If structured agent workflows such as Superpowers are available, use the relevant planning, testing, debugging, review, and verification workflows for non-trivial changes.

Repository-specific instructions in this file and the user's explicit task remain authoritative.
