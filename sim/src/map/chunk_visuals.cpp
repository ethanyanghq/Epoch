#include "alpha/map/chunk_visuals.hpp"

#include "alpha/api/constants.hpp"

namespace alpha::map {
namespace {

uint8_t terrain_color_index_for_cell(const MapCell& cell) noexcept {
  if (cell.water >= 3) {
    return 0;
  }
  if (cell.water == 2) {
    return 1;
  }
  if (cell.water == 1) {
    return 2;
  }
  if (cell.slope >= 4) {
    return 3;
  }
  if (cell.slope == 3) {
    return 4;
  }
  if (cell.elevation >= 180) {
    return 5;
  }
  if (cell.fertility >= 70 && cell.vegetation >= 60) {
    return 6;
  }
  if (cell.fertility >= 55) {
    return 7;
  }
  if (cell.vegetation >= 70) {
    return 8;
  }
  if (cell.fertility <= 20) {
    return 9;
  }
  return 10;
}

}  // namespace

ChunkVisualResult build_chunk_visual_result(const MapGrid& map_grid, const ChunkVisualQuery& query) {
  ChunkVisualResult result{
      .chunk = query.chunk,
  };

  if (!map_grid.contains_chunk(query.chunk)) {
    return result;
  }

  result.width = kChunkSize;
  result.height = kChunkSize;

  const int32_t origin_x = static_cast<int32_t>(query.chunk.x) * kChunkSize;
  const int32_t origin_y = static_cast<int32_t>(query.chunk.y) * kChunkSize;

  for (int32_t local_y = 0; local_y < kChunkSize; ++local_y) {
    for (int32_t local_x = 0; local_x < kChunkSize; ++local_x) {
      const int32_t world_x = origin_x + local_x;
      const int32_t world_y = origin_y + local_y;
      const MapCell& cell = map_grid.cell(world_x, world_y);

      result.cells[static_cast<std::size_t>(local_y) * kChunkSize + static_cast<std::size_t>(local_x)] =
          ChunkVisualCell{
              .terrain_color_index = terrain_color_index_for_cell(cell),
          };
    }
  }

  return result;
}

}  // namespace alpha::map
