# Alpha Save Format

## Purpose

This document defines the **target end-state save boundary** for Alpha.

This save format is part of the final Alpha design. It is not fully implemented in the current
codebase yet. For the current implementation subset, see `docs/implementation_status.md`.

The save format must preserve enough simulation state to resume deterministically for the same future inputs.

## Save principles

- Save authoritative gameplay state only.
- Rebuild caches, dirty-region trackers, and presentation state on load.
- Preserve stable object IDs.
- Preserve hidden fractional accumulators and RNG state.
- Serialize in stable order.
- Support a binary canonical save with an optional JSON debug export.

## Save artifacts

- Canonical runtime save: binary snapshot
- Optional debug save: JSON export with the same logical fields

## Top-level layout

```text
SaveHeader
CalendarState
MapSnapshot
WorldRngState
SettlementSnapshot[]
ZoneSnapshot[]
FarmPlotSnapshot[]
RoadSnapshot
ProjectSnapshot[]
```

## Save header

```cpp
struct SaveHeader {
  uint32_t format_version;
  uint32_t ruleset_version;
  uint32_t generator_version;
  uint16_t map_width;
  uint16_t map_height;
};
```

Rules:
- `format_version` is required from the first implemented save.
- `ruleset_version` tracks gameplay-table compatibility.
- `generator_version` tracks terrain-generation compatibility.

## Calendar state

```cpp
struct CalendarState {
  uint16_t year;
  uint16_t month_index;
};
```

Rules:
- `year` and `month_index` store the current committed world calendar state.
- `month_index` is **1-based** and valid values are `1` through `12`.
- A newly created world starts at `year = 1`, `month_index = 1`.

## Map snapshot

The save stores both the generation inputs and the realized terrain arrays.

```cpp
struct MapSnapshot {
  uint64_t terrain_seed;
  std::string generation_config_id;
  std::vector<uint8_t> elevation;
  std::vector<uint8_t> slope;
  std::vector<uint8_t> water;
  std::vector<uint8_t> fertility;
  std::vector<uint8_t> vegetation;
};
```

Rules:
- Arrays are serialized in row-major order.
- Array length must equal `map_width * map_height`.
- Saving the full realized terrain prevents drift if world-generation tuning changes later.

## World RNG state

```cpp
struct WorldRngState {
  uint64_t seed;
  uint64_t stream_state;
};
```

## Settlement snapshot

```cpp
struct BuildingSnapshot {
  uint8_t building_type;
  bool exists;
  bool staffed;
};

struct LaborStateSnapshot {
  int32_t last_serf_fill;
  int32_t last_artisan_fill;
  int32_t last_noble_fill;
  int32_t reassignment_budget_tenths;
  int32_t protected_food_floor_serfs;
};

struct StockpileSnapshot {
  int32_t food_tenths;
  int32_t wood_tenths;
  int32_t stone_tenths;
};

struct SettlementSnapshot {
  uint32_t id;
  int32_t center_x;
  int32_t center_y;
  std::vector<uint32_t> footprint_cell_indices;
  int32_t population_whole;
  int32_t population_fraction_tenths;
  int32_t development_pressure_tenths;
  StockpileSnapshot stockpile;
  std::array<BuildingSnapshot, 2> buildings;
  LaborStateSnapshot labor_state;
  uint32_t founding_source_settlement_id;
  bool has_founding_source;
};
```

Rules:
- Footprint cells are serialized as flattened row-major cell indices.
- Building slots are fixed to Estate I and Warehouse I.
- Founding source is present only while the settlement is still being founded.

## Zone snapshot

```cpp
struct ZoneSnapshot {
  uint32_t id;
  uint32_t owner_settlement_id;
  uint8_t zone_type;
  std::vector<uint32_t> member_cell_indices;
};
```

Rules:
- Member cells are sorted ascending.
- Zones are serialized in ascending `ZoneId` order.

## Farm plot snapshot

```cpp
struct FarmPlotSnapshot {
  uint32_t id;
  uint32_t parent_zone_id;
  std::vector<uint32_t> cell_indices;
  uint8_t state;
  uint16_t avg_fertility_tenths;
  uint16_t avg_access_cost_tenths;
  bool forested_flag;
  uint16_t current_year_labor_coverage_tenths;
};
```

Rules:
- Plot identity must survive unaffected edits when possible.
- Plots are serialized in ascending `FarmPlotId` order.

## Road snapshot

```cpp
struct RoadSnapshot {
  std::vector<uint32_t> road_cell_indices;
};
```

Rules:
- Road persistence is per-cell in Alpha.
- Grouped-road persistence is deferred until the product needs it.

## Project snapshot

```cpp
struct ProjectResourceSnapshot {
  int32_t food_tenths;
  int32_t wood_tenths;
  int32_t stone_tenths;
};

struct ProjectSnapshot {
  uint32_t id;
  uint32_t owner_settlement_id;
  uint8_t family;
  uint8_t type;
  int32_t target_x;
  int32_t target_y;
  std::vector<uint32_t> route_cell_indices;
  uint8_t priority;
  uint8_t status;
  ProjectResourceSnapshot required_materials;
  ProjectResourceSnapshot reserved_materials;
  ProjectResourceSnapshot consumed_materials;
  int32_t remaining_common_work_tenths;
  int32_t remaining_skilled_work_tenths;
  int32_t access_modifier_tenths;
  std::vector<uint16_t> blocker_codes;
};
```

Rules:
- Projects are serialized in ascending `ProjectId` order.
- Founding, road, building, and expansion projects share one storage shape.

## Not saved

Do not save:
- chunk caches
- reach caches
- path caches
- dirty chunk flags
- transient overlay buffers
- Godot selection state
- Godot camera state
- open panel state

These are reconstructed on load.

## Load invariants

On load, the runtime must:
1. validate header dimensions and versions
2. rebuild world state from saved arrays and object snapshots
3. restore stable IDs
4. rebuild caches from authoritative data
5. mark all visible chunks dirty for redraw

## JSON debug export

The optional JSON debug export should:
- mirror the binary logical structure
- use explicit field names instead of positional tuples
- keep enum values readable by name where practical
- remain stable enough for snapshot diffing in tests

## Compatibility policy

- Before Milestone 5, backward compatibility is best-effort only.
- After Milestone 5, format changes must increment `format_version` and provide a migration or a deliberate compatibility break note.
