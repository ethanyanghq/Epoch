# Alpha Milestone 1 Task 4 Prompt

Implement Milestone 1, Task 4: the first settlement summary query for Alpha.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/coding_standards.md`
- `docs/save_format.md`

Use them proactively while implementing:
- `docs/simulation_api.md` is the contract source of truth for query types, result shapes, and method names
- `docs/coding_standards.md` is the architecture and determinism guardrail
- `docs/milestone_plan.md` defines the current milestone target and what is intentionally out of scope
- `docs/alpha_final_implementation_spec.md` defines the minimal settlement start state and fixed Alpha building constraints
- `docs/save_format.md` locks the current calendar convention and the fixed two-building-slot expectation that runtime state should stay consistent with
- the tech handoff defines the repo/module boundaries and the thin-bridge requirement

Objective:
Create the next smallest end-to-end proof after Task 3:
- one minimal authoritative settlement owned by the sim
- one minimal `get_settlement_summary()` query implemented in the sim and exposed through the bridge
- typed settlement summary structs aligned with `docs/simulation_api.md`
- one deterministic settlement read path sourced entirely from sim-owned world state
- no gameplay logic in the bridge

Scope:
- extend the real C++ implementation under `sim/include/alpha/` and `sim/src/`
- keep all authoritative settlement source data in `sim/`
- expand the bridge under `extension/include/godot_bridge/` and `extension/src/godot_bridge/`
- update `sim/CMakeLists.txt` and `extension/CMakeLists.txt` if needed
- implement only the minimum settlement state and query path needed to support one first settlement summary
- keep the bridge thin and non-authoritative, per `docs/coding_standards.md`

Required behavior:
- implement `SettlementSummary` and `BuildingStateView` in the typed API layer if they do not already exist there
- implement `get_settlement_summary(SettlementId)` in `WorldApi` and expose it through the bridge
- `create_world()` initializes exactly one minimal settlement owned by the sim
- the initial settlement uses the Alpha start template from `docs/alpha_final_implementation_spec.md` unless a compile-driven reason forces a smaller temporary subset, in which case note it in the final summary
- support exactly one real settlement summary path in this task:
  - the starting settlement created with a new world
- `SettlementSummary.settlement_id`, `center`, `population_whole`, `food`, `wood`, `stone`, `buildings`, and `active_zone_count` are populated consistently with the contract
- `SettlementSummary.buildings` uses the fixed two-building-slot model aligned with Alpha:
  - `BuildingType::EstateI`
  - `BuildingType::WarehouseI`
- the settlement summary is derived deterministically from sim-owned settlement/world data already stored in `sim/`
- repeated world creation with the same seeds produces identical settlement summary results for the same `SettlementId`
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- keep this task intentionally small; do not build the full settlement gameplay model yet
- do not add housing capacity, social classes, labor assignment systems, logistics rules, or construction progression in this task
- do not invent Godot-owned settlement state
- store only the minimum sim-owned settlement state needed to answer the typed summary query cleanly
- keep traversal order stable
- avoid floats for gameplay-critical state
- do not add dynamic dictionary-style payloads
- if `godot-cpp` / real GDExtension bindings are still not present in the repo, keep the bridge as a typed native wrapper and do not invent fake engine-owned state

Explicit non-goals for this task:
- no Godot UI scenes or settlement panel rendering code
- no save/load implementation
- no zoning simulation
- no roads simulation
- no logistics
- no founding gameplay
- no settlement expansion
- no project queue behavior
- no additional settlement detail queries beyond the first summary needed here
- no per-cell Godot API methods
- no broad query surface beyond what this task needs

Constraints:
- authoritative gameplay and settlement state must live only in `sim/`
- GDExtension must only convert/expose types and call sim methods
- design for determinism from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`
- do not add more per-cell state than allowed by `docs/alpha_final_implementation_spec.md`

Suggested first files:
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/core/simulation.hpp`
- `sim/include/alpha/settlements/settlement_types.hpp` or an equivalent small settlement model header if that fits the current layout better
- matching `.cpp` files in `sim/src/settlements/`
- updates to:
  - `sim/include/alpha/world/world_state.hpp`
  - `sim/src/core/simulation.cpp`
  - `sim/src/api/world_api.cpp`
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `create_world()` initializes one deterministic sim-owned starting settlement
- `get_settlement_summary()` is implemented in `WorldApi` and exposed through the bridge
- querying the starting settlement returns a correctly shaped typed summary result
- the returned building state uses the fixed two-slot Alpha building model consistently
- two worlds created with the same seeds return identical settlement summary data for the same settlement id
- the implementation is consistent with:
  - `docs/simulation_api.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`
  - `docs/alpha_final_implementation_spec.md`

Recommended tests:
- valid world creation still succeeds for `1024x1024`
- querying the starting settlement id returns a populated summary with deterministic stockpile and building values
- querying the same settlement twice from the same world returns identical results
- creating two worlds with the same seeds returns identical settlement summary results
- building entries are stable and remain in the expected fixed Alpha slot order
- if invalid settlement ids are handled explicitly for now, test that behavior explicitly and document it

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md`
- identify any ambiguities or conflicts found in the docs
- state the next immediate task after this one
