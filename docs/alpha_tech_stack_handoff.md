# Alpha Tech Stack Handoff

## 1. Goal

Build **Alpha** as a **native macOS game** with a lightweight engine shell and a high-performance deterministic simulation core.

This stack is optimized for:
- native Mac shipping
- fast iteration on UI and tools
- deterministic monthly simulation
- good performance on large raster maps
- low engine complexity relative to a full custom engine

---

## 2. Final stack choice

### Recommended stack
- **Engine / app shell:** Godot 4.x
- **Simulation core:** C++20
- **Native integration layer:** Godot **GDExtension**
- **UI / tools / glue:** GDScript
- **Build system for native module:** CMake
- **macOS packaging:** Xcode toolchain for signing and notarization
- **Data/config files:** TOML or JSON
- **Save format:** binary snapshot + optional JSON debug export
- **Version control:** Git
- **CI:** GitHub Actions or equivalent

### Why this stack
This game is not graphics-limited. It is simulation-limited and tooling-limited.

The expensive problems are:
- deterministic world stepping
- zone graph updates
- access/reach/path caching
- logistics
- labor allocation
- chunked map rendering
- clear debug UI

Godot solves the app shell, rendering, UI, and tool-side problems cheaply.
C++ solves the performance-sensitive simulation problems cleanly.

---

## 3. High-level architecture

## 3.1 System split

### Godot owns
- app lifecycle
- menus
- camera
- map interaction
- overlays
- selection
- zoning brush input
- road drawing input
- construction/founding UI
- debug panels
- save/load UI
- turn-advance button and presentation
- audio if added later

### C++ sim owns
- authoritative world state
- map data model
- settlements
- stockpiles
- zone components
- farm plots
- roads data model
- construction projects
- labor allocation
- production
- logistics
- food consumption and starvation
- settlement expansion
- deterministic turn stepping
- access and route caches

### GDScript owns
- screen logic
- editor convenience tools
- command marshaling
- UI formatting
- debug drawing requests
- querying data from the sim and pushing it to widgets

### Hard rule
**Do not put authoritative simulation state into Godot scene nodes.**
The simulation core is the source of truth.

---

## 4. Repository structure

Use a **monorepo**.

```text
alpha/
  README.md
  .gitignore
  docs/
    alpha_final_implementation_spec.md
    alpha_tech_stack_handoff.md
    simulation_api.md
    save_format.md
    coding_standards.md
    milestone_plan.md

  game/
    project.godot
    addons/
    scenes/
      main/
      map/
      ui/
      overlays/
      debug/
    scripts/
      app/
      ui/
      commands/
      debug/
      data_view/
    assets/
      icons/
      fonts/
      placeholder/
    export/
      macos/

  sim/
    CMakeLists.txt
    include/
      alpha/
        api/
        core/
        world/
        map/
        zones/
        settlements/
        logistics/
        labor/
        construction/
        save/
        util/
    src/
      api/
      core/
      world/
      map/
      zones/
      settlements/
      logistics/
      labor/
      construction/
      save/
      util/
    tests/
      unit/
      scenario/
      regression/
    third_party/

  extension/
    CMakeLists.txt
    src/
      godot_bridge/
    include/
      godot_bridge/

  data/
    balance/
      resources/
      buildings/
      labor/
      logistics/
      terrain/
    scenarios/
      test/
    generation/

  tools/
    scripts/
      build_macos.sh
      run_tests.sh
      export_macos.sh
      format.sh
    profiling/
    fixtures/

  ci/
    github/
      workflows/
```

---

## 5. Language boundaries

## 5.1 C++20
Use C++ for anything that is:
- performance-sensitive
- deterministic
- stateful
- large-scale
- core gameplay logic
- hard to test if mixed with UI

That includes:
- map storage
- path cost evaluation
- zone flood fill and component rebuilding
- farm plot partitioning
- settlement simulation
- stockpile logic
- logistics resolution
- labor assignment
- construction progression
- serialization
- monthly turn stepping

