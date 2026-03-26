# Alpha Milestone 1 Task 10 Prompt

Implement Milestone 1, Task 10: real Godot binding registration and the launchable macOS shell.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/coding_standards.md`
- `README.md`

Use them proactively while implementing:
- `docs/alpha_tech_stack_handoff.md` defines the Godot + C++ + GDExtension split for Alpha
- `docs/milestone_plan.md` defines the Milestone 1 requirement for a native macOS prototype and native library integration
- `docs/simulation_api.md` is the contract source of truth for the bridge-facing method set
- `docs/coding_standards.md` is the architecture guardrail that keeps gameplay authority out of Godot
- `README.md` documents the current repo layout and baseline workflow expectations

Objective:
Create the next smallest end-to-end proof after Task 9:
- replace the placeholder bridge registration with real `godot-cpp`-backed GDExtension binding registration
- create the first launchable Godot desktop shell under `game/`
- wire the Godot shell to the native bridge without moving simulation authority out of `sim/`

Scope:
- add the real GDExtension registration path through `godot-cpp`
- expose the currently implemented typed bridge methods needed for the Milestone 1 shell
- create the first Godot project/bootstrap files under `game/`
- create an application root scene/script that can initialize the native bridge and create/load a world
- update build scripts so a native macOS launch path exists instead of placeholder-only validation

Required behavior:
- the repository contains a real Godot project configuration for the `game/` shell
- the extension builds as a real GDExtension module rather than only a typed native wrapper
- the Godot app launches on macOS and can successfully call into the native bridge
- the Godot shell can at minimum:
  - initialize the bridge
  - create a new world
  - load a saved world if one is provided
  - advance one month through the bridge
- the bridge remains thin and does not own gameplay validation or authoritative state
- method exposure should stay one-to-one with the typed bridge contract where practical
- if any doc ambiguity appears, prefer the more specific stack guidance in `docs/alpha_tech_stack_handoff.md`, then note the issue in the final summary

Implementation guidance:
- keep the initial shell intentionally plain; Milestone 1 needs a working vertical slice, not polished UX
- do not port gameplay logic into GDScript
- prefer a small app root and thin Godot-facing wrapper classes over a wide Godot script surface
- make the build/export setup practical for local development on macOS
- if exact typed structs require interim Godot conversion wrappers, keep them one-to-one and explicit

Explicit non-goals for this task:
- no full map rendering yet
- no settlement inspector UI yet
- no painting tools yet
- no polished art, audio, or final UX
- no C# integration

Constraints:
- authoritative gameplay state must remain in `sim/`
- GDExtension must only convert/expose types and call sim methods
- preserve the repo/module homes described in `docs/milestone_plan.md`
- keep the implementation intentionally small, clean, and extensible

Suggested first files:
- `extension/include/godot_bridge/alpha_world_bridge.hpp`
- `extension/src/godot_bridge/alpha_world_bridge.cpp`
- `extension/src/godot_bridge/register_types.cpp`
- `extension/CMakeLists.txt`
- `game/project.godot`
- `game/scenes/main/app_root.tscn`
- `game/scripts/app/app_root.gd`
- `game/scripts/data_view/world_bridge.gd`
- updates to:
  - `tools/scripts/build_macos.sh`
  - `tools/scripts/export_macos.sh`

Acceptance criteria:
- the sim target builds
- the extension target builds as a real GDExtension module
- the Godot shell launches natively on macOS
- the Godot shell can create/load a world and advance a month through the native bridge
- the implementation is consistent with:
  - `docs/alpha_tech_stack_handoff.md`
  - `docs/simulation_api.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build, test, and launch surface that exists at this point and report exactly what was run

Recommended tests:
- a local launch smoke test verifies the Godot shell starts and initializes the native bridge
- create-world and advance-month actions succeed through the Godot-facing binding path
- load-world succeeds through the Godot-facing binding path if Task 5 save/load landed fully
- build scripts fail clearly when Godot or `godot-cpp` prerequisites are missing

Final response requirements:
- summarize exactly what was added
- list any deviations from the typed bridge contract or build assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
