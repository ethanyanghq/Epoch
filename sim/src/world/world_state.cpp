#include "alpha/world/world_state.hpp"

#include <cstddef>

#include "alpha/api/constants.hpp"

namespace alpha::world {
namespace {

constexpr uint16_t kSupportedMapWidth = 1024;
constexpr uint16_t kSupportedMapHeight = 1024;

}  // namespace

bool is_supported_world_size(const uint16_t width, const uint16_t height) noexcept {
  return width == kSupportedMapWidth && height == kSupportedMapHeight;
}

std::vector<ChunkCoord> make_all_chunk_coords(const uint16_t width, const uint16_t height) {
  const int32_t chunk_width = (static_cast<int32_t>(width) + kChunkSize - 1) / kChunkSize;
  const int32_t chunk_height = (static_cast<int32_t>(height) + kChunkSize - 1) / kChunkSize;

  std::vector<ChunkCoord> dirty_chunks;
  dirty_chunks.reserve(static_cast<std::size_t>(chunk_width * chunk_height));

  for (int16_t chunk_y = 0; chunk_y < chunk_height; ++chunk_y) {
    for (int16_t chunk_x = 0; chunk_x < chunk_width; ++chunk_x) {
      dirty_chunks.push_back({chunk_x, chunk_y});
    }
  }

  return dirty_chunks;
}

void clear_dirty_chunks(WorldState& world_state) {
  world_state.dirty_chunks.clear();
}

void advance_calendar(CalendarState& calendar) noexcept {
  if (calendar.month >= 12) {
    calendar.month = 1;
    ++calendar.year;
    return;
  }

  ++calendar.month;
}

WorldMetrics build_world_metrics(const WorldState& world_state) {
  return {
      .settlement_count = static_cast<uint32_t>(world_state.settlements.size()),
      .zone_count = world_state.zone_count,
      .plot_count = world_state.plot_count,
      .project_count = world_state.project_count,
      .road_cell_count = world_state.road_cell_count,
      .dirty_chunk_count = static_cast<uint32_t>(world_state.dirty_chunks.size()),
  };
}

}  // namespace alpha::world