## 5.2 GDScript
Use GDScript for:
- menus
- camera controls
- button handlers
- inspector windows
- overlay toggles
- debug panels
- tool modes
- selection behavior
- mouse and keyboard interaction
- reading structured sim query results and formatting them for UI

## 5.3 Do not use GDScript for
- pathfinding
- monthly sim rules
- stockpile math
- production math
- labor allocation
- large grid iteration
- deterministic core data
- save canonicalization

## 5.4 Optional later use of C#
Do **not** introduce C# in milestone 1.
It adds another language boundary without solving the hard problem.

---

## 6. Core runtime contract

The simulation must be treated like a service with a narrow API.

## 6.1 Command flow
Godot sends commands to the sim.
The sim validates and mutates state.
Godot queries read models for display.

### Example command categories
- create new world
- load world
- save world
- zone cells
- remove zone cells
- queue road
- queue building
- queue settlement founding
- set project priority
- advance one month

### Example query categories
- map chunk render data
- terrain overlay data
- zone overlay data
- settlement summary
- settlement stockpile
- settlement labor breakdown
- project list
- route/access overlay
- blocker reasons

## 6.2 Interface style
Prefer **plain typed structs** and small enums over dynamic dictionaries.
Avoid a chatty per-cell API.
Use chunked requests and batched responses.

---

## 7. Simulation module structure

Recommended internal module layout:

```text
sim/include/alpha/
  api/
    world_api.hpp
    command_types.hpp
    query_types.hpp
    result_types.hpp
  core/
    simulation.hpp
    turn_pipeline.hpp
    deterministic_rng.hpp
  map/
    map_grid.hpp
    movement_cost.hpp
    chunk_cache.hpp
  world/
    world_state.hpp
    ids.hpp
  settlements/
    settlement.hpp
    stockpile.hpp
    population.hpp
    expansion.hpp
  zones/
    zone_component.hpp
    zoning.hpp
    farmland.hpp
    forestry.hpp
    quarry.hpp
  labor/
    labor_allocator.hpp
  logistics/
    logistics_solver.hpp
    transport_network.hpp
  construction/
    project.hpp
    project_queue.hpp
    road_builder.hpp
  save/
    serializer.hpp
    deserializer.hpp
  util/
    fixed.hpp
    small_vector.hpp
    bitset_grid.hpp
```

---

## 8. Data design rules

## 8.1 IDs
Every durable gameplay object gets a stable ID:
- settlement id
- zone component id
- farm plot id
- project id
- road segment group id if needed

Do not use scene-tree identity as game identity.

## 8.2 Numeric rules
- Use integer or fixed-point math for gameplay-critical calculations.
- Store resources as integer tenths.
- Keep month stepping deterministic.
- Avoid hidden floating-point drift in core systems.

## 8.3 Iteration rules
Whenever outcome order matters:
- sort explicitly
- use stable containers or stable traversal order
- never rely on hash map iteration order

## 8.4 Save rules
The save format must preserve exact sim state.
The save should not depend on Godot scene reconstruction to infer gameplay state.

---

## 9. Rendering and map presentation

## 9.1 Hard rendering rule
Do **not** spawn one node per cell.
A 1024x1024 grid is too large for that model.

## 9.2 Recommended rendering model
Render the world in **chunks**.

Each chunk should be able to provide:
- base terrain visual
- zoning overlay
- road overlay
- settlement overlay
- debug overlay

Use chunk invalidation and partial redraw.
Rebuild only dirty chunks after commands or month advancement.

## 9.3 Overlay model
Overlays should be data-driven.
Godot requests overlay textures or chunk payloads from the sim-facing bridge layer.

Examples:
- fertility heatmap
- slope heatmap
- movement cost heatmap
- settlement reach overlay
- farmland plot overlay
- logistics shortage overlay
- blocked project overlay

---

## 10. Godot scene structure

Keep it small.

```text
Main
  GameRoot
    WorldViewport
      MapRenderer
      OverlayRenderer
      SelectionRenderer
    UI
      TopBar
      LeftToolPanel
      RightInspectorPanel
      BottomStatusBar
      Modals
    Debug
      DebugPanel
```

