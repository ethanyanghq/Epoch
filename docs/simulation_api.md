# Alpha Simulation API

## Purpose

This document defines the canonical contract between the Godot shell, the GDExtension bridge, and the C++ simulation core.

The simulation is treated as a service with a narrow typed API:
- Godot sends commands
- the sim validates and mutates state
- Godot requests read models for rendering and UI

## Design rules

- Use plain typed structs and small enums rather than dynamic map-shaped payloads.
- Batch mutations and chunk read requests.
- Avoid per-cell API calls in gameplay flows.
- All gameplay validation and blocker reasons originate in the sim.
- The bridge converts types only and does not own gameplay logic.

## Core types

### Scalar aliases

- `ResourceAmountTenths = int32_t`
- `WorkAmountTenths = int32_t`
- `MonthIndex = uint16_t`
- `YearIndex = uint16_t`

### Coordinates

```cpp
struct CellCoord {
  int32_t x;
  int32_t y;
};

struct ChunkCoord {
  int16_t x;
  int16_t y;
};
```

### Stable IDs

```cpp
struct SettlementId { uint32_t value; };
struct ZoneId { uint32_t value; };
struct FarmPlotId { uint32_t value; };
struct ProjectId { uint32_t value; };
```

### Enums

```cpp
enum class PriorityLabel : uint8_t {
  Required,
  High,
  Normal,
  Low,
  Paused
};

enum class ZoneType : uint8_t {
  Farmland,
  Forestry,
  Quarry
};

enum class ProjectFamily : uint8_t {
  Road,
  Building,
  Founding,
  Expansion
};

enum class BuildingType : uint8_t {
  EstateI,
  WarehouseI
};

enum class OverlayType : uint8_t {
  Fertility,
  Slope,
  MovementCost,
  ZoneOwner,
  FarmlandPlots,
  Reach,
  LogisticsShortage,
  ProjectBlockers
};
```

## World lifecycle

### Create world

```cpp
struct WorldCreateParams {
  uint64_t terrain_seed;
  uint64_t gameplay_seed;
  uint16_t map_width;
  uint16_t map_height;
  std::string generation_config_path;
};

struct CreateWorldResult {
  bool ok;
  std::string error_message;
  std::vector<ChunkCoord> dirty_chunks;
};
```

Rules:
- Milestone 1 supports only `1024 x 1024` world creation.
- A newly created world starts at `year = 1`, `month = 1`.
- `MonthIndex` is **1-based** and valid values are `1` through `12`.
- On successful creation, `dirty_chunks` contains the current full set of chunks that must be rebuilt from sim state before first presentation.

### Load world

```cpp
struct LoadWorldParams {
  std::string path;
};

struct LoadWorldResult {
  bool ok;
  std::string error_message;
  uint32_t format_version;
  std::vector<ChunkCoord> dirty_chunks;
};
```

### Save world

```cpp
struct SaveWorldParams {
  std::string path;
  bool write_json_debug_export;
};

struct SaveWorldResult {
  bool ok;
  std::string error_message;
  std::string json_debug_path;
};
```

## Commands

Commands are batched so input tools can submit one consistent mutation set per user action.

### Command payloads

```cpp
struct ZoneCellsCommand {
  SettlementId settlement_id;
  ZoneType zone_type;
  std::vector<CellCoord> cells;
};

struct RemoveZoneCellsCommand {
  SettlementId settlement_id;
  std::vector<CellCoord> cells;
};

struct QueueRoadCommand {
  SettlementId settlement_id;
  std::vector<CellCoord> route_cells;
  PriorityLabel priority;
};

struct QueueBuildingCommand {
  SettlementId settlement_id;
  BuildingType building_type;
  PriorityLabel priority;
};

struct QueueFoundingCommand {
  SettlementId source_settlement_id;
  CellCoord target_center;
  PriorityLabel priority;
};

struct SetProjectPriorityCommand {
  ProjectId project_id;
  PriorityLabel priority;
};
```

### Batch interface

```cpp
using CommandVariant = std::variant<
  ZoneCellsCommand,
  RemoveZoneCellsCommand,
  QueueRoadCommand,
  QueueBuildingCommand,
  QueueFoundingCommand,
  SetProjectPriorityCommand
>;

struct CommandBatch {
  std::vector<CommandVariant> commands;
};
```

### Validation results

```cpp
enum class CommandRejectReason : uint16_t {
  InvalidSettlement,
  InvalidCells,
  IllegalZoneTarget,
  IllegalRoadRoute,
  IllegalBuildingRequest,
  IllegalFoundingTarget,
  MissingResources,
  DuplicateUniqueBuilding,
  InvalidProject,
  InvalidPriority,
  Unknown
};

struct CommandOutcome {
  bool accepted;
  uint32_t command_index;
  CommandRejectReason reject_reason;
  std::string reject_message;
};

struct BatchResult {
  std::vector<CommandOutcome> outcomes;
  std::vector<ChunkCoord> dirty_chunks;
  std::vector<OverlayType> dirty_overlays;
  std::vector<SettlementId> dirty_settlements;
  std::vector<ProjectId> new_projects;
};
```

## Turn advancement

```cpp
struct PhaseTiming {
  std::string phase_name;
  uint32_t duration_ms;
};

struct ResourceDelta {
  SettlementId settlement_id;
  ResourceAmountTenths food_delta;
  ResourceAmountTenths wood_delta;
  ResourceAmountTenths stone_delta;
};

struct PopulationDelta {
  SettlementId settlement_id;
  int32_t whole_delta;
  int32_t fractional_delta_tenths;
  bool starvation_occurred;
};

struct TurnReport {
  YearIndex year;
  MonthIndex month;
  std::vector<PhaseTiming> phase_timings;
  std::vector<ResourceDelta> stockpile_deltas;
  std::vector<PopulationDelta> population_deltas;
  std::vector<ProjectId> completed_projects;
  std::vector<ProjectId> newly_blocked_projects;
  std::vector<ProjectId> newly_unblocked_projects;
  std::vector<SettlementId> shortage_settlements;
  std::vector<ChunkCoord> dirty_chunks;
  std::vector<OverlayType> dirty_overlays;
};
```

