# Epoch

Epoch is the repository for **Alpha**, a native macOS strategy/simulation game built around a deterministic monthly simulation loop.

Alpha is scoped as a **single-society feudal agrarian sandbox**. The game is intended to prove that terrain, settlement placement, roads, zoning, labor reach, local stockpiles, logistics, and settlement growth create meaningful outcomes.

This repository is currently an **implementation baseline**, not a playable game. The architecture, documentation, repository layout, and script entrypoints are in place so Milestone 1 implementation can begin on stable ground.

## What Exists Today

The repo currently provides:
- locked planning and handoff documents
- a normalized monorepo structure for `game/`, `sim/`, `extension/`, `data/`, `tools/`, and `ci/`
- baseline CMake entrypoints for the native modules
- placeholder local scripts for build, test, export, and formatting
- a GitHub Actions workflow that validates repository structure and required docs

What it does **not** provide yet:
- a playable Godot application
- a compiled simulation library
- a working GDExtension bridge
- implemented gameplay systems
- a real packaging or formatter pipeline

## Project Direction

Alpha is planned as:
- **Engine shell:** Godot 4.x
- **Simulation core:** C++20
- **Native bridge:** Godot GDExtension
- **UI and tooling glue:** GDScript
- **Native build system:** CMake
- **Packaging target:** macOS

The core architecture rule is simple:
- the **C++ simulation** owns authoritative gameplay state
- **Godot** owns presentation, input, camera, overlays, menus, and debug UI
- the **extension layer** stays thin and only bridges typed commands and queries

If a system matters for determinism or game rules, it belongs in `sim/`, not in Godot scene state.

## Current Milestone

The repository is at **Milestone 0 / baseline lock**.

The next major target is **Milestone 1: stack-proving vertical slice**, which is intended to prove:
- native macOS launch
- deterministic monthly stepping in C++
- chunked terrain rendering
- one-settlement interaction path
- save/load round-tripping
- the initial Godot <-> sim bridge contract

## Repository Layout

```text
README.md                    Project overview and contributor entrypoint
docs/                        Canonical design, API, save, standards, and milestone docs
game/                        Planned Godot project shell, scenes, scripts, and export config
sim/                         Planned deterministic C++20 simulation core
extension/                   Planned GDExtension bridge between Godot and the sim
data/                        Balance tables, generation inputs, and scenario fixtures
tools/                       Local build, test, export, profiling, and formatting entrypoints
ci/                          CI workflow definitions
EpochAlphaScope.md           Root copy of the Alpha implementation scope/spec
EpochStack.md                Root copy of the stack and architecture handoff
```

## Canonical Docs

Start here if you are implementing against this repo:
- `docs/alpha_final_implementation_spec.md`: locked gameplay scope and core simulation rules
- `docs/alpha_tech_stack_handoff.md`: stack choice, architecture split, and repository shape
- `docs/milestone_plan.md`: delivery sequencing, milestone gates, and critical path
- `docs/simulation_api.md`: command/query contract for the sim boundary
- `docs/save_format.md`: save ownership, versioning, and serialization boundaries
- `docs/coding_standards.md`: implementation rules and codebase expectations

The root documents `EpochAlphaScope.md` and `EpochStack.md` mirror the same handoff material for convenience, but day-to-day implementation should anchor on the copies under `docs/`.

## Working With The Repo

### Baseline validation

From the repository root:

```bash
tools/scripts/build_macos.sh
tools/scripts/run_tests.sh
tools/scripts/export_macos.sh
tools/scripts/format.sh
```

Current behavior:
- `build_macos.sh` checks that baseline files and module entrypoints exist
- `run_tests.sh` checks required directories and documentation
- `export_macos.sh` reserves the canonical macOS export entrypoint
- `format.sh` reserves the canonical formatting entrypoint

These scripts are intentionally lightweight right now. They validate the baseline; they do not build or export a game yet.

### CI

`ci/github/workflows/validate.yml` runs the baseline validation scripts on pushes and pull requests.

## Implementation Expectations

A few rules are already locked by the handoff docs:
- simulation advances in discrete **monthly turns** only
- the simulation must be **deterministic** for the same seed and command history
- Alpha uses **food, wood, and stone** as the active economy
- authoritative game state should never be duplicated into Godot scene nodes
- save/load, blocker explainability, and debug visibility are part of the product, not cleanup tasks

## Suggested Reading Order

If you are new to the project, read in this order:
1. `docs/alpha_final_implementation_spec.md`
2. `docs/alpha_tech_stack_handoff.md`
3. `docs/milestone_plan.md`
4. `docs/simulation_api.md`
5. `docs/save_format.md`
6. `docs/coding_standards.md`

## Status Summary

Epoch is ready for implementation work, but it is still in the repository-and-spec baseline stage. The current repo is best understood as a well-defined starting point for building Alpha, not as a finished or partially playable product.