### Recommended scene responsibilities
- **Main**: bootstraps app and world session
- **GameRoot**: owns major controllers
- **MapRenderer**: draws chunked terrain and roads
- **OverlayRenderer**: draws selected overlay layer
- **SelectionRenderer**: selection box, hover, brush preview
- **UI**: tool buttons and inspectors
- **DebugPanel**: fps, sim timing, cache timing, object counts, blockers

---

## 11. GDExtension bridge design

The bridge should be very thin.
It should not contain gameplay rules.
It should only:
- convert between Godot types and sim types
- expose commands
- expose queries
- expose lifecycle hooks
- expose performance counters

This section describes the **target bridge shape**.
For exact currently implemented methods, see `docs/implementation_status.md` and the bridge
headers. If a concrete method name here conflicts with `docs/simulation_api.md`, defer to
`docs/simulation_api.md` for the intended final API contract.

### Good bridge API shape
- `create_world(seed, settings)`
- `load_world(path)`
- `save_world(path)`
- `apply_commands(batch)`
- `advance_month()`
- `get_settlement_summary(id)`
- `get_projects_for_settlement(id)`
- `get_chunk_visual(chunk_x, chunk_y, layer)`
- `get_overlay_chunk(chunk_x, chunk_y, overlay_type)`
- `get_last_turn_report()`

### Bad bridge API shape
- `set_cell_fertility(x, y, value)` for normal gameplay
- `get_everything()` giant blob queries every frame
- per-cell GDScript calls in inner loops

---

## 12. Testing strategy

Testing must start immediately.
This project is simulation-heavy and easy to break silently.

## 12.1 Test layers

### Unit tests
For:
- movement cost
- zone legality
- flood fill and component splitting
- farm plot selection order
- labor allocation
- starvation bands
- expansion scoring
- logistics reservation behavior

### Scenario tests
Small handcrafted maps that test:
- founding legality
- road cutting through farmland
- farmland retreat on labor shortage
- settlement expansion consuming farmland
- food shortage and starvation
- import/export bottlenecks

### Regression tests
Given:
- seed
- starting commands
- number of months

Assert exact final state for:
- stockpiles
- population
- projects
- active zones
- active plots

## 12.2 Golden output
Store small deterministic golden snapshots for critical scenarios.
If a rule changes, update intentionally.
Do not let regressions drift silently.

---

## 13. Coding standards

### C++
- small headers
- explicit ownership
- minimal inheritance
- prefer plain structs and functions
- no engine-style object graph inside sim
- avoid premature templates unless clearly useful
- prefer data-oriented layouts for hot paths

### GDScript
- UI only
- no hidden gameplay logic
- short scripts
- one clear owner per screen/widget

### General
- all core rules live in the sim
- all player-visible blocker reasons come from the sim
- all tuning values live in data files, not magic numbers scattered through code

---

## 14. Build and packaging plan for macOS

## 14.1 Development environment
Primary development target:
- Apple Silicon Mac

Support target later if needed:
- Intel Mac builds

## 14.2 Build pipeline
- build C++ sim static or shared library
- build GDExtension bridge against Godot headers
- copy extension artifacts into Godot project
- run Godot project locally
- export signed macOS app bundle
- notarize before distribution

## 14.3 Debugging tools required early
- sim step time
- per-phase month timing
- chunk redraw timing
- cache hit/miss counters
- object counts
- save/load validation command

---

## 15. First milestone

Milestone 1 should **prove the stack** before content breadth.
Do not try to build full Alpha immediately.

## 15.1 Milestone 1 objective
A native macOS prototype that can:
- generate or load a test map
- render chunked terrain
- select a settlement
- paint farmland / forestry / quarry zones
- draw roads
- advance one month
- run the sim in C++ deterministically
- show stockpiles, labor allocation, and project blockers in UI
- save and load successfully

This milestone is about architectural risk reduction, not polish.

