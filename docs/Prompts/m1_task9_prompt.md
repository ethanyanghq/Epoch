# Alpha Milestone 1 Task 9 Prompt

Implement Milestone 1, Task 9: the first real monthly settlement simulation and settlement detail query.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/alpha_final_implementation_spec.md` defines the Milestone 1 monthly order, labor, resource, and farmland behavior that should drive this task
- `docs/simulation_api.md` is the contract source of truth for `TurnReport`, `SettlementDetail`, and related read models
- `docs/coding_standards.md` is the determinism and architecture guardrail
- `docs/milestone_plan.md` defines the Milestone 1 requirement for deterministic monthly stepping with visible phase timing
- `docs/save_format.md` should stay aligned with any new labor, plot, and project state that lands here

Objective:
Create the next smallest end-to-end proof after Task 8:
- implement the first meaningful one-settlement monthly simulation loop
- add the first real `get_settlement_detail(SettlementId)` query
- move Milestone 1 from placeholder month advancement to a real stockpile/labor/project update path
- surface phase timing and outcome reporting through `TurnReport`

Scope:
- implement the minimum authoritative settlement simulation needed for Milestone 1:
  - local food / wood / stone stockpile updates
  - monthly labor allocation
  - basic food consumption and shortage signaling
  - farmland opening / active / fallow flow
  - prototype forestry and quarry output
  - construction labor/progress participation
- implement `get_settlement_detail(SettlementId)` in `WorldApi` and expose it through the bridge
- add the minimum farm plot and labor state needed to answer the typed detail query cleanly

Required behavior:
- `advance_month()` performs a real deterministic monthly pass with stable phase ordering
- `TurnReport.phase_timings` contains meaningful phase timing entries for the real monthly pass
- `TurnReport` reports stockpile deltas, population deltas where applicable, completed projects, blocker transitions, and dirty outputs consistently with sim state
- `get_settlement_detail()` returns a valid typed `SettlementDetail`
- `SettlementDetail` includes:
  - the current summary
  - labor fill and labor demand views
  - current farm plot views
  - transport/development placeholders or real values consistent with the current Milestone 1 subset
- repeated 12-month advancement on identical seeds and identical command streams produces identical outputs across runs
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific rule/contract source between `docs/alpha_final_implementation_spec.md` and `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- keep the simulation intentionally narrow to the one-settlement Milestone 1 slice
- do not invent end-state complexity that belongs to Milestone 2
- preserve stable iteration order for labor, plots, projects, and settlements
- keep hidden accumulators and any RNG state saveable from the first version that lands
- if some `SettlementDetail` fields remain placeholders for Milestone 1, keep them typed and explicitly documented

Explicit non-goals for this task:
- no multi-settlement logistics
- no founding or expansion
- no fully polished balancing
- no Godot UI work yet
- no route debug query

Constraints:
- authoritative gameplay state must live only in `sim/`
- the bridge must only convert/expose types and call sim methods
- design for determinism and save/load compatibility from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/core/simulation.hpp`
- `sim/include/alpha/world/world_state.hpp`
- settlement/farm helpers such as:
  - `sim/include/alpha/settlements/labor_state.hpp`
  - `sim/include/alpha/settlements/farm_plots.hpp`
  - matching `.cpp` files in `sim/src/settlements/`
- updates to:
  - `sim/src/core/simulation.cpp`
  - `sim/src/api/world_api.cpp`
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`, `sim/tests/scenario/`, or `sim/tests/regression/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `advance_month()` performs a meaningful deterministic Milestone 1 monthly pass
- `get_settlement_detail()` is implemented in `WorldApi` and exposed through the bridge
- stockpile, labor, and project state change coherently across month advancement
- `TurnReport` contains meaningful phase timing and outcome data
- fixed-seed multi-month runs are reproducible across repeated executions
- the implementation is consistent with:
  - `docs/simulation_api.md`
  - `docs/alpha_final_implementation_spec.md`
  - `docs/save_format.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build and test surface that exists at this point and report exactly what was run

Recommended tests:
- advancing one month updates stockpiles and project state coherently for the starting settlement
- `get_settlement_detail()` returns deterministic labor and plot views for identical worlds
- fixed-seed 12-month runs with identical commands produce identical turn reports and settlement summaries
- shortage conditions set `food_shortage_flag` and related outputs consistently
- save/load in the middle of a multi-month run preserves future determinism if Task 5 save/load landed fully

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md` or Milestone 1 rule assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
