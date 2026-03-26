#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "alpha/api/constants.hpp"
#include "alpha/world/ids.hpp"

namespace alpha {

using ResourceAmountTenths = int32_t;
using WorkAmountTenths = int32_t;
using MonthIndex = uint16_t;
using YearIndex = uint16_t;

struct CellCoord {
  int32_t x = 0;
  int32_t y = 0;
};

struct ChunkCoord {
  int16_t x = 0;
  int16_t y = 0;
};

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

struct WorldCreateParams {
  uint64_t terrain_seed = 0;
  uint64_t gameplay_seed = 0;
  uint16_t map_width = 0;
  uint16_t map_height = 0;
  std::string generation_config_path;
};

struct CreateWorldResult {
  bool ok = false;
  std::string error_message;
  std::vector<ChunkCoord> dirty_chunks;
};

struct PhaseTiming {
  std::string phase_name;
  uint32_t duration_ms = 0;
};

struct ResourceDelta {
  SettlementId settlement_id;
  ResourceAmountTenths food_delta = 0;
  ResourceAmountTenths wood_delta = 0;
  ResourceAmountTenths stone_delta = 0;
};

struct PopulationDelta {
  SettlementId settlement_id;
  int32_t whole_delta = 0;
  int32_t fractional_delta_tenths = 0;
  bool starvation_occurred = false;
};

struct TurnReport {
  YearIndex year = 0;
  MonthIndex month = 0;
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

struct ChunkVisualCell {
  uint8_t terrain_color_index = 0;
  uint8_t road_flag = 0;
  uint8_t settlement_flag = 0;
  uint8_t reserved = 0;
};

struct ChunkVisualResult {
  ChunkCoord chunk;
  uint16_t width = 0;
  uint16_t height = 0;
  std::array<ChunkVisualCell, kChunkCellCount> cells;
};

struct OverlayLegendEntry {
  uint8_t value_index = 0;
  std::string label;
};

struct OverlayChunkResult {
  ChunkCoord chunk;
  OverlayType overlay_type = OverlayType::Fertility;
  uint16_t width = 0;
  uint16_t height = 0;
  std::array<uint8_t, kChunkCellCount> values{};
  std::vector<OverlayLegendEntry> legend;
};

struct BuildingStateView {
  BuildingType building_type = BuildingType::EstateI;
  bool exists = false;
  bool staffed = false;
};

struct SettlementSummary {
  SettlementId settlement_id;
  CellCoord center;
  int32_t population_whole = 0;
  ResourceAmountTenths food = 0;
  ResourceAmountTenths wood = 0;
  ResourceAmountTenths stone = 0;
  std::array<BuildingStateView, 2> buildings{};
  uint32_t active_zone_count = 0;
  uint32_t active_project_count = 0;
  bool food_shortage_flag = false;
};

struct WorldMetrics {
  uint32_t settlement_count = 0;
  uint32_t zone_count = 0;
  uint32_t plot_count = 0;
  uint32_t project_count = 0;
  uint32_t road_cell_count = 0;
  uint32_t dirty_chunk_count = 0;
};

}  // namespace alpha
