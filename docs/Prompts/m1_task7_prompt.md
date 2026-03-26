# Alpha Milestone 1 Task 7 Prompt

Implement Milestone 1, Task 7: road queueing and the first project list query.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/simulation_api.md` is the contract source of truth for command payloads, priorities, project read models, and method names
- `docs/alpha_final_implementation_spec.md` defines the Milestone 1 road and construction constraints that apply here
- `docs/coding_standards.md` is the determinism and architecture guardrail
- `docs/milestone_plan.md` defines the road queueing and one-construction-queue Milestone 1 scope
- `docs/save_format.md` should stay aligned with any new authoritative road/project state that lands here

Objective:
Create the next smallest end-to-end proof after Task 6:
- extend `apply_commands()` to support road queueing
- add the first authoritative project model in the sim
- implement `get_projects(ProjectListQuery)` as the first project read path
- keep route legality, project ownership, and blocker reasoning fully sim-owned

Scope:
- support exactly these additional real command/query paths in this task:
  - `QueueRoadCommand`
  - `SetProjectPriorityCommand`
  - `get_projects(ProjectListQuery)`
- add minimal authoritative project storage, stable `ProjectId` allocation, and priority state
- add authoritative queued road route storage needed for Milestone 1 road projects
- return blockers and validation failures through typed result payloads rather than ad hoc strings only

Required behavior:
- `QueueRoadCommand` validates:
  - settlement id exists
  - route cells are in bounds
  - route is non-empty and contiguous under the chosen prototype rule
  - route respects current terrain legality for prototype road placement
- accepted road queue commands create deterministic project entries owned by the requesting settlement
- `SetProjectPriorityCommand` updates existing project priority deterministically and rejects invalid project ids cleanly
- `get_projects(ProjectListQuery)` returns a valid typed `ProjectListResult`
- project views include stable identity, family, priority, status, remaining work placeholders or real values, and blocker text consistent with current sim state
- accepted command batches return new project ids and dirty settlement/chunk information consistently
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- keep this task focused on road queue creation and project read models, not on full monthly construction progress yet
- store route cells in stable order
- choose explicit project status/blocker modeling that can expand into the later construction task without redesign
- keep validation deterministic and avoid bridge-owned path rules
- if movement/access calculations require placeholder fields before real construction progress lands, keep them clearly temporary and sim-owned

Explicit non-goals for this task:
- no completed road construction progression yet unless strictly required for code structure
- no building queue yet
- no route debug query
- no Godot-side road drawing UI
- no labor allocation yet
- no logistics, founding, or multi-settlement systems

Constraints:
- authoritative gameplay, project, and road-queue state must live only in `sim/`
- the bridge must only convert/expose types and call sim methods
- design for determinism and stable IDs from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/api/world_api.hpp`
- a small project model such as:
  - `sim/include/alpha/projects/project_types.hpp`
  - `sim/src/projects/project_types.cpp`
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
- road queue commands are implemented through `apply_commands()`
- `get_projects()` is implemented in `WorldApi` and exposed through the bridge
- valid road queue commands create deterministic project entries
- invalid road routes are rejected with stable reasons/messages
- project priority changes are reflected consistently in `get_projects()` results
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
- queueing a valid road route returns an accepted outcome and a new project id
- queueing an invalid or discontinuous route is rejected explicitly
- querying projects for the starting settlement returns the queued road project
- updating project priority changes the returned project view deterministically
- creating two identical worlds and applying the same road command batch produces identical project results
- save/load preserves queued road projects if Task 5 save/load landed fully

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md`
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