## 15.2 Milestone 1 feature scope
Include:
- one map
- one settlement
- local stockpile
- food / wood / stone only
- zoning legality
- roads with movement cost benefit
- monthly labor allocation
- farmland opening / active / fallow flow
- basic forestry and quarry output
- one construction queue with road projects and one building type
- simple turn report panel

Exclude from milestone 1:
- multiple settlements
- inter-settlement logistics
- founding new settlements
- automatic settlement expansion
- final art
- audio
- advanced debug polish

## 15.3 Milestone 1 acceptance criteria
Milestone 1 is done when:
1. The game launches natively on macOS.
2. A test map renders smoothly with chunked drawing.
3. Zoning a region updates correctly and stays performant.
4. Drawing a road changes movement/access cost as expected.
5. Advancing 12 months on a fixed seed gives identical output across runs.
6. Save, quit, reload, and continue produces the same future results.
7. UI can explain why a project is blocked or slow.
8. Sim month timing is inside a reasonable prototype budget.

---

## 16. Recommended milestone sequence after Milestone 1

### Milestone 2
Add:
- second and third settlements
- founding
- inter-settlement logistics
- import/export shortages
- route and transport debugging overlays

### Milestone 3
Add:
- automatic settlement expansion
- Estate I
- Warehouse I
- improved priority controls
- stronger turn reports

### Milestone 4
Add:
- full Alpha balancing pass
- usability pass
- scenario test suite growth
- save compatibility hardening
- shipping-quality Mac export pipeline

---

## 17. Risk register

### Risk 1: too much logic in Godot
Bad outcome:
- sim gets split across UI and native code
- bugs become hard to reproduce

Mitigation:
- hard rule that Godot is not authoritative for gameplay state

### Risk 2: per-cell rendering or scripting overhead
Bad outcome:
- map becomes slow despite simple visuals

Mitigation:
- chunk rendering only
- batched queries only
- no node-per-cell design

### Risk 3: nondeterministic sim
Bad outcome:
- impossible debugging
- save/load divergence

Mitigation:
- fixed-step monthly turns
- stable iteration order
- deterministic tests from day one

### Risk 4: overengineering custom engine pieces
Bad outcome:
- months lost on infrastructure

Mitigation:
- let Godot handle app-shell work
- keep custom work focused on simulation and data flow only

### Risk 5: API too chatty between GDScript and C++
Bad outcome:
- frame hitches and spaghetti glue code

Mitigation:
- batch commands
- chunk queries
- coarse-grained summaries

---

## 18. Immediate next tasks

### Engineering setup
1. Create monorepo with `game/`, `sim/`, `extension/`, `data/`, `docs/`.
2. Create empty Godot desktop app that launches on macOS.
3. Create C++ sim library with a minimal `World` and `advance_month()`.
4. Wire a GDExtension that can create a world and return a simple summary string or struct.
5. Set up CMake build and local build script.

### Rendering setup
6. Implement a chunked map renderer with placeholder terrain colors.
7. Add camera pan/zoom and hovered-cell inspection.
8. Add one overlay toggle for fertility or slope.

### Sim setup
9. Implement map grid storage and movement cost lookup.
10. Implement one settlement with stockpile and population.
11. Implement farmland zone components and legality checks.
12. Implement monthly labor allocation for farmland only.
13. Implement October harvest and November expand/retreat logic.

### UI setup
14. Add a right-side settlement inspector.
15. Add zoning tool buttons.
16. Add an advance-month button and phase/timing report.
17. Add blocker reason text for at least one project type.

### Validation setup
18. Add deterministic scenario tests.
19. Add save/load roundtrip test.
20. Add a profiling/debug panel.

---

## 19. Final instruction to engineering

Do not optimize for theoretical engine purity.
Optimize for getting a deterministic, inspectable, native macOS simulation running fast.

The winning architecture for Alpha is:
- Godot for shell and tools
- C++ for the real game
- narrow bridge between them
- chunked rendering
- deterministic test-first simulation
