# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

CoreEngine is a custom modular real-time C++23 game engine/renderer, built almost entirely with **C++ modules** (`.ixx` / `.cppm` files under `src/`). It has a custom RHI abstraction with two backends — **DX12** (primary, Windows-only) and **OpenGL** (secondary/alternative) — a hybrid gameplay architecture (EnTT ECS + a level/scene system, not "everything is ECS"), a Jolt Physics integration, and a Recast/Detour navigation layer.

Read `README.md` first for the full subsystem map, data flow, and per-frame update order — it is detailed and current. `tests/README.md` defines test conventions in detail (naming, fixture placement, determinism rules). `docs/architecture/*.md` documents two specific ongoing refactors (Scene/Level/Editor/Render boundaries, and editor/debug UI ViewModel conventions) — read these before touching `Scene`, `LevelInstance`, or editor ImGui panel code.

## Build

This is a Windows-first project using CMake + Ninja + MSVC, requiring **CMake 4.3.0+** (for `import std;` / C++23 module support) and **Visual Studio 2022** (any edition, C++ desktop workload) with a recent MSVC toolset, plus **vcpkg**.

Two version-conflict traps to know about, since fixed paths in docs/scripts break as soon as a machine differs from the one that wrote them:

- **The CMake bundled with the VS installer/IDE component is usually older than 4.3.0** and is pinned to that VS release — configure will fail (or silently lose `import std;` support) if that's the `cmake` you're invoking. Install/use a standalone CMake 4.3.0+ (installer, winget, or `pip install cmake`) and confirm with `cmake --version` before configuring.
- **If more than one Visual Studio version is installed side by side** (e.g. VS2022 *and* VS2026), a generic Developer shell, `vswhere`, or whatever `cl.exe` happens to be first on `PATH` can resolve to the wrong toolset — this project is validated against the VS2022 (MSVC 143) toolset specifically. Launch the **VS2022-specific** Developer shell ("x64 Native Tools Command Prompt for VS 2022" / "Developer PowerShell for VS 2022", or run VS2022's `VsDevCmd.bat` directly) rather than a generic one, and if ambiguity remains, pin the toolset explicitly with `-T version=14.4` (or the exact toolset in use) on the configure line.

The Ninja generator needs `cl.exe`/`link.exe` on `PATH` for both configure *and* every subsequent build invocation (not just the first) — if you open a plain shell instead of the VS2022 Developer shell, reconfigure may succeed but the compile step will fail with "cl.exe not found"-style errors. Don't hardcode absolute CMake/VS/vcpkg install paths in scripts or docs; they differ per machine and per how many VS versions are installed.

**If `VCPKG_ROOT` isn't set and you don't know where vcpkg lives on this machine**, don't guess a path — read it back out of a previously-configured tree instead, since `Z_VCPKG_ROOT_DIR` in `CMakeCache.txt` records exactly which vcpkg that tree was configured against:
```powershell
Select-String -Path out\build\x64-Debug-DX12\CMakeCache.txt -Pattern 'Z_VCPKG_ROOT_DIR'
```
If no build tree exists yet, a filesystem search for `vcpkg.exe` is the fallback — there's no fixed convention for where it's installed per-machine.

**Every tool invocation in an agentic session (each `PowerShell`/`Bash` tool call, in Claude Code or similar) is a fresh process** — env vars set by running `VsDevCmd.bat` (or setting `$env:VCPKG_ROOT`) in one call do **not** persist to the next call. Re-sourcing `VsDevCmd.bat` on every single build/configure invocation works but is slow (it re-resolves VS state each time). Faster: capture the dev-shell environment once to a file, then re-apply it from that file at the start of each subsequent command in the same session:
```powershell
# once per session:
cmd.exe /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && set' | Out-File -Encoding utf8 vsenv.txt
# prepended to every subsequent command in the session:
Get-Content vsenv.txt | ForEach-Object { if ($_ -match '^([^=]+)=(.*)$') { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') } }
$env:VCPKG_ROOT = '<path from Z_VCPKG_ROOT_DIR above>'
```

### Before running a build: sanity-check the environment

A full build is ~760 module compile steps and can take several minutes, so it's worth spending 5 seconds confirming the toolchain first instead of discovering a bad environment after a long build fails at the end:

```powershell
cmake --version                 # must be 4.3.0+ — if it's older, the wrong `cmake` is on PATH (likely the one bundled with VS)
where.exe cl.exe                # must resolve to a VS2022 (14.4x) path — if empty or a different VS version, wrong/no Developer shell
echo $env:VCPKG_ROOT             # should be set, or pass -DCMAKE_TOOLCHAIN_FILE explicitly
Test-Path out\build\x64-Debug-DX12\CMakeCache.txt   # if true, the tree is already configured — skip straight to --build
```

If `cmake --version` or `where.exe cl.exe` look wrong, fix the shell/PATH before configuring — don't try to work around it with absolute paths, since that just reproduces the machine-specific-path problem this doc used to have.

Configure when the build tree is new or CMake cache/options changed:

```powershell
cmake -S . -B out\build\x64-Debug-DX12 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCORE_RENDER_BACKEND=DX12
```

If `VCPKG_ROOT` isn't set, add `-DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"` (or the equivalent path on your machine).

Build the app target from the configured tree:

```powershell
cmake --build out\build\x64-Debug-DX12 --target app
```

Clean built outputs while keeping the configured CMake cache:

```powershell
cmake --build out\build\x64-Debug-DX12 --target clean
```

Rebuild without reconfiguring:

```powershell
cmake --build out\build\x64-Debug-DX12 --target clean
cmake --build out\build\x64-Debug-DX12 --target app
```

Key CMake options (all cache variables):
- `CORE_RENDER_BACKEND` — `GL` or `DX12` (default `DX12`). DX12 is Windows-only and is the actively developed backend; GL is the alternative/fallback path.
- `WITH_TESTS` (default `ON`) — builds the `tests/` tree.
- `USE_SUBMODULES` (default `ON`) — prefers `extern/*` git submodules (e.g. `extern/googletest`, `extern/glfw`, `extern/glm`, `extern/imgui`) over `FetchContent` when present. `extern/imgui` must be the **docking** branch/tag (`v1.91.0-docking`) or it's rejected at configure time.
- `ENABLE_IMPORT_STD` (default `ON`) — uses experimental CMake support for `import std;`. Configure fails loudly if the toolchain doesn't expose `CMAKE_CXX_COMPILER_IMPORT_STD`.
- `CORE_TEST_SANITIZER_PROFILE` / `CORE_ENABLE_ASAN` / `CORE_ENABLE_UBSAN` (default `OFF`) — sanitizer-oriented test builds. On MSVC only ASan is supported; UBSan is force-disabled with a warning. Requires `WITH_TESTS=ON`. See `tests/README.md` for the known-good Clang/GCC+Ninja invocation.

Two-config presets exist for Rider/VS in `CMakeSettings.json` (`x64-Debug-GL`, `x64-Debug-DX12`), both Ninja + `msvc_x64_x64`, with build roots under `out\build\<name>` and install roots under `out\install\<name>`.

Dependencies (EnTT, Jolt Physics, RecastNavigation, STB, Assimp, and — for GL — GLFW/GLM/GLEW/OpenGL; for DX12 — ImGui docking + d3d12/dxgi/dxguid) are pulled via `FetchContent` unless the equivalent `extern/<name>` submodule exists and `USE_SUBMODULES=ON`. The DX12 backend links `d3d12`, `dxgi`, `dxguid`, `comdlg32`, and requires `d3dcompiler_47`/`d3dcompiler` to be discoverable.

The `app` executable's post-build step copies `assets/` next to the built binary — asset paths are resolved relative to the executable.

### Faster iteration / checking the build

- **Don't `--target clean` unless the task actually needs a clean build** (verifying a from-scratch build works, or ruling out stale-incremental-state as a bug cause). A no-op incremental `cmake --build ... --target app` after no source changes finishes in seconds; a full clean rebuild takes several minutes because every `.cppm`/`.ixx` is a separate compile step with module-scan ordering. Default to a plain incremental build.
- **Only reconfigure (`cmake -S . -B ...`) when `CMakeLists.txt`, cache options, or the toolchain changed.** An existing `CMakeCache.txt` in the target `out\build\<preset>` dir means `cmake --build` alone is enough.
- **Run configure/build as a background command** and check the tail of its output rather than waiting on/streaming the full log — a clean build easily produces 700+ lines of `[n/761] Building CXX object ...` noise that isn't worth holding in context.
- **To confirm success or find the actual failure fast**, don't read the whole log — check the process exit code first, then if it's nonzero, search the output for the actual error lines instead of scanning line by line:
  ```powershell
  Select-String -Path <output-file> -Pattern 'error C\d|LNK\d|FAILED:' 
  ```
  A clean build's only expected noise is upstream deprecation/experimental CMake warnings (RecastNavigation's old `cmake_minimum_required`, CMake's `import std;` experimental-support warning) and occasional `[[nodiscard]]`-discarded-value warnings (`C4834`) — neither indicates a real problem.
- `--target app` builds only the engine lib + executable. Building the default target (no `--target`, or `--target ALL`) also builds every test executable in `tests/` (`WITH_TESTS=ON` by default) — much slower; only do this when tests are actually in scope.

## Tests

Tests use GoogleTest, built as several executables in `tests/`, discovered via `gtest_discover_tests` (CTest). Run from the build directory:

```powershell
ctest --test-dir out\build\x64-Debug-DX12 --output-on-failure
```

Run a single test or subset by name (GoogleTest filter syntax works through CTest's `-R` regex on discovered test names, or directly via the test binary):

```powershell
ctest --test-dir out\build\x64-Debug-DX12 -R TestGameplayGraph --output-on-failure
out\build\x64-Debug-DX12\tests\CoreEngineModuleTests.exe --gtest_filter=GameplayGraph.SomeCase
```

Test executables and what they cover:
- `CoreEngineModuleTests` — the main unit test suite (`tests/unit/**`), backend-agnostic by default.
- `CoreEnginePhysicsLevelSmokeTests`, `CoreEngineCharacterPhysicsSmokeTests`, `CoreEngineAIPhysicsSmokeTests` — Jolt-backed integration/smoke tests (`tests/integration/**`), labeled `physics;smoke;integration` (character/ai variants add their own labels).
- `CoreEngineModuleDx12SmokeTests` — only registered when `WIN32 AND CORE_RENDER_BACKEND STREQUAL DX12`; labeled `dx12;smoke`.
- `CoreEngineJoltSmokeTests` — Jolt dependency smoke test, labeled `jolt;smoke`.

Filter by label instead of name when you want a category, e.g. `ctest --test-dir out\build\x64-Debug-DX12 -L smoke` or `-L "physics;smoke"`.

Follow `tests/README.md` conventions when adding tests: file per subject as `Test<Subject>.cpp` under `tests/unit/<SubsystemTests>/`, `TEST(<Subject>, ...)` suite naming, deterministic execution (no wall-clock/random dependence, explicit `Drain()` stepping for async-like flows via `FakeJobSystem`/`FakeRenderQueue` in `tests/FakeTextureIO.h`), and remember to add new sources to `tests/CMakeLists.txt` — there is no glob.

## Architecture notes beyond the README

- **Module boundary conventions are actively enforced by ongoing refactors**, not just aspirational: `Scene` (`src/Scene/Scene.cppm`, module `core:scene`) is the authoritative runtime world; `LevelAsset`/`LevelInstance` (`src/Level/`) are the serializable-data ↔ runtime-binding split; editor-only state is being extracted out of `Scene` into `EditorSceneState`. Keep the dependency direction acyclic: Scene/Level runtime data → render extraction → renderer. See `docs/architecture/scene-level-editor-render-boundaries.md`.
- **Editor/debug ImGui panels follow a Model/ViewModel/View/Command/Service split** (not a formal framework, no base classes required): ViewModels hold only per-frame snapshot data, persistent panel state, and pending-edit state — never owning references to `Scene`, `LevelInstance`, renderer/RHI objects, or asset runtime objects. Mutations flow through commands/services, not direct ImGui-widget-to-engine-state writes. See `docs/architecture/editor-ui-architecture.md` before adding or migrating a panel (it also defines the `TransformInspectorViewModel` contract as a worked example).
- **Physics** (`src/Physics/Jolt/*`, `src/Physics/LevelPhysicsRuntime.*`) wraps Jolt Physics 5.6 with single precision, default allocator, exceptions/RTTI disabled, and no debug renderer/profiler compiled in (see the `JPH_*`/`ENABLE_*` cache forces in `CMakeLists.txt` if you need to change Jolt build config).
- **Navigation** (`src/Navigation/`, `App/NavigationDebugDraw.cppm`, `App/LevelNavigation.cppm`) wraps RecastNavigation v1.6.0 (Recast/Detour/DebugUtils targets), pinned via `CMAKE_POLICY_VERSION_MINIMUM 3.5` since upstream declares an old CMake minimum.
- **Gameplay AI** (`src/Gameplay/AI/*`) implements a GOAP-style planner/executor stack (goal selection, planner, plan execution, action binding/runtime/task, decision contracts/runtime) plus steering/route-following (`GameplayRoute*`) and traversal (`src/Gameplay/Traversal/*`, e.g. door/jump executors) — this is one of the most recently active areas (see recent commit history for "DevelopmentScenario" migrations touching these).
- **`src/App/Development/DevelopmentScenario.*`** is the current mechanism for standing up named runtime/editor test scenarios in `app` (as opposed to ad hoc scene setup) — check here when a scenario-driven manual test needs updating alongside a subsystem change.
- The renderer is a strict layered stack: `App` (orchestration only) → `Renderer` facade (`src/Render/Renderer.cppm`, backend-agnostic public entry point) → `RHI` (`src/Render/RHI.cppm`, the contract) → concrete backend (`src/Render/DirectX12/*` or `src/Render/OpenGL/*`). `RenderGraph` (`src/Render/RenderGraph.cppm`) organizes per-frame resources/passes on top of this. Module lists for each backend are assembled explicitly in `CMakeLists.txt` (`COMMON_MODULES`, `GL_MODULES`, `DX12_MODULES`) — adding a new `.cppm` requires adding it to the relevant list there, not just creating the file.
- Per-frame gameplay order matters and is easy to get wrong: `GameplayRuntime::BeginFrame()` → `PreAnimationUpdate()` (input → commands → combat/interaction → gameplay graph → movement/locomotion → scene sync → push animation params) → `Scene::UpdateSkinned(deltaSeconds)` → `PostAnimationUpdate()` (consume animation notifies back into gameplay). See README §2.12 and §4 for the full sequence.
