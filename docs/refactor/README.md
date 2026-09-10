# Refactoring Campaign

Repository refactoring follows this bounded workflow:

```text
AGENTS.md
    ↓
master-plan.md
    ↓
generated repository index
    ↓
Rxx task
    ↓
targeted source inspection
    ↓
tests / architecture checks
```

[`master-plan.md`](master-plan.md) is the campaign roadmap and defines the intended
sequence of independently reviewable phases. [`debt-register.md`](debt-register.md)
is the human-maintained record of reproducible architectural debt discovered while
executing those phases. [`index/`](index/) is a generated navigation aid for finding
subsystems, module dependencies, tests, and structural hotspots; source code and
architecture checks remain authoritative.

Regenerate the index from the repository root with:

```sh
python tools/refactor/build_repo_index.py
```

Never edit generated files under `index/` manually. Refactoring proceeds only
through narrow Rxx tasks; use the index to limit source inspection to each task's
explicit scope.
