# Alpha Milestone 1 Task 12 Prompt

Implement Milestone 1, Task 12: zoning and road input tools plus the settlement inspector and turn report UI.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/milestone_plan.md` defines the Milestone 1 requirement for one-settlement interaction, zoning input, road input, settlement summary UI, and turn report visibility
- `docs/simulation_api.md` is the contract source of truth for command batches, settlement read models, project read models, and turn report payloads
- `docs/alpha_final_implementation_spec.md` defines the Milestone 1 settlement, resource, and project expectations
- `docs/alpha_tech_stack_handoff.md` defines the UI/sim split
- `docs/coding_standards.md` keeps gameplay validation in the sim

Objective:
Create the next smallest end-to-end proof after Task 11:
- wire Godot input tools to the typed command batch path for zoning and road drawing
- add a minimal settlement inspector for the currently selected settlement
- add a visible turn report panel for monthly outcomes and phase timing
- expose enough UI to explain stockpiles, labor allocation, and project blockers

Scope:
- implement Godot-side input tools for:
  - paint farmland / forestry / quarry zones
  - remove zoned cells
  - draw road routes and submit them as typed commands
- add a minimal settlement inspector using `get_settlement_summary()` and `get_settlement_detail()`
- add a project panel using `get_projects()`
- add a turn report/profiling panel driven by `advance_month()` results
- add save/load controls if the shell does not already expose them

Required behavior:
- zoning and road tools submit typed command batches through the bridge rather than mutating local presentation state
- command rejection reasons are visible to the player in a minimal but understandable way
- selecting the starting settlement populates a visible inspector showing at least:
  - stockpiles
  - labor allocation or demand
  - current farm plots or equivalent settlement detail
  - active projects and blockers
- advancing one month updates the visible settlement/project state and shows a turn report with phase timings
- the UI can explain why at least one project is blocked or progressing slowly
- save/load controls work through the sim-owned lifecycle if Task 5 save/load landed fully

Implementation guidance:
- keep the UI functional and direct rather than polished
- prefer a small number of clear panels over a wide scattered prototype UI
- do not duplicate gameplay validation in GDScript
- keep Godot-side state as a read-model cache and input orchestrator, not an authority
- if a profiling/debug panel is useful here, keep it focused on real Milestone 1 debugging value

Explicit non-goals for this task:
- no final art pass
- no audio
- no advanced debug polish
- no multi-settlement UI
- no route debug UI unless it is strictly useful for the current Milestone 1 road tool

Constraints:
- authoritative gameplay state must remain in `sim/`
- all legality checks and blocker reasoning must originate in the sim
- preserve the repo/module homes described in `docs/milestone_plan.md`
- keep the implementation intentionally small, clean, and extensible

Suggested first files:
- `game/scenes/ui/settlement_panel.tscn`
- `game/scenes/ui/project_panel.tscn`
- `game/scenes/ui/turn_report_panel.tscn`
- `game/scripts/ui/settlement_panel.gd`
- `game/scripts/ui/project_panel.gd`
- `game/scripts/ui/turn_report_panel.gd`
- `game/scripts/commands/zone_tool.gd`
- `game/scripts/commands/road_tool.gd`
- updates to:
  - `game/scripts/app/app_root.gd`
  - `game/scenes/main/app_root.tscn`
  - `game/scripts/data_view/world_bridge.gd`

Acceptance criteria:
- the Godot shell launches on macOS
- zoning and road drawing input submit typed commands to the sim successfully
- the settlement inspector shows the selected settlement’s stockpiles, labor, and related summary/detail state
- the project panel shows project state and blocker reasoning
- the turn report panel shows visible monthly outcomes and phase timings
- the implementation is consistent with:
  - `docs/milestone_plan.md`
  - `docs/simulation_api.md`
  - `docs/alpha_final_implementation_spec.md`
  - `docs/alpha_tech_stack_handoff.md`
  - `docs/coding_standards.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build, test, launch, map, command-tool, and UI surface that exists at this point and report exactly what was run

Recommended tests:
- a UI smoke path verifies zoning commands and road commands reach the bridge and update the view
- command rejection messages are surfaced for an illegal zone or road request
- selecting a settlement populates summary/detail panels consistently
- advancing a month refreshes the visible inspector, project panel, and turn report
- save/load from the shell preserves visible state if Task 5 save/load landed fully

Final response requirements:
- summarize exactly what was added
- list any deviations from the command/query contract or current UI assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
