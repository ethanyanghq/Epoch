# Alpha Milestone 1 Task 8 Prompt

Implement Milestone 1, Task 8: construction progression for road projects and one building project path.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/simulation_api.md` is the contract source of truth for building queue commands, project views, blocker reporting, and turn report fields
- `docs/alpha_final_implementation_spec.md` defines the Milestone 1 construction and building constraints that apply here
- `docs/coding_standards.md` is the determinism and architecture guardrail
- `docs/milestone_plan.md` defines the one construction queue plus blocker-reporting Milestone 1 scope
- `docs/save_format.md` should stay aligned with any new authoritative project, road, and building state that lands here

Objective:
Create the next smallest end-to-end proof after Task 7:
- support one real building queue command path
- progress queued road projects and one building project path through the monthly sim
- materialize finished roads into authoritative road state with movement-cost benefit
- surface meaningful blocker reasoning for at least one project type

Scope:
- support exactly these additional real paths in this task:
  - `QueueBuildingCommand`
  - real monthly construction progress for queued road projects
  - one real building project path for Milestone 1
- add authoritative built-road state
- add project reservation/progress state needed to explain whether a project is building, blocked, or complete
- update `get_chunk_visual()` and any relevant overlay/query paths so completed roads are visible and affect sim-owned access/movement calculations

Required behavior:
- `QueueBuildingCommand` validates settlement id, building uniqueness rules, and current legality requirements
- accepted building queue commands create deterministic project entries with typed family/priority/status information
- monthly advancement progresses queued projects deterministically
- completed road projects mark built road cells in authoritative world state
- built road cells affect the sim-owned movement/access calculation path used by current Milestone 1 systems
- at least one project type returns explicit blocker reasoning when it cannot progress
- `get_projects()` reflects current required materials/work, reserved amounts, remaining work, priority, status, and blockers consistently with the sim state
- `get_chunk_visual()` reflects finished roads through `road_flag`
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- keep the implementation focused on the first real construction loop rather than the full end-state building roster
- if the current placeholder settlement template must be adjusted to allow a meaningful single building project path, do so deliberately and document the change
- keep project progression deterministic and phase-order-safe
- prefer explicit status/blocker enums or stable strings over ad hoc booleans scattered across the codebase
- ensure road completion updates dirty chunks and any relevant overlays/read models cleanly

Explicit non-goals for this task:
- no multi-building economy
- no inter-settlement transport
- no founding or expansion projects
- no Godot-side project UI yet
- no polished route debug tools

Constraints:
- authoritative gameplay, project, road, and building state must live only in `sim/`
- the bridge must only convert/expose types and call sim methods
- design for determinism, stable IDs, and save/load compatibility from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/core/simulation.hpp`
- `sim/include/alpha/world/world_state.hpp`
- a small construction/project helper such as:
  - `sim/include/alpha/projects/project_progress.hpp`
  - `sim/src/projects/project_progress.cpp`
- updates to:
  - `sim/src/core/simulation.cpp`
  - `sim/src/api/world_api.cpp`
  - `sim/src/map/chunk_visuals.cpp`
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- building queue commands are implemented through `apply_commands()`
- monthly advancement progresses queued road and building projects deterministically
- completed roads become visible through chunk visuals and affect movement/access cost in the current sim path
- project blocker reporting is available for at least one real project type
- `get_projects()` returns meaningful live project data rather than placeholder-only values
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
- queueing a valid building project creates a deterministic project entry
- invalid duplicate or illegal building requests are rejected explicitly
- advancing months progresses queued road work deterministically and eventually marks road cells built
- chunk visuals reflect finished roads after project completion
- blocked projects report stable blocker information
- save/load preserves in-progress and completed projects if Task 5 save/load landed fully

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md` or current settlement-start assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
