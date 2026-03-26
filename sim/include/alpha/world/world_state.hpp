#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "alpha/api/result_types.hpp"
#include "alpha/map/map_grid.hpp"
#include "alpha/projects/project_types.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/zones/zone_types.hpp"

namespace alpha::world {

struct CalendarState {
  YearIndex year = 1;
  MonthIndex month = 1;
};

struct WorldRngState {
  uint64_t seed = 0;
  uint64_t stream_state = 0;
};

struct WorldState {
  uint64_t terrain_seed = 0;
  uint16_t map_width = 0;
  uint16_t map_height = 0;
  std::string generation_config_path;
  CalendarState calendar;
  WorldRngState world_rng;
  map::MapGrid map_grid;
  std::vector<ChunkCoord> dirty_chunks;
  std::vector<settlements::SettlementState> settlements;
  std::vector<zones::ZoneState> zones;
  std::vector<zones::CellZoneState> zone_cells;
  std::vector<projects::ProjectState> projects;
  ZoneId next_zone_id{1};
  ProjectId next_project_id{1};
  uint32_t zone_count = 0;
  uint32_t plot_count = 0;
  uint32_t project_count = 0;
  uint32_t road_cell_count = 0;
};

bool is_supported_world_size(uint16_t width, uint16_t height) noexcept;
std::vector<ChunkCoord> make_all_chunk_coords(uint16_t width, uint16_t height);
void clear_dirty_chunks(WorldState& world_state);
void advance_calendar(CalendarState& calendar) noexcept;
WorldMetrics build_world_metrics(const WorldState& world_state);

}  // namespace alpha::world
