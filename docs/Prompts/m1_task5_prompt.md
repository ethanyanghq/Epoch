# Alpha Milestone 1 Task 5 Prompt

Implement Milestone 1, Task 5: save/load round-tripping for the current native subset.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_final_implementation_spec.md`
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`

Use them proactively while implementing:
- `docs/save_format.md` is the source of truth for save ownership, versioning, and serialization boundaries
- `docs/simulation_api.md` is the contract source of truth for lifecycle method names and payload shapes
- `docs/coding_standards.md` is the determinism and architecture guardrail
- `docs/milestone_plan.md` defines the Milestone 1 exit criteria that require save/load equivalence
- the handoff docs define the repo/module split and the thin-bridge requirement

Objective:
Create the next smallest end-to-end proof after Task 4:
- implement `save_world()` and `load_world()` in the sim and expose them through the bridge
- persist enough authoritative state to reload the current Milestone 1 subset deterministically
- keep save ownership entirely in `sim/`
- rebuild presentation-facing dirty state correctly on load

Scope:
- extend the real C++ implementation under `sim/include/alpha/` and `sim/src/`
- add the typed lifecycle structs required by `docs/simulation_api.md`
- implement the first real serializer/deserializer path for:
  - save header and format version
  - calendar state
  - terrain/map snapshot
  - RNG state needed by the current world generator/sim
  - the one starting settlement and its stockpile/building state
- expose `save_world()` and `load_world()` through `WorldApi` and the bridge
- keep the bridge thin and non-authoritative, per `docs/coding_standards.md`

Required behavior:
- implement `SaveWorldParams`, `SaveWorldResult`, `LoadWorldParams`, and `LoadWorldResult` if they do not already exist in the typed API layer
- implement `save_world(SaveWorldParams)` in `WorldApi` and expose it through the bridge
- implement `load_world(LoadWorldParams)` in `WorldApi` and expose it through the bridge
- a saved world preserves enough state that:
  - `get_chunk_visual()` results match after reload
  - `get_overlay_chunk()` results match after reload
  - `get_settlement_summary()` results match after reload
  - continuing month advancement after reload remains deterministic
- `LoadWorldResult.dirty_chunks` returns the full current chunk set that must be rebuilt after load
- serialization order is stable and deterministic
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/save_format.md`, then note the issue in the final summary

Implementation guidance:
- keep this task intentionally limited to the currently implemented subset rather than the full end-state save surface
- do not serialize derived presentation caches that should be rebuilt on load
- preserve stable IDs and current calendar conventions exactly
- if optional JSON debug export is implemented now, keep it one-to-one with the authoritative binary fields
- do not move save logic into the bridge or Godot-facing code
- use stable traversal and field ordering from the first implemented save format onward

Explicit non-goals for this task:
- no zoning serialization yet unless needed by code that lands in the same task
- no roads serialization yet unless needed by code that lands in the same task
- no project serialization yet unless needed by code that lands in the same task
- no Godot UI save/load screens
- no scenario system
- no broad debug tooling beyond what is needed to verify save/load correctness

Constraints:
- authoritative runtime and save state must live only in `sim/`
- GDExtension must only convert/expose types and call sim methods
- design for deterministic reload behavior from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`

Suggested first files:
- `sim/include/alpha/api/world_api.hpp`
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/core/simulation.hpp`
- `sim/include/alpha/world/world_state.hpp`
- a small save module such as:
  - `sim/include/alpha/save/save_io.hpp`
  - `sim/src/save/save_io.cpp`
- updates to:
  - `sim/src/api/world_api.cpp`
  - `sim/src/core/simulation.cpp`
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `save_world()` is implemented in `WorldApi` and exposed through the bridge
- `load_world()` is implemented in `WorldApi` and exposed through the bridge
- saving and reloading a created world preserves current chunk, overlay, and settlement summary results
- reloading a save and advancing produces the same future results as advancing without interruption
- the implementation is consistent with:
  - `docs/simulation_api.md`
  - `docs/save_format.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build and test surface that exists at this point and report exactly what was run

Recommended tests:
- saving a newly created world succeeds and returns a valid result payload
- loading a just-saved world succeeds and returns the expected format version and dirty chunks
- pre-save and post-load chunk visual results match for the same query
- pre-save and post-load fertility overlay results match for the same query
- pre-save and post-load settlement summary results match for the same settlement id
- saving, loading, and then advancing one month matches the uninterrupted world
- invalid or missing save paths are handled explicitly and documented

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md` or `docs/save_format.md`
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
