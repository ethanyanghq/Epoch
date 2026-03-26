# Alpha Milestone 1 Task 11 Prompt

Implement Milestone 1, Task 11: chunked map rendering, camera controls, hover inspection, and overlay viewing in Godot.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/milestone_plan.md` defines the Milestone 1 requirement for chunked terrain rendering, camera pan/zoom, and one overlay path
- `docs/simulation_api.md` is the contract source of truth for chunk and overlay query shapes
- `docs/alpha_final_implementation_spec.md` defines the fixed map/chunk assumptions
- `docs/alpha_tech_stack_handoff.md` defines the intended Godot/sim split
- `docs/coding_standards.md` keeps render/UI logic separate from authoritative gameplay state

Objective:
Create the next smallest end-to-end proof after Task 10:
- render the map in Godot through chunk queries rather than per-cell scene nodes
- add camera pan/zoom and hovered-cell inspection
- add the first overlay viewing path
- support settlement selection from the map view

Scope:
- implement a chunk-driven terrain rendering path in `game/`
- query chunk visuals from the native bridge and rebuild only dirty chunks when practical
- add camera controls appropriate for the Milestone 1 prototype
- add hovered-cell inspection using sim-owned world coordinates
- support toggling between base terrain and the first overlay path already implemented in the sim
- add settlement selection from the rendered map

Required behavior:
- the map renders through `get_chunk_visual()` results rather than one scene node per cell
- map rendering supports at least the fixed Milestone 1 world/chunk assumptions cleanly
- camera pan/zoom works on desktop input
- hovered-cell inspection reports meaningful cell/location information to the UI layer
- overlay viewing uses `get_overlay_chunk()` rather than Godot-owned derived terrain state
- clicking or selecting the starting settlement updates a Godot-side selected-settlement state without moving authority out of the sim
- if dirty chunk or dirty overlay information is available from prior tasks, the renderer uses it to refresh affected regions rather than rebuilding the entire map each frame

Implementation guidance:
- keep the rendering path intentionally practical and prototype-friendly
- avoid per-cell nodes and avoid hiding heavy rebuild work behind opaque helper code
- if a full chunk-cache system is needed, keep it explicit and small
- keep coordinate conversion logic centralized and testable
- the Godot shell may keep transient presentation caches, but gameplay truth must remain sim-owned

Explicit non-goals for this task:
- no zone painting input yet
- no road drawing input yet
- no full settlement inspector yet
- no polished visual effects or final art
- no simulation logic in GDScript

Constraints:
- authoritative gameplay state must remain in `sim/`
- Godot should request typed read models rather than reconstructing gameplay data locally
- preserve the repo/module homes described in `docs/milestone_plan.md`
- keep the implementation intentionally small, clean, and extensible

Suggested first files:
- `game/scenes/map/world_map.tscn`
- `game/scripts/app/app_root.gd`
- `game/scripts/data_view/chunk_cache.gd`
- `game/scripts/data_view/world_bridge.gd`
- `game/scripts/ui/hover_panel.gd`
- `game/scripts/ui/overlay_toggle.gd`
- updates to:
  - `game/scenes/main/app_root.tscn`
  - `game/scenes/overlays/`
  - `game/scenes/ui/`

Acceptance criteria:
- the Godot shell launches on macOS
- the map renders through chunk queries rather than per-cell scene nodes
- camera pan/zoom and hovered-cell inspection work in the prototype
- the first overlay path is viewable in the map
- the starting settlement can be selected from the map
- the implementation is consistent with:
  - `docs/milestone_plan.md`
  - `docs/simulation_api.md`
  - `docs/alpha_final_implementation_spec.md`
  - `docs/alpha_tech_stack_handoff.md`
  - `docs/coding_standards.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build, test, launch, and map-interaction surface that exists at this point and report exactly what was run

Recommended tests:
- a Godot-facing smoke path verifies that chunk queries can populate the initial map view
- selecting the starting settlement updates the current selected settlement id consistently
- overlay toggling requests overlay chunks and refreshes the expected rendered regions
- camera/hover coordinate conversion is validated with a small automated or scripted check where practical

Final response requirements:
- summarize exactly what was added
- list any deviations from the chunk/overlay contract or current rendering assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
