#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "alpha/api/result_types.hpp"

namespace alpha::world {

struct CalendarState {
  YearIndex year = 1;
  MonthIndex month = 1;
};

struct WorldState {
  uint64_t terrain_seed = 0;
  uint64_t gameplay_seed = 0;
  uint16_t map_width = 0;
  uint16_t map_height = 0;
  std::string generation_config_path;
  CalendarState calendar;
  std::vector<ChunkCoord> dirty_chunks;
  uint32_t settlement_count = 0;
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
