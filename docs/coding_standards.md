# Alpha Coding Standards

## Purpose

These rules exist to protect determinism, architectural clarity, and debugging quality.

## Ownership boundaries

- `sim/` owns all authoritative gameplay state and all gameplay rules.
- `extension/` owns type conversion and Godot exposure only.
- `game/` owns scenes, input, widgets, rendering orchestration, and debug presentation.
- `data/` owns tuneable tables, generation settings, and scenario fixtures.

## Hard rules

- Do not place authoritative simulation state in Godot scene nodes.
- Do not put gameplay validation or blocker reasoning in GDScript.
- Do not put gameplay rules in GDExtension wrappers.
- Do not add per-cell Godot nodes for map rendering.
- Do not rely on hash iteration order in outcome-critical logic.
- Do not use floating-point math for gameplay-critical resource and work calculations when fixed-point or integer math is sufficient.

## C++ rules

- Prefer plain structs and free functions over deep inheritance.
- Keep headers small and focused.
- Use explicit strong ID types for durable objects.
- Use stable traversal order whenever outcome order matters.
- Keep hot-path containers simple and data-oriented.
- Use `std::vector` and sorted storage before introducing custom containers.
- Centralize month stepping in the turn pipeline instead of spreading rule logic across arbitrary modules.

## GDScript rules

- GDScript is UI-side only.
- Keep scripts short and single-purpose.
- Use command builders to construct sim mutations rather than ad hoc payload assembly in many widgets.
- Prefer read-model binding and renderer helpers over cross-widget direct coupling.

## Bridge rules

- Keep the Godot bridge thin.
- Methods should mirror the documented simulation API closely.
- Convert Godot types at the edge, then hand off to typed sim structs.
- Expose batched commands and chunk queries, not per-cell imperative methods.

## Determinism rules

- Store resources and work as integer tenths.
- Save RNG state explicitly.
- Sort IDs before serialization.
- Keep turn stepping fixed to one monthly pass per player advance.
- Make cache rebuilds derived-only and safe to discard.

## Data rules

- Tuneable values belong in `data/balance/`, not scattered magic numbers.
- Scenario fixtures belong in `data/scenarios/test/`.
- Generation settings belong in `data/generation/`.
- If a value is implementation-owned tuning, give it one canonical home.

## Save rules

- Save enough state to resume deterministically.
- Do not infer gameplay state from Godot scene reconstruction.
- Rebuild caches and UI state after load.
- Keep binary and JSON debug save field names aligned where possible.

## Testing rules

- Add unit coverage as each sim rule system lands.
- Add scenario coverage for cross-system behavior.
- Maintain deterministic regression fixtures from the first runnable milestone.
- Do not silently rewrite golden outputs when rules change; update them intentionally.

## Review standards

- Review for bugs, determinism risks, save/load gaps, and behavior regressions first.
- Treat missing tests on new core rules as a real defect.
- Reject convenience changes that blur the Godot/sim boundary.
