# CoreEngine — Master Refactoring Plan

Target branch: `Deferred`

## Objective

Perform a repository-wide architectural and maintainability refactor without turning it into a single uncontrolled rewrite.

The refactor must:

* preserve existing behavior unless a phase explicitly changes a contract;
* proceed through small reviewable patches;
* make ownership and subsystem boundaries explicit;
* reduce cross-layer coupling;
* improve build/test/refactor ergonomics;
* leave architecture rules mechanically enforceable where practical;
* avoid repeatedly loading the entire repository into an AI agent context.

The work is organized as a sequence of independently mergeable phases.

---

## Phase 0 — Refactoring Harness and Repository Index

No production behavior changes.

Create:

```text
AGENTS.md

docs/refactor/
    README.md
    master-plan.md
    debt-register.md

    index/
        repo.md
        hotspots.md
        file-metrics.tsv
        module-deps.tsv
        test-map.tsv

        subsystems/
            app.md
            animation.md
            assets.md
            core.md
            ecs.md
            editor.md
            gameplay.md
            ai.md
            input.md
            level.md
            navigation.md
            physics.md
            render.md
            scene.md

tools/refactor/
    build_repo_index.py
```

### `AGENTS.md`

Keep it intentionally small: roughly 80–120 lines.

It is the entry point, not the architecture encyclopedia.

It should contain:

```text
project identity
toolchain constraints
build/test commands
non-negotiable architecture rules
patch discipline
links to refactor/index documents
instructions for locating deeper context
```

Do not duplicate the complete README, CLAUDE.md, or architecture documents.

### Repository index

`build_repo_index.py` should inspect tracked repository files and generate the index without dumping source contents to stdout.

For every relevant source file collect at least:

```text
path
subsystem
file type
byte size
line count
declared C++ module / partition
direct module imports
```

For tests collect:

```text
test path
probable subsystem
imported project modules
test executable/group where determinable
```

Exclude from source indexing:

```text
.git
out/
build outputs
dlls/
extern/
large binary assets
FBX/textures/models
generated IDE files
```

Authored JSON assets should be indexed by path/category but their complete contents must not be loaded globally.

### Module graph

Generate direct C++ module edges:

```text
module A -> module B
```

Do not generate a gigantic natural-language explanation for every edge.

Produce compact TSV plus a short human-readable summary containing:

```text
high fan-in modules
high fan-out modules
cross-subsystem dependencies
suspected dependency inversions
potential cycles
large module interfaces
```

### Hotspot index

Rank candidates using signals such as:

```text
LOC / file size
module fan-in/fan-out
number of responsibilities
number of dependents
cross-subsystem imports
frequency of changes if cheaply available
test coverage proximity
```

File size alone must never be treated as proof that a refactor is necessary.

### Index consumption rule

Future Codex tasks must follow:

```text
AGENTS.md
    ↓
docs/refactor/index/repo.md
    ↓
relevant subsystem index
    ↓
symbol search
    ↓
only relevant implementation/tests
```

Never:

```text
read entire repository
    ↓
think about task
```

---

## Phase 1 — Baseline and Architectural Safety Net

Before structural modifications, establish the current known-good state.

Record:

```text
DX12 build status
unit-test status
integration/smoke status
architecture checks
known existing failures
known warnings that are not part of this refactor
```

Do not fix unrelated failures while establishing the baseline.

Expand `tests/architecture/` gradually so important dependency boundaries become executable contracts.

Candidate rules:

```text
generic AI must not import domain-specific AI
Scene must not import renderer backend
Gameplay must not import DX12/OpenGL
RHI must not depend on App/Gameplay/Editor
renderer backend must stay behind Render/RHI boundaries
runtime modules must not depend on editor UI
```

---

## Phase 2 — Build and Module Organization

Refactor the top-level build description without changing runtime architecture.

Goals:

```text
reduce CMakeLists.txt responsibility
group module source lists by subsystem
separate common / DX12 / OpenGL / tests configuration
make source ownership obvious
avoid duplicated backend setup
```

Possible target structure:

```text
cmake/
    CoreSources.cmake
    AnimationSources.cmake
    GameplaySources.cmake
    RenderSources.cmake
    Dx12Sources.cmake
    OpenGLSources.cmake
    Dependencies.cmake
    CompilerOptions.cmake
```

Keep actual C++ module names stable during this phase.

