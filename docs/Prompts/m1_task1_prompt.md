# Alpha Milestone 1 Task 1 Prompt

Implement Milestone 1, Task 1: the minimal sim/bridge spine for Alpha.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/simulation_api.md` is the contract source of truth for types and method names
- `docs/coding_standards.md` is the architecture and determinism guardrail
- `docs/milestone_plan.md` defines the current milestone target and what is intentionally out of scope
- the two handoff docs define the locked product and tech constraints

Objective:
Create the smallest end-to-end native core that proves the architecture:
- a C++20 sim library in `sim/`
- a thin GDExtension bridge in `extension/`
- one minimal `WorldApi` that supports `create_world()`, `advance_month()`, and `get_world_metrics()`
- typed request/result structs aligned with `docs/simulation_api.md`

Scope:
- add the first real C++ headers and sources under `sim/include/alpha/` and `sim/src/`
- add the first bridge files under `extension/include/godot_bridge/` and `extension/src/godot_bridge/`
- update `sim/CMakeLists.txt` and `extension/CMakeLists.txt` so these targets actually build
- implement only the minimum world state needed to create a world and advance one month
- keep the bridge thin and non-authoritative, per `docs/coding_standards.md`

Required behavior:
- `create_world()` succeeds for a valid 1024x1024 world config
- world state stores at minimum: seeds, map dimensions, current year, current month, and enough metrics state for `get_world_metrics()`
- `advance_month()` increments the calendar by one month and returns a basic `TurnReport`
- `get_world_metrics()` returns a valid typed payload
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/simulation_api.md`, then note the issue in the final summary

Explicit non-goals for this task:
- no Godot UI scenes or gameplay UI
- no save/load implementation
- no zoning
- no roads
- no logistics
- no construction
- no population simulation beyond placeholder metrics if needed
- no dynamic dictionary-style API design

Constraints:
- authoritative gameplay state must live only in `sim/`
- GDExtension must only convert/expose types and call sim methods
- design for determinism from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/core/simulation.hpp`
- `sim/include/alpha/world/ids.hpp`
- `sim/include/alpha/world/world_state.hpp`
- matching `.cpp` files in `sim/src/`
- `extension/include/godot_bridge/alpha_world_bridge.hpp`
- `extension/src/godot_bridge/alpha_world_bridge.cpp`
- `extension/src/godot_bridge/register_types.cpp`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `WorldApi` can create a world and advance one month
- the bridge exposes the minimal lifecycle methods cleanly
- the implementation is consistent with:
  - `docs/simulation_api.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build and test surface that exists at this point and report exactly what was run

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md`
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
