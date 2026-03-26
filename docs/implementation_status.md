# Alpha Implementation Status

## Purpose

This document tracks the **current implemented state** of the repository.

Use it alongside the design docs:
- `docs/alpha_final_implementation_spec.md`, `docs/alpha_tech_stack_handoff.md`,
  `docs/simulation_api.md`, and `docs/save_format.md` describe the **intended end state**
  for Alpha.
- this file describes the **currently landed subset** in the codebase
- `docs/Prompts/` records milestone task prompts and implementation history for work that has
  already been completed

When the current code and the end-state docs differ, the difference should be treated as
implementation progress unless this file or the relevant code explicitly says otherwise.

## Authoritative sources for the current state

For the callable API and runtime behavior that exist today, prefer:
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `extension/include/godot_bridge/alpha_world_bridge.hpp`
- unit tests under `sim/tests/unit/`

## Currently implemented

The native codebase currently includes:
- a buildable `alpha_sim` static library
- a buildable `alpha_godot_bridge` static library
- a minimal sim-owned `WorldApi`
- deterministic `create_world()` support for fixed `1024 x 1024` worlds
- sim-owned binary save/load round-tripping for the current Milestone 1 native subset
- deterministic calendar advancement through `advance_month()`
- the first typed `apply_commands()` batch path for zone painting and removal
- sim-owned terrain/map storage for:
  - elevation
  - slope
  - water
  - fertility
  - vegetation
- `get_chunk_visual()` for fixed `64 x 64` chunk queries
- `get_overlay_chunk()` for a sim-owned fertility overlay path
- `get_overlay_chunk()` for a sim-owned zone-owner overlay path
- one deterministic sim-owned starting settlement created with a new world
- `get_settlement_summary(SettlementId)` for the initial settlement summary read path
- `get_world_metrics()` for current live world counters
- authoritative sim-owned zone state for:
  - farmland
  - forestry
  - quarry
- sim-owned project queries through `get_projects(ProjectListQuery)`
- queue-time validation and creation for:
  - road projects
  - Warehouse I building projects
- deterministic monthly construction progression for:
  - queued road projects
  - one real Warehouse I building project path
- authoritative built-road persistence, chunk-visual rendering, and movement-cost impact on
  current reach/zoning calculations
- zone save/load persistence with stable zone ids and authoritative ownership
- project and built-road save/load persistence with stable project ids and live progress/status
- native tests covering world creation, chunk queries, overlay queries, settlement summary
  reads, metrics, month advancement, save/load equivalence, zoning mutations, zoning
  save/load equivalence, project queueing, construction progression, blocker reporting, and
  road access impact

## Current public API surface

The currently exposed sim/bridge methods are:
- `create_world(WorldCreateParams)`
- `load_world(LoadWorldParams)`
- `save_world(SaveWorldParams)`
- `apply_commands(CommandBatch)`
- `advance_month()`
- `get_chunk_visual(ChunkVisualQuery)`
- `get_overlay_chunk(OverlayChunkQuery)`
- `get_settlement_summary(SettlementId)`
- `get_world_metrics()`

## Not implemented yet

The following end-state interfaces and systems are still planned and should not be treated as
currently callable unless their code lands:
- settlement detail queries
- route debug queries
- Estate I as a real building project path
- founding, logistics, and settlement simulation beyond the current Milestone 1 zoning and
  construction slice
- real Godot binding registration through `godot-cpp`

## Working rule for documentation

Do not rewrite the end-state docs down to the current implementation.
Instead:
- keep the design docs describing the target Alpha state
- update this file when new code lands
- update the end-state docs only when the intended final design changes
