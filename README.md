# Epoch

Epoch is the repository for **Alpha**, a native macOS strategy/simulation game built around a deterministic monthly simulation loop.

Alpha is scoped as a **single-society feudal agrarian sandbox**. The game is intended to prove that terrain, settlement placement, roads, zoning, labor reach, local stockpiles, logistics, and settlement growth create meaningful outcomes.

This repository is currently a **partial implementation of the Alpha target architecture**, not a playable game. The documentation under `docs/` describes the intended end state; the codebase contains an early Milestone 1 native sim/bridge subset that is building toward that target.

## What Exists Today

The repo currently provides:
- locked planning and handoff documents
- a normalized monorepo structure for `game/`, `sim/`, `extension/`, `data/`, `tools/`, and `ci/`
- buildable CMake entrypoints for the current native modules
- local scripts for native build, native test, macOS export, and formatting entrypoints
- a GitHub Actions workflow that validates repository structure and required docs
- a buildable `alpha_sim` static library
- a real `godot-cpp`-backed `alpha_godot_bridge` GDExtension build path
- a minimal Godot project shell that can initialize the native bridge, create/load/save a world, and advance one month
- a minimal deterministic world lifecycle with chunk visual queries and smoke-test coverage

What it does **not** provide yet:
- a rendered gameplay map or wider Milestone 1 interaction surface
- full gameplay systems
- the full end-state bridge/query/command surface described in the design docs
- a production-ready packaging or formatter pipeline

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

## Documentation Model

The docs under `docs/` describe the **intended final state** of Alpha.

Use these roles:
- `docs/alpha_final_implementation_spec.md`, `docs/alpha_tech_stack_handoff.md`, `docs/simulation_api.md`, and `docs/save_format.md` describe the target product/runtime shape
- `docs/milestone_plan.md` describes delivery sequencing toward that target
- `docs/implementation_status.md` describes the subset that is implemented in code today
- `docs/Prompts/` records milestone task prompts and implementation history for work that has already been completed

## Current Milestone

The repository is in an **early Milestone 1 implementation state**.

The broader Milestone 1 target remains the stack-proving vertical slice, intended to prove:
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
- `docs/simulation_api.md`: target command/query contract for the sim boundary
- `docs/save_format.md`: target save ownership, versioning, and serialization boundaries
- `docs/coding_standards.md`: implementation rules and codebase expectations
- `docs/implementation_status.md`: current implemented subset and current callable API surface

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
- `build_macos.sh` builds the sim target and the macOS GDExtension and can launch the Godot shell with `--run`
- `run_tests.sh` configures, builds, and runs the current sim test suite with CTest
- `export_macos.sh` builds the release GDExtension and asks Godot to export the macOS app bundle
- `format.sh` reserves the canonical formatting entrypoint

The native build/export scripts require a local Godot 4.x executable and a matching local
`godot-cpp` checkout. They fail clearly when either prerequisite is missing.

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

Epoch remains an early Milestone 1 implementation, but the repo now includes the first real
Godot shell and GDExtension registration path in addition to the native sim/bridge spine. It is
still a stack-proofing vertical slice, not a feature-complete or polished game.
