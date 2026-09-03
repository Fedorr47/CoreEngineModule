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

## Sanitizer-oriented deterministic test profile

The build now supports an optional sanitizer-oriented test profile that is **off by default** and does not change normal developer workflows unless explicitly enabled.

### CMake options

- `CORE_TEST_SANITIZER_PROFILE` (default `OFF`): convenience profile for sanitizer test builds.
- `CORE_ENABLE_ASAN` (default `OFF`): enables AddressSanitizer on supported toolchains.
- `CORE_ENABLE_UBSAN` (default `OFF`): enables UndefinedBehaviorSanitizer on supported toolchains.

Profile behavior:

- `CORE_TEST_SANITIZER_PROFILE=ON` enables `CORE_ENABLE_ASAN=ON`.
- On Clang/GCC, it also enables `CORE_ENABLE_UBSAN=ON`.
- On MSVC-family toolchains, UBSan is explicitly disabled (ASan-only profile).

If sanitizers are requested with `WITH_TESTS=OFF`, configure fails with a clear error because the profile is test-oriented.

### Toolchain support and explicit fallback behavior

- **Clang/GCC**: supports ASan and UBSan profile flags.
- **MSVC / clang-cl (MSVC frontend)**:
    - ASan is supported via `/fsanitize=address`.
    - UBSan is not supported by this profile; the build emits a warning and disables UBSan.
- **Other compilers**: build emits an explicit warning and disables sanitizer instrumentation.

### Determinism notes

- This profile does not add randomness or test-order changes.
- Existing deterministic rules still apply (fixed fixtures, explicit drains, no wall-clock assumptions).
- For single-config generators (for example Ninja), pass an explicit build type (`Debug` or `RelWithDebInfo`) for repeatable local/CI invocation.

### Known-good local run path (Clang/GCC + Ninja)

```bash
cmake -S . -B build-asan-ubsan -G Ninja \
  -DWITH_TESTS=ON \
  -DCORE_TEST_SANITIZER_PROFILE=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan-ubsan --target CoreEngineModuleTests
ctest --test-dir build-asan-ubsan --output-on-failure
```

### Explicitly limited case example (MSVC + UBSan request)

```powershell
cmake -S . -B build-msvc-asan -G "Visual Studio 17 2022" `
  -DWITH_TESTS=ON `
  -DCORE_ENABLE_ASAN=ON `
  -DCORE_ENABLE_UBSAN=ON
```

Expected configure behavior:

- A warning is printed that `CORE_ENABLE_UBSAN` is unsupported on MSVC toolchains and is disabled.
- ASan remains enabled.
## AI architecture and asset checks

With `WITH_TESTS=ON`, CTest includes `AI.ImportBoundaries` and
`AI.AssetComposition`. Configuration requires Python 3.9 or newer; these checks
are not silently skipped when the interpreter is missing. Run the architecture
label with the ordinary build directory:

```sh
ctest --test-dir out/build/<preset> -C Debug -L architecture --output-on-failure
```

They can also run independently of graphics libraries and the C++ toolchain:

```sh
cmake -S tests/architecture -B out/architecture
ctest --test-dir out/architecture -L architecture --output-on-failure
```

CI should treat a nonzero CTest exit code as failure. To enforce a merge gate,
configure the job running these checks as a required status in repository branch
protection. CTest registration itself does not change GitHub repository settings.

The optional `tests/tools/run_goap_asset_portable_checks.py` compiles authoring,
prepared templates and observation dependency ordering with GCC ASan/UBSan. It
uses inert runtime compiler callbacks and only entity aliases from EnTTHelpers;
world-dependent composition/execution is not tested by this harness. It is separate
from the cross-platform architecture checks and does not replace the MSVC module
build or runtime GoogleTests. See `docs/architecture/goap-prepared-templates.md`
for the snapshot/rebuild contract and relevant runtime suites.
