# Alpha Milestone 1 Task 6 Prompt

Implement Milestone 1, Task 6: the first batched command path and zoning legality.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/simulation_api.md` is the contract source of truth for command payloads, result shapes, and method names
- `docs/alpha_final_implementation_spec.md` defines the allowed Milestone 1 zoning types and map/state constraints
- `docs/coding_standards.md` is the architecture and determinism guardrail
- `docs/milestone_plan.md` defines the Milestone 1 interaction path and command surface expectations
- `docs/save_format.md` should stay aligned with any new authoritative zone state that lands here

Objective:
Create the next smallest end-to-end proof after Task 5:
- implement the first real `apply_commands()` batch path in the sim and expose it through the bridge
- support zone painting and removal for:
  - farmland
  - forestry
  - quarry
- keep zoning legality and dirty-region ownership fully sim-side

Scope:
- add the typed command batch and validation result types required by `docs/simulation_api.md` if they do not already exist
- implement `apply_commands(CommandBatch)` in `WorldApi` and expose it through the bridge
- support exactly these real mutation commands in this task:
  - `ZoneCellsCommand`
  - `RemoveZoneCellsCommand`
- add minimal authoritative zone storage, stable `ZoneId` allocation, and settlement ownership tracking needed for Milestone 1 zoning
- update settlement summary/metrics output so zone counts reflect applied commands

Required behavior:
- `apply_commands()` accepts a batch of typed commands and returns a typed `BatchResult`
- zone commands validate:
  - settlement id exists
  - target cells are in bounds
  - target cells satisfy the current legality rules for the requested zone type
- removing zone cells updates authoritative zone ownership cleanly and deterministically
- accepted zone mutations dirty the correct chunks, overlays, and settlement summaries
- rejected commands produce stable `CommandOutcome` entries with a reject reason and message
- repeated application of the same valid command batch on identical worlds produces identical results
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific command contract in `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- keep this task focused on authoritative zoning and legality, not on full production gameplay yet
- choose explicit, deterministic zone storage over ad hoc cell flags
- keep iteration and ID allocation stable
- if zone merging/splitting logic is needed, keep it minimal and deterministic
- preserve a clean path for later farm plot generation and construction ownership without implementing those systems yet
- if new overlays or dirty-settlement paths are needed for zoning feedback, keep them small and typed

Explicit non-goals for this task:
- no road queueing yet
- no construction queue behavior yet
- no farm plot generation yet
- no labor allocation yet
- no logistics, founding, or multi-settlement systems
- no Godot-side painting UI
- no route debug query

Constraints:
- authoritative gameplay state must live only in `sim/`
- the bridge must only convert/expose types and call sim methods
- design for determinism and stable IDs from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/world/ids.hpp`
- a small zoning model such as:
  - `sim/include/alpha/zones/zone_types.hpp`
  - `sim/src/zones/zone_types.cpp`
- updates to:
  - `sim/include/alpha/world/world_state.hpp`
  - `sim/include/alpha/core/simulation.hpp`
  - `sim/src/core/simulation.cpp`
  - `sim/src/api/world_api.cpp`
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `apply_commands()` is implemented in `WorldApi` and exposed through the bridge
- valid zone paint commands update authoritative sim state deterministically
- valid zone removal commands update authoritative sim state deterministically
- dirty chunks, dirty overlays, and dirty settlements are reported consistently
- settlement summary and metrics reflect zoned state correctly
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
- painting a farmland, forestry, or quarry zone on valid cells is accepted
- painting out-of-bounds or illegal cells is rejected with a stable reject reason
- removing previously zoned cells updates settlement summary and metrics correctly
- applying the same valid zoning batch to two identical worlds produces identical results
- accepted zone commands return stable dirty chunk and dirty settlement sets
- save/load after zoning preserves zone state if Task 5 save/load landed fully

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md`
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