No broad source-file moves merely to make directories prettier.

---

## Phase 3 — App as Composition Root

`App` should orchestrate systems, not contain subsystem implementation logic.

Audit and decompose:

```text
AppLifecycle
AppBootstrap
App runtime helpers
mode switching
resize handling
asset streaming drive
frame coordination
debug UI invocation
renderer invocation
shutdown
```

Preserve the existing frame order exactly while extracting responsibilities.

Desired direction:

```text
App
 ├── initialization/composition
 ├── platform/window ownership
 ├── frame coordination
 ├── editor/game mode coordination
 └── shutdown coordination
```

Subsystem algorithms should remain in their owning subsystems.

Avoid creating a generic service-locator framework.

---

## Phase 4 — Scene / Level / Editor / Render Ownership Boundary

This is one of the highest-priority architectural phases.

Finish the existing ownership direction:

```text
LevelAsset
    authored serializable data

LevelInstance
    runtime binding between authored data and runtime entities/resources

Scene
    authoritative runtime world representation

EditorSceneState
    transient editor-only state

RenderFramePacket
    immutable renderer-facing frame representation
```

Target dependency flow:

```text
LevelAsset
    ↓
LevelInstance
    ↓
Scene / runtime state
    ↓
RenderSceneExtractor
    ↓
RenderFramePacket
    ↓
Renderer
```

Renderer must eventually stop traversing live `Scene`.

Move editor-only selection/gizmo/transient state out of runtime Scene where practical.

Do not solve this by placing a broad `shared_mutex` around Scene.

---

## Phase 5 — Runtime / Render Thread Ownership and Handoff

Do not necessarily introduce a second render thread yet.

First make the ownership contract correct.

Establish explicit render-owner affinity for:

```text
Renderer
RHI device
command submission
swapchains
present
resize
GPU texture/mesh upload
GPU resource destruction
bindless/descriptor updates
ImGui GPU submission
```

Add development assertions where appropriate.

Replace or explicitly constrain immediate execution paths so a future non-render caller cannot accidentally execute GPU work.

Target asset handoff:

```text
worker
    CPU load/decode
        ↓
protected completion queue
        ↓
render owner
    GPU upload/destruction
```

Shutdown ordering must remain deterministic.

---

## Phase 6 — Editor / Debug UI Isolation

Follow the existing ViewModel / Command / Service direction.

Target:

```text
runtime state
    ↓ snapshot
ViewModel
    ↓
ImGui View
    ↓ command/edit
Command/Service
    ↓
runtime owner applies mutation
```

Views must not gradually become owners of:

```text
Scene&
LevelInstance&
Renderer&
RHI objects
Asset runtime objects
```

Move panel by panel.

Do not create a giant UI framework or mandatory base hierarchy.

---

## Phase 7 — Asset and Resource Pipeline

Audit:

```text
AssetManager
ResourceManager
CPU decoding
mesh/texture import
streaming state
in-flight requests
GPU upload handoff
resource destruction
descriptor/binding updates
error reporting
```

Make resource lifecycle states explicit and deterministic.

Prefer:

```text
Unloaded
Loading
Loaded
Failed
```

plus only the additional states actually required by current behavior.

Verify:

```text
duplicate in-flight requests
failure/retry semantics
shutdown with pending work
externally-owned GPU resources
imported resources
upload/destroy ownership
```

Do not introduce a new allocator or async framework without measured need.

---

## Phase 8 — Renderer / RenderGraph / RHI

Once Scene/runtime ownership is clean, audit the rendering layers internally.

Maintain:

```text
App
 ↓
Renderer facade
 ↓
RenderGraph / render infrastructure
 ↓
RHI
 ↓
DX12 or OpenGL backend
```

Refactor targets:

```text
RHI interface responsibility
resource ownership
RenderGraph imported-vs-owned resources
attachment/framebuffer validation
pass construction boundaries
descriptor lifetime
upload infrastructure
backend leakage
shader/PSO caches
debug rendering
per-frame allocation hot paths
```

DX12 remains the primary backend.

For OpenGL, explicitly choose between:

```text
maintained compatibility backend
```

or

```text
best-effort/frozen backend
```

Do not pay permanent complexity for an undefined compatibility promise.

---

## Phase 9 — Gameplay / Animation / Physics / Navigation Runtime Boundaries

