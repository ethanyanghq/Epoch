# Alpha Milestone Implementation Plan

## 1. Purpose

This document operationalizes the Alpha engineering handoff into a milestone-based execution plan.

It is the delivery plan for building Alpha from the locked gameplay scope in `EpochAlphaScope.md` and the locked technical approach in `EpochStack.md`.

## 2. Source precedence

When documents conflict, use this order:
1. `EpochAlphaScope.md`
2. `EpochStack.md`
3. This milestone plan for sequencing, ownership, and delivery gates

This plan does not change gameplay rules unless it explicitly calls out an implementation-owned tuning area already allowed by the scope spec.

## 3. Program objective

Alpha must ship as a native macOS game with:
- Godot 4.x as the app shell
- a C++20 deterministic simulation core
- a thin GDExtension bridge
- a monthly turn-based feudal agrarian sandbox proving that terrain, roads, labor, zoning, local stockpiles, logistics, founding, and settlement expansion matter

## 4. Delivery principles

- De-risk architecture before feature breadth.
- Keep authoritative gameplay state in the C++ sim only.
- Deliver a playable vertical slice early, then widen scope.
- Add deterministic tests as each rule system lands.
- Treat save/load, blocker explainability, and debug visibility as product requirements, not cleanup work.
- Prefer data-driven tuning tables over hard-coded constants scattered through runtime code.

## 5. Planning assumptions

- Baseline plan assumes a small engineering team and must remain executable on a mostly serial critical path.
- Parallel work should begin only after the world model, bridge contract, and render/query shapes are stable.
- Visual polish is intentionally deferred until the full Alpha loop is playable and inspectable.
- Alpha scope remains limited to food, wood, and stone as active resources.
- Alpha internal buildings remain limited to Estate I and Warehouse I.

## 6. Workstreams

The full implementation should be managed across five workstreams:

- Simulation core: world state, map, zones, labor, logistics, construction, save/load, determinism
- Game shell and UI: Godot scenes, camera, selection, inspectors, overlays, turn flow, reports
- Native bridge and platform: GDExtension, CMake, local scripts, macOS packaging pipeline
- Data and balance: terrain tables, building costs, labor weights, logistics thresholds, test scenarios
- Quality and tooling: unit/scenario/regression tests, profiling, debug panels, golden snapshots, CI

## 7. Critical path

The critical path for the plan is:
1. Lock repo layout, interface boundaries, and docs
2. Prove the engine-shell plus sim-core stack
3. Complete the single-settlement gameplay loop
4. Add multi-settlement founding and logistics
5. Add automatic expansion and full Alpha control/reporting surface
6. Harden saves, performance, tests, and packaging

## 8. Non-negotiable runtime targets

These targets come from the locked Alpha scope and should be used as planning gates throughout implementation:

- World size: 1024 x 1024 cells
- Settlement ceiling: up to 48 settlements
- Zone ceiling: up to 400 zones
- Road ceiling: up to 40,000 road cells
- Project ceiling: up to 200 active projects
- Visible monthly resolution target: about 1.5 seconds
- Monthly sim target at normal load: under 300 ms
- Monthly sim target at heavy late-game load: under 750 ms

## 9. Milestone overview

| Milestone | Goal | Primary proof |
| --- | --- | --- |
| M0 | Program baseline and spec lock | Team can build against one consistent architecture and doc set |
| M1 | Stack-proving vertical slice | Native macOS build, deterministic monthly step, chunked map, one-settlement interaction |
| M2 | Single-settlement Alpha core | One settlement can survive, produce, build, and fail according to locked rules |
| M3 | Multi-settlement network | Founding and inter-settlement logistics create meaningful network outcomes |
| M4 | Full Alpha feature completion | Automatic expansion and full player control/reporting complete the locked Alpha loop |
| M5 | Hardening and release readiness | Performance, saves, regression coverage, and Mac packaging are production-ready |

## 10. Milestone 0: Program baseline and spec lock

### Objective

Create the implementation foundation so the team can build without making new structural decisions midstream.

### In scope

- Normalize the repo into the intended monorepo shape across `docs/`, `game/`, `sim/`, `extension/`, `data/`, `tools/`, and `ci/`
- Copy or rename handoff docs into stable documentation locations if desired, without changing their contents
- Author `docs/simulation_api.md`
- Author `docs/save_format.md`
- Author `docs/coding_standards.md`
- Define module ownership for `sim`, `game`, and `extension`
- Define naming rules for stable IDs, command types, query types, and result types
- Set up local build, test, and export script placeholders
- Set up CI skeleton for build and test execution

### Required design decisions

- Final command/query boundary between Godot and the sim
- Chunk size and query shape for render data and overlays
- Save snapshot ownership and versioning approach
- Canonical data file locations for balance tables and scenarios

### Key artifacts

- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`
- repo skeleton and build script stubs

### Exit criteria

- The repo structure matches the intended architecture closely enough for implementation to start
- The bridge contract is documented with command batches, query batches, and turn/report results
- Save boundaries are documented clearly enough that gameplay engineers do not infer state from Godot scenes
- Coding standards explicitly lock the "sim authoritative, Godot non-authoritative" rule

## 11. Milestone 1: Stack-proving vertical slice

### Objective

Retire the highest technical risks before expanding gameplay scope.

### Product proof

Alpha launches as a native macOS prototype that renders a map, accepts player input, advances one deterministic month in C++, and explains core outcomes in UI.

### In scope

- Godot desktop shell that launches on macOS
- C++ sim library with a minimal `World`, map grid, stable IDs, and `advance_month()`
- Thin GDExtension bridge exposing lifecycle, batched commands, batched queries, and performance counters
- Chunked terrain renderer with camera pan/zoom and hovered-cell inspection
- One overlay path for terrain or fertility
- One settlement with stockpile, population, and summary panel
- Zoning input for farmland, forestry, and quarry legality
- Road drawing input with terrain validation and movement-cost effect
- One construction queue with road projects and one internal building project path
- One-month turn pipeline with visible phase timing report
- Save/load roundtrip
- Deterministic test harness and profiling/debug panel

### Implementation sequence

1. Build the empty Godot shell and native library integration.
2. Create `World`, map storage, IDs, and minimal monthly turn pipeline in C++.
3. Define the first stable bridge methods:
   - `create_world(seed, settings)`
   - `load_world(path)`
   - `save_world(path)`
   - `apply_commands(batch)`
   - `advance_month()`
   - `get_chunk_visual(chunk_x, chunk_y, layer)`
   - `get_overlay_chunk(chunk_x, chunk_y, overlay_type)`
   - `get_settlement_summary(id)`
   - `get_last_turn_report()`
4. Implement chunk rendering, camera, selection, and one overlay.
5. Implement zoning legality and road queueing against the sim.
6. Implement a minimal settlement inspector and turn report UI.
7. Add deterministic run and save/load equivalence tests.

### Deferred from this milestone

- Multi-settlement play
- Inter-settlement logistics
- Founding
- Settlement expansion
- Estate I and Warehouse I as full gameplay systems beyond the single prototype building path
- Final art, audio, and polished debug UX

### Exit criteria

- The game launches natively on macOS
- The map renders through chunk queries, not per-cell scene nodes
- A fixed seed plus fixed commands produces identical 12-month results across runs
- Save, quit, reload, and continue produces the same future results
- Zoning and roads are performant enough for prototype use
- UI can show blocker reasoning for at least one project type

## 12. Milestone 2: Single-settlement Alpha core

### Objective

Complete the one-settlement gameplay loop so the player can see the full local economy and labor model operating under locked Alpha rules.

### In scope

- Full settlement data model
- Population growth, mortality, food coverage, and starvation bands
- Labor allocator with protected base roles, priority-weighted extra roles, and inertia
- Local stockpile reservations and consumption rules
- Farmland component model, plot generation, plot opening, active/fallow states, and November structural review
- Forestry and quarry production using zone components directly
- Construction queue model with statuses, reservations, labor usage, and blocker reasons
- Estate I and Warehouse I construction and staffing behavior
- Road projects that build cell by cell and affect access immediately
- Priority controls for projects and labor-sensitive work
- Read models for stockpiles, labor breakdown, projects, and blocker explanations

### Required supporting artifacts

- Balance tables for terrain movement, road costs, building costs, labor weights, and food consumption
- Scenario fixtures for labor shortage, food shortage, blocked construction, and road/farmland interaction
- Save schema updates for plots, role state, projects, reserves, and hidden accumulators

### Exit criteria

- One settlement can survive, grow, starve, produce, and build according to the locked monthly order
- Roads materially change access and construction outcomes
- Farmland expansion and retreat behave according to November-only structural changes
- Estate I bonus applies only when built and staffed
- Blocker reporting can answer why a project is not building faster
- Unit and scenario coverage exists for the new rule systems

## 13. Milestone 3: Multi-settlement network

### Objective

Add the first networked gameplay systems so location and connectivity begin to matter beyond one settlement footprint.

### In scope

- Multiple active settlements in one world
- Settlement founding as a multi-month project created by a source settlement
- Founding legality checks against terrain, reachability, footprint overlap, and zoned land conflicts
- Source-settlement population and material transfer rules
- Settlement node graph and cheapest-valid-land-path routing between settlements
- Pull-based import/export flow using local deficits, surpluses, and reserve rules
- Transport-capacity accounting
- Route/access/logistics debug overlays
- Turn report additions for imports, exports, shortages, and transport bottlenecks

### Implementation sequence

1. Add settlement-to-settlement path queries and connectivity caching.
2. Implement founding commands, project data, completion flow, and save-state persistence.
3. Add logistics solver with reserve logic, request generation, exporter selection, and transport spend.
4. Add read models and overlays for route costs, trade connections, and unmet demand.
5. Add scenario and regression coverage for multi-settlement outcomes.

### Exit criteria

- New settlements can be founded legally from an existing source settlement
- Connected settlements can move resources through the monthly logistics pass
- Food and construction shortages propagate locally and across the network without global pooling
- Roads have visible economic value at network scale
- Deterministic regression coverage includes at least one multi-settlement scenario

## 14. Milestone 4: Full Alpha feature completion

### Objective

Complete the remaining locked Alpha systems and the full player control/reporting surface.

### In scope

- Automatic settlement expansion using development pressure
- Expansion candidate validity, scoring, costs, and one-cell-at-a-time project flow
- Interaction between settlement expansion, zones, road occupancy, and farmland recalculation
- Final player action set:
  - zone land
  - draw roads
  - queue Estate I and Warehouse I
  - sponsor founding
  - set priorities
  - advance the month
- Final project feedback set:
  - required materials
  - labor requirement
  - skill requirement
  - priority
  - expected bottleneck
- Final overlay set for terrain, access, zones, roads, shortages, and blockers
- Stronger monthly turn reports and settlement inspectors

### Exit criteria

- The complete locked Alpha feature set is playable end to end
- Settlement growth changes land use through automatic expansion
- Roads, zoning, stockpiles, logistics, and expansion interact without desynchronization
- All player-visible blocker reasons come from the sim, not UI-side inference
- Saves restore the full Alpha state exactly enough to continue deterministically

## 15. Milestone 5: Hardening and release readiness

### Objective

Turn feature-complete Alpha into a stable, testable, and shippable macOS product.

### In scope

- Performance optimization against target monthly compute budgets
- Scenario suite growth and golden regression coverage
- Save compatibility hardening and version migration policy
- Debug tooling for labor, access, routes, stockpiles, caches, and project bottlenecks
- Usability pass on inspectors, overlays, and turn reports
- Data-driven tuning pass for implementation-owned values
- macOS export, signing, notarization, and packaging pipeline
- CI jobs for build, tests, and regression snapshots

### Exit criteria

- Normal-load and heavy-load monthly sim budgets are met or clearly profiled with remaining hotspots isolated
- Regression suite protects core outcomes for stockpiles, population, projects, zones, and plots
- Save/load is stable across supported build versions
- Mac export pipeline produces a distributable build
- Alpha is ready for structured playtesting and balance iteration

## 16. Public interfaces that must be explicitly specified

These interfaces must be documented and versioned during implementation rather than improvised in code:

- Sim commands for world lifecycle, zoning, road queueing, building queueing, founding, priority changes, and month advance
- Sim queries for chunk visuals, overlays, settlement summaries, stockpiles, labor breakdowns, projects, route/access views, and blocker reasons
- Result types for turn reports, validation failures, project updates, and save/load outcomes
- Stable ID types for settlements, zone components, farm plots, projects, and any persistent road grouping
- Save snapshot schema and version rules

## 17. Testing plan by stage

### Unit tests

Add unit tests as each system lands for:
- movement cost
- zoning legality
- zone splitting and minimum-size cleanup
- farm plot ordering and structural review rules
- labor allocation and priority behavior
- starvation bands
- founding validation
- expansion scoring
- logistics reservation behavior

### Scenario tests

Maintain handcrafted scenarios for:
- road cutting through farmland
- farmland retreat under labor shortage
- blocked project waiting on wood or stone
- food shortage and starvation
- founding legality and failure cases
- import/export bottlenecks
- settlement expansion consuming zoned land

### Regression tests

Use deterministic fixtures defined by:
- seed
- starting world data
- command sequence
- month count

Assert exact final state for:
- stockpiles
- population
- projects
- zone components
- farm plots
- settlement footprints

## 18. Risks and mitigations

- Too much logic in Godot: enforce that all gameplay rules and blocker reasons originate in the sim
- Chatty bridge API: use batched commands, chunk queries, and coarse-grained read models only
- Nondeterministic sim: use stable iteration, fixed-point or integer math, deterministic RNG, and golden tests from milestone 1 onward
- Render overhead on large maps: use chunk rendering, not node-per-cell scene graphs
- Scope drift: use the hard exclusions in `EpochAlphaScope.md` as release boundaries
- Overengineering infrastructure: do not build custom engine systems when Godot can handle the shell, UI, and rendering needs directly

## 19. Definition of done for the full plan

The implementation plan is considered fulfilled when:
- Alpha's locked gameplay scope is complete with no excluded systems leaking in
- The game is deterministic for identical seeds and input sequences
- The macOS build, save/load, and regression pipeline are operational
- The UI can explain simulation outcomes without hidden gameplay logic
- The codebase structure matches the architecture split in the stack handoff
