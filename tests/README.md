# Tests: conventions and fixture layout

This document defines the conventions for `tests/` under **CR-23 Test Foundation & Harness Hardening**.

It standardizes the style that already exists in this repository so new tests do not drift into ad-hoc naming, duplicated fake setup, or inconsistent backend assumptions.

## Overview

Current tests are built as one `CoreEngineModuleTests` target with GoogleTest discovery in `tests/CMakeLists.txt`.

The suite is currently unit-focused and grouped by subsystem (`InputTests`, `Math`, `GameplayTests`, etc.).

## Current test layout

Current baseline layout:

- `tests/CMakeLists.txt` — test target and source list.
- `tests/FakeTextureIO.h` — shared fake implementations for texture/resource pipeline tests.
- `tests/unit/AnimationTests/`
- `tests/unit/GameplayTests/`
- `tests/unit/InputTests/`
- `tests/unit/Math/`
- `tests/unit/RenderTests/`
- `tests/unit/ResourceTests/`
- `tests/unit/TimerTests/`

Examples:

- `tests/unit/InputTests/TestInputCore.cpp`
- `tests/unit/Math/TestMathUtils.cpp`
- `tests/unit/ResourceTests/TestTextureStorage.cpp`

## Naming conventions

Use existing naming patterns:

- **File names:** `Test<Subject>.cpp` (for example: `TestInputCore.cpp`, `TestGameplayWorld.cpp`).
- **Test suites:** use `<Subject>` in `TEST`/`TEST_F` first argument (for example: `TEST(InputCore, ...)`, `TEST_F(TextureStorageTest, ...)`).
- **Test case names:** behavior-focused, short, and specific (`PressReleaseEdges`, `DecodeFailureSetsFailedState`, `CatchupTicksAreClamped`).

Guidelines:

- Prefer one source file per subject/module under test.
- Keep names stable and searchable; avoid generic names like `Works`/`BasicTest`.
- If adding backend-specific tests, include backend in the suite or case name (for example `Dx12...`).

## Folder conventions

Follow the existing tree; do not introduce a new top-level test taxonomy unless there is a clear project-wide migration.

- Put new unit tests under `tests/unit/<SubsystemTests>/`.
- Reuse existing subsystem folders when the subject fits.
- Add a new folder under `tests/unit/` only when no existing subsystem folder is a good fit.

Recommended additions for future differentiation:

- Keep backend/integration-oriented tests clearly separated by path and name (for example a dedicated `tests/integration/` tree when such tests are introduced).
- Keep pure unit tests in `tests/unit/`.

## Fixture and fake/helper conventions

Use the smallest scope that avoids duplication.

### Local fixture (inside one test file)

Prefer a local fixture (`namespace { class ... : public ::testing::Test { ... }; }`) when setup is only used by one file.

Current example: `TextureStorageTest` in `tests/unit/ResourceTests/TestTextureStorage.cpp`.

### Shared fake/helper

Promote to shared helper only when at least one of these is true:

- The same fake/setup is reused by multiple files.
- The helper models a stable test seam used across a subsystem.
- Keeping copies would likely diverge behavior.

Current shared example: `tests/FakeTextureIO.h` (`FakeTextureDecoder`, `FakeTextureUploader`, `FakeJobSystem`, `FakeRenderQueue`).

### Placement rules

- `tests/<Name>.h` for cross-subsystem shared fakes/helpers.
- `tests/unit/<Subsystem>/...Helper.h` for subsystem-local helpers reused by multiple files in that subsystem.
- Keep helper APIs minimal and deterministic; avoid hidden global state.

## Deterministic test rules

Tests must be deterministic and backend-independent by default.

- Do not depend on wall-clock timing, random seeds without fixed seed, or OS scheduling order.
- Prefer explicit stepping/draining for async-like flows (pattern already used via `jobSystem.Drain()` and `renderQueue.Drain()`).
- Assert concrete state transitions and values, not incidental side effects.
- Use tight floating-point assertions appropriately (`EXPECT_FLOAT_EQ` / `EXPECT_NEAR` with explicit epsilon).
- Ensure each test resets or owns its state (for fixtures, use `SetUp`/`TearDown` when cleanup is needed).

## Backend assumptions

Project reality: **DX12 is the primary runtime backend**.

Test guidance:

- Most unit tests should remain backend-agnostic and not require real GPU/backend initialization.
- Prefer testing contracts through abstractions and fakes (for example texture I/O interfaces), not backend implementation details.
- Backend-specific or integration tests are allowed when needed, but they must be clearly marked by path/name and should not be mixed with ordinary unit behavior tests.
- If a test requires backend-specific behavior, document that requirement in the test file header comment and name it explicitly.

## Adding a new test checklist

1. Choose target folder (`tests/unit/<SubsystemTests>/...`) based on existing layout.
2. Name file as `Test<Subject>.cpp`.
3. Reuse existing shared helpers/fakes first; add local fixture if scope is single-file.
4. Promote to shared helper only when reused or clearly stable as shared seam.
5. Keep test deterministic (no implicit time/order dependence).
6. Keep backend assumptions explicit; default to backend-agnostic unit coverage.
7. Add the new test source to `tests/CMakeLists.txt`.