Preserve the existing gameplay frame contract:

```text
BeginFrame
    ↓
PreAnimationUpdate
    ↓
Scene animation update
    ↓
PostAnimationUpdate
```

Audit ownership between:

```text
GameplayWorld
Character
Action
Combat
Interaction
GameplayGraph
Animation bridge
Physics
Navigation
Scene sync
```

The graph layer should orchestrate state/tasks, not absorb character movement or domain algorithms.

Review the Animation split between:

```text
Animation/Assets
Animation/Runtime
Animation/Serialization
Animation/Tooling
Gameplay/Animation
Render animation/model data
```

Move code based on ownership, not merely based on filenames.

Physics and Navigation should expose runtime services/data without leaking Jolt/Recast implementation details into higher gameplay layers.

---

## Phase 10 — AI / GOAP / Steering

Preserve the current asset-driven scenario direction.

Target generic AI dependency structure:

```text
generic GOAP
    planner
    world state
    goal definitions
    action definitions
    executor
    decision runtime
    action binding registry

domain layer
    semantic action runtimes
    observation adapters
    contextual entity resolution
    gameplay-event mapping

assets
    facts
    goals
    actions
    bindings
    routes
    scenario configuration
```

Generic GOAP must not learn about:

```text
AccessKey
coins
shops
doors
weapons
specific level node names
```

Continue enforcing this through architecture tests.

Review steering and navigation integration for duplicated runtime wiring, but do not invent a universal steering framework unless duplication actually justifies one.

Replanning, reservations and observation remain explicit contracts rather than hidden planner behavior.

---

## Phase 11 — Core APIs and C++23 Quality Pass

Only after major ownership boundaries stabilize.

Audit:

```text
ownership types
raw pointer semantics
shared_ptr usage
string_view/span lifetime
strong IDs/handles
nodiscard
error handling
assert vs runtime validation
hot-path allocations
const correctness
module interface size
unnecessary exported API
```

Do not mechanically convert code to ranges, expected, optional, or another C++23 feature.

Use language features only when they simplify an existing contract.

Do not replace the custom math system wholesale as part of general cleanup. Treat that as a separate migration requiring dedicated tests and performance/behavior validation.

---

## Phase 12 — Tests, Dead Code, Documentation and Final Cleanup

After architectural changes stabilize:

```text
remove obsolete compatibility paths
remove dead helpers
remove duplicated local abstractions
reduce stale comments
update architecture docs
update generated repository index
run complete test suite
run available sanitizers/static checks
record remaining intentional debt
```

Formatting-only cleanup must remain separate from logic patches.

---

# Patch Discipline

A phase is not a patch.

Each phase must be split into independently reviewable patches.

Preferred pattern:

```text
Patch A
behavior-preserving preparation

Patch B
architecture boundary introduction

Patch C
consumer migration

Patch D
old path removal

Patch E
tests / architecture guard
```

A normal patch should touch the minimum coherent set of files.

Do not combine:

```text
file moves
public API redesign
behavior changes
formatting cleanup
unrelated warnings
```

unless they are inseparable.

---

# Codex Context Discipline

For every refactor task:

```text
1. Read root AGENTS.md.
2. Read docs/refactor/index/repo.md.
3. Read only the relevant subsystem index.
4. Inspect architecture documentation explicitly referenced there.
5. Search for concrete symbols.
6. Read implementation files around those symbols.
7. Read corresponding tests.
8. Make the smallest coherent patch.
9. Update index only if architecture/file structure changed.
```

Large shell outputs must be redirected to files.

Do not dump:

```text
recursive trees
full build logs
complete grep results
generated dependency graphs
entire test logs
```

into the agent conversation.

Inspect only summaries or matching error sections.

---

# Definition of Done

The repository-wide refactor is complete when:

* subsystem ownership is understandable from structure and documentation;
* important dependency directions are architecture-tested;
* renderer no longer relies on mutable live runtime state across the render boundary;
* GPU/RHI ownership is explicit;
* App remains orchestration rather than subsystem implementation;
* editor UI operates through controlled snapshot/edit boundaries;
* asset CPU/GPU handoff is explicit;
* generic AI remains domain-independent;
* CMake/source ownership is maintainable;
* build/test/index instructions are reproducible;
* remaining technical debt is recorded rather than hidden;
* no single giant migration is required to understand or continue the architecture.
