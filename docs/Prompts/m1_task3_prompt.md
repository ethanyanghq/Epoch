# Alpha Milestone 1 Task 3 Prompt

Implement Milestone 1, Task 3: the first overlay chunk query for Alpha.

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
- `docs/alpha_final_implementation_spec.md` defines the allowed per-cell world state and fixed chunk/map assumptions
- `docs/save_format.md` locks the current calendar convention and should stay consistent with runtime state
- the tech handoff defines the repo/module boundaries and the thin-bridge requirement

Objective:
Create the next smallest end-to-end proof after Task 2:
- one minimal `get_overlay_chunk()` query implemented in the sim and exposed through the bridge
- typed overlay query/result structs aligned with `docs/simulation_api.md`
- one deterministic overlay path sourced entirely from sim-owned terrain data
- no gameplay logic in the bridge

Scope:
- extend the real C++ implementation under `sim/include/alpha/` and `sim/src/`
- keep all authoritative overlay source data in `sim/`
- expand the bridge under `extension/include/godot_bridge/` and `extension/src/godot_bridge/`
- update `sim/CMakeLists.txt` and `extension/CMakeLists.txt` if needed
- implement only the minimum overlay query path needed to support one first overlay
- keep the bridge thin and non-authoritative, per `docs/coding_standards.md`

Required behavior:
- implement `OverlayChunkQuery` and `OverlayChunkResult` if they do not already exist in the typed API layer
- chunk overlay queries use the fixed 64x64 chunk contract from `docs/simulation_api.md`
- `get_overlay_chunk(OverlayChunkQuery)` returns a valid typed `OverlayChunkResult`
- support exactly one real overlay in this task:
  - `OverlayType::Fertility`
- `OverlayChunkResult.chunk`, `overlay_type`, `width`, `height`, `values`, and `legend` are populated consistently with the contract
- `OverlayChunkResult.values` for `OverlayType::Fertility` are derived deterministically from sim-owned terrain/map data already stored in `sim/`
- repeated world creation with the same seeds produces identical overlay chunk results for the same query
- method names and struct names should match `docs/simulation_api.md` unless there is a compile-driven reason not to
- if any doc ambiguity appears, prefer the more specific contract in `docs/simulation_api.md`, then note the issue in the final summary

Implementation guidance:
- do not add new authoritative per-cell world state for this task
- derive the fertility overlay from the existing sim-owned fertility field
- keep traversal order stable
- avoid floats for gameplay-critical state
- do not add dynamic dictionary-style payloads
- do not push overlay ownership into the bridge or Godot
- choose a small, explicit legend encoding for fertility and keep it deterministic
- if `godot-cpp` / real GDExtension bindings are still not present in the repo, keep the bridge as a typed native wrapper and do not invent fake engine-owned state

Explicit non-goals for this task:
- no Godot UI scenes or overlay rendering code
- no save/load implementation
- no zoning
- no roads simulation
- no settlement placement gameplay
- no additional overlay types beyond the first one needed here
- no per-cell Godot API methods
- no broad query surface beyond what this task needs

Constraints:
- authoritative gameplay and terrain state must live only in `sim/`
- GDExtension must only convert/expose types and call sim methods
- design for determinism from the start
- keep the implementation intentionally small, clean, and extensible
- follow the repo module homes described in `docs/milestone_plan.md`
- do not add more per-cell state than allowed by `docs/alpha_final_implementation_spec.md`

Suggested first files:
- `sim/include/alpha/api/query_types.hpp`
- `sim/include/alpha/api/result_types.hpp`
- `sim/include/alpha/map/overlay_chunks.hpp` or an equivalent small overlay helper if that fits the current layout better
- matching `.cpp` files in `sim/src/map/`
- updates to:
  - `sim/include/alpha/api/world_api.hpp`
  - `sim/include/alpha/core/simulation.hpp`
  - `sim/include/alpha/world/world_state.hpp` only if strictly needed
  - `extension/include/godot_bridge/alpha_world_bridge.hpp`
  - `extension/src/godot_bridge/alpha_world_bridge.cpp`
- add or extend tests under `sim/tests/unit/`

Acceptance criteria:
- the sim target builds
- the extension target builds
- `get_overlay_chunk()` is implemented in `WorldApi` and exposed through the bridge
- a valid fertility overlay query for a 64x64 chunk returns a correctly shaped typed result
- the returned legend is deterministic and consistent with the value encoding
- two worlds created with the same seeds return identical fertility overlay data for the same query
- the implementation is consistent with:
  - `docs/simulation_api.md`
  - `docs/coding_standards.md`
  - `docs/milestone_plan.md`
  - `docs/alpha_final_implementation_spec.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase landed up to this task, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- at minimum, validate the cumulative build and test surface that exists at this point and report exactly what was run

Recommended tests:
- querying overlay chunk `(0, 0)` for `OverlayType::Fertility` returns `width = 64`, `height = 64`, and a fully populated `values` array
- querying the same fertility chunk twice from the same world returns identical results
- creating two worlds with the same seeds returns identical fertility overlay chunk results
- fertility legend contents are stable and match the produced value range
- if you choose to handle invalid chunk coordinates, test that behavior explicitly and document it
- if unsupported overlay types are handled explicitly for now, test that behavior explicitly and document it

Final response requirements:
- summarize exactly what was added
- list any deviations from `docs/simulation_api.md`
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state the next immediate task after this one