Rules:
- `advance_month()` performs exactly one deterministic monthly pass.
- `TurnReport.year` and `TurnReport.month` report the **post-advance** calendar value.
- If a world begins at `year = 1`, `month = 1`, the first successful `advance_month()` returns `year = 1`, `month = 2`.
- Dirty collections in the `TurnReport` are the chunks and overlays newly dirtied by that monthly pass.

## Queries

### Chunk visuals

Chunk queries operate on fixed 64x64 cells.

```cpp
struct ChunkVisualQuery {
  ChunkCoord chunk;
  uint8_t layer_index;
};

struct ChunkVisualCell {
  uint8_t terrain_color_index;
  uint8_t road_flag;
  uint8_t settlement_flag;
  uint8_t reserved;
};

struct ChunkVisualResult {
  ChunkCoord chunk;
  uint16_t width;
  uint16_t height;
  std::array<ChunkVisualCell, 64 * 64> cells;
};
```

### Overlay chunks

```cpp
struct OverlayChunkQuery {
  ChunkCoord chunk;
  OverlayType overlay_type;
};

struct OverlayLegendEntry {
  uint8_t value_index;
  std::string label;
};

struct OverlayChunkResult {
  ChunkCoord chunk;
  OverlayType overlay_type;
  uint16_t width;
  uint16_t height;
  std::array<uint8_t, 64 * 64> values;
  std::vector<OverlayLegendEntry> legend;
};
```

### Settlement summaries

```cpp
struct BuildingStateView {
  BuildingType building_type;
  bool exists;
  bool staffed;
};

struct SettlementSummary {
  SettlementId settlement_id;
  CellCoord center;
  int32_t population_whole;
  ResourceAmountTenths food;
  ResourceAmountTenths wood;
  ResourceAmountTenths stone;
  std::array<BuildingStateView, 2> buildings;
  uint32_t active_zone_count;
  uint32_t active_project_count;
  bool food_shortage_flag;
};
```

### Settlement details

```cpp
struct RoleFillView {
  int32_t serfs;
  int32_t artisans;
  int32_t nobles;
};

struct LaborDemandView {
  int32_t protected_base;
  int32_t extra_roles;
  int32_t idle;
};

struct FarmPlotView {
  FarmPlotId plot_id;
  ZoneId parent_zone_id;
  uint16_t size;
  uint16_t avg_fertility_tenths;
  uint16_t avg_access_cost_tenths;
  uint16_t labor_coverage_tenths;
  std::string state_name;
};

struct SettlementDetail {
  SettlementSummary summary;
  RoleFillView role_fill;
  LaborDemandView labor_demand;
  std::vector<FarmPlotView> farm_plots;
  uint32_t transport_capacity;
  uint32_t development_pressure_tenths;
};
```

### Projects

```cpp
struct ProjectView {
  ProjectId project_id;
  SettlementId owner_settlement_id;
  ProjectFamily family;
  std::string type_name;
  PriorityLabel priority;
  std::string status_name;
  ResourceAmountTenths required_food;
  ResourceAmountTenths required_wood;
  ResourceAmountTenths required_stone;
  ResourceAmountTenths reserved_food;
  ResourceAmountTenths reserved_wood;
  ResourceAmountTenths reserved_stone;
  WorkAmountTenths remaining_common_work;
  WorkAmountTenths remaining_skilled_work;
  int32_t access_modifier_tenths;
  std::vector<std::string> blockers;
};

struct ProjectListQuery {
  SettlementId settlement_id;
};

struct ProjectListResult {
  std::vector<ProjectView> projects;
};
```

### Route debug

```cpp
struct RouteDebugQuery {
  SettlementId source_settlement_id;
  SettlementId target_settlement_id;
};

struct RouteDebugResult {
  bool reachable;
  int32_t route_cost_tenths;
  uint32_t transport_points_required;
  std::vector<CellCoord> route_cells;
  std::vector<std::string> notes;
};
```

### Metrics

```cpp
struct WorldMetrics {
  uint32_t settlement_count;
  uint32_t zone_count;
  uint32_t plot_count;
  uint32_t project_count;
  uint32_t road_cell_count;
  uint32_t dirty_chunk_count;
};
```

Rules:
- `get_world_metrics()` returns the current live sim-state counts.
- `dirty_chunk_count` is the current size of the sim-owned dirty chunk set at the time of the query, not a historical counter.

## Godot-facing bridge methods

The bridge should expose these methods directly or with one-to-one wrappers:

- `create_world(WorldCreateParams)`
- `load_world(LoadWorldParams)`
- `save_world(SaveWorldParams)`
- `apply_commands(CommandBatch)`
- `advance_month()`
- `get_chunk_visual(ChunkVisualQuery)`
- `get_overlay_chunk(OverlayChunkQuery)`
- `get_settlement_summary(SettlementId)`
- `get_settlement_detail(SettlementId)`
- `get_projects(ProjectListQuery)`
- `get_route_debug(RouteDebugQuery)`
- `get_world_metrics()`

## Versioning

- The API is end-state-first and may be implemented incrementally.
- Unused query fields may return default values until the owning milestone lands.
- Method names and core payload shapes should remain stable once introduced.
- If a bridge method is documented but the Godot binding dependency is not yet present, it is acceptable to land the sim-facing typed wrapper first and add the final GDExtension registration path when the binding is available.
