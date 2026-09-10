# R01 Repository Baseline

Baseline captured on 2026-09-10 before production refactoring.

## Repository and environment

| Item | Value |
|---|---|
| Branch | `work` |
| Baseline commit | `4e6135afcb16eadf8cf745ca6419df75164b8c0d` |
| Host | Linux 6.18.35, x86-64 |
| Intended primary toolchain | Windows, Visual Studio 2022 MSVC 14.4x, C++23, CMake, Ninja |
| Available toolchain | CMake 3.28.3; Ninja 1.11.1; Python 3.12.13; GCC 13.3.0 |
| Primary configuration | `out/build/x64-Debug-DX12`, Debug, DX12, target `app` |

The documented Windows/MSVC toolchain and an existing configured DX12 build tree
were not available in this Linux environment. No build directory was deleted or
reconfigured.

## Results

| Category | Result | Command / notes |
|---|---|---|
| Architecture | **PASS** | `cmake -S tests/architecture -B /tmp/coreengine-r01-architecture -G Ninja`, then `ctest --test-dir /tmp/coreengine-r01-architecture --output-on-failure`: 2/2 passed (`AI.ImportBoundaries`, `AI.AssetComposition`). |
| Architecture (direct) | **PASS** | `python3 tests/architecture/check_ai_dependencies.py` and `python3 tests/tools/check_goap_asset_composition.py`. |
| Portable GOAP check | **PASS** | `python3 tests/tools/run_goap_asset_portable_checks.py` completed successfully. This is a supplemental portable check, not a replacement for the MSVC module build or runtime GoogleTests. |
| Unit | **NOT RUN** | The `CoreEngineModuleTests` C++ target was not built because no configured build tree exists and the primary DX12 configuration is Windows-only. |
| Integration | **NOT RUN** | The physics, character, and AI integration targets were unavailable for the same toolchain/configuration reason. |
| Smoke | **NOT RUN** | The Jolt, physics, and DX12 smoke targets were unavailable for the same reason. |
| DX12 build | **NOT RUN** | `cmake --build out/build/x64-Debug-DX12 --target app` stopped before compilation because the documented build directory does not exist. A DX12 configuration cannot be created on Linux. |
| OpenGL build | **NOT RUN** | No existing OpenGL configuration was present; creating one was outside the optional baseline scope. |
| Index determinism | **PASS** | `python3 tools/refactor/build_repo_index.py` indexed 476 files, 176 modules, 713 dependency edges, 167 test mappings, and 40 hotspots without changing tracked index files. |

## Existing failures and classifications

No repository test or architecture failure was reproduced.

| Command | Affected target | Classification | Summary / reproduction |
|---|---|---|---|
| `cmake --build out/build/x64-Debug-DX12 --target app` | `app` | Configuration failure caused by environment/toolchain availability | CMake reports that `out/build/x64-Debug-DX12` is not a directory. The repository rejects DX12 configuration on non-Windows hosts, so compilation was not attempted. |

This pre-build failure is not a refactoring regression and is not evidence of
production-code debt.

## Warnings and limitations

- `/AGENTS.md` was not present in the container; the repository-root `AGENTS.md`
  and the task-provided instructions were used.
- Runtime GoogleTests and the application were not executable without a compatible
  configured C++ build.
- The generated repository overview, hotspot list, module dependency table, and
  test map were present, internally consistent at a lightweight review, and
  reproducible. Hotspot scores remain navigation signals rather than debt claims.
