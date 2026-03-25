#include <cassert>
#include <cstddef>

#include "alpha/api/world_api.hpp"

namespace {

void assert_chunk_results_equal(const alpha::ChunkVisualResult& left,
                                const alpha::ChunkVisualResult& right) {
  assert(left.chunk.x == right.chunk.x);
  assert(left.chunk.y == right.chunk.y);
  assert(left.width == right.width);
  assert(left.height == right.height);

  for (std::size_t index = 0; index < left.cells.size(); ++index) {
    assert(left.cells[index].terrain_color_index == right.cells[index].terrain_color_index);
    assert(left.cells[index].road_flag == right.cells[index].road_flag);
    assert(left.cells[index].settlement_flag == right.cells[index].settlement_flag);
    assert(left.cells[index].reserved == right.cells[index].reserved);
  }
}

}  // namespace

int main() {
  alpha::WorldApi world_api;

  const alpha::CreateWorldResult invalid_create_result = world_api.create_world({
      .terrain_seed = 11,
      .gameplay_seed = 29,
      .map_width = 512,
      .map_height = 1024,
      .generation_config_path = "data/generation/test_world.json",
  });

  assert(!invalid_create_result.ok);
  assert(!invalid_create_result.error_message.empty());
  assert(invalid_create_result.dirty_chunks.empty());

  const alpha::CreateWorldResult create_result = world_api.create_world({
      .terrain_seed = 11,
      .gameplay_seed = 29,
      .map_width = 1024,
      .map_height = 1024,
      .generation_config_path = "data/generation/test_world.json",
  });

  assert(create_result.ok);
  assert(create_result.error_message.empty());
  assert(create_result.dirty_chunks.size() == 256U);

  const alpha::ChunkVisualResult first_chunk = world_api.get_chunk_visual({
      .chunk = {.x = 0, .y = 0},
      .layer_index = 0,
  });
  assert(first_chunk.chunk.x == 0);
  assert(first_chunk.chunk.y == 0);
  assert(first_chunk.width == alpha::kChunkSize);
  assert(first_chunk.height == alpha::kChunkSize);
  for (const alpha::ChunkVisualCell& cell : first_chunk.cells) {
    assert(cell.road_flag == 0U);
    assert(cell.settlement_flag == 0U);
    assert(cell.reserved == 0U);
  }

  const alpha::ChunkVisualResult repeated_chunk = world_api.get_chunk_visual({
      .chunk = {.x = 0, .y = 0},
      .layer_index = 0,
  });
  assert_chunk_results_equal(first_chunk, repeated_chunk);

  alpha::WorldApi second_world_api;
  const alpha::CreateWorldResult second_create_result = second_world_api.create_world({
      .terrain_seed = 11,
      .gameplay_seed = 29,
      .map_width = 1024,
      .map_height = 1024,
      .generation_config_path = "data/generation/test_world.json",
  });
  assert(second_create_result.ok);

  const alpha::ChunkVisualResult second_world_chunk = second_world_api.get_chunk_visual({
      .chunk = {.x = 0, .y = 0},
      .layer_index = 0,
  });
  assert_chunk_results_equal(first_chunk, second_world_chunk);

  const alpha::ChunkVisualResult invalid_chunk = world_api.get_chunk_visual({
      .chunk = {.x = 16, .y = 0},
      .layer_index = 0,
  });
  assert(invalid_chunk.chunk.x == 16);
  assert(invalid_chunk.chunk.y == 0);
  assert(invalid_chunk.width == 0U);
  assert(invalid_chunk.height == 0U);
  for (const alpha::ChunkVisualCell& cell : invalid_chunk.cells) {
    assert(cell.terrain_color_index == 0U);
    assert(cell.road_flag == 0U);
    assert(cell.settlement_flag == 0U);
    assert(cell.reserved == 0U);
  }

  alpha::WorldMetrics metrics = world_api.get_world_metrics();
  assert(metrics.settlement_count == 0U);
  assert(metrics.zone_count == 0U);
  assert(metrics.plot_count == 0U);
  assert(metrics.project_count == 0U);
  assert(metrics.road_cell_count == 0U);
  assert(metrics.dirty_chunk_count == 256U);

  const alpha::TurnReport first_turn_report = world_api.advance_month();
  assert(first_turn_report.year == 1U);
  assert(first_turn_report.month == 2U);
  assert(first_turn_report.phase_timings.size() == 5U);
  assert(first_turn_report.stockpile_deltas.empty());
  assert(first_turn_report.population_deltas.empty());
  assert(first_turn_report.completed_projects.empty());
  assert(first_turn_report.dirty_chunks.empty());
  assert(first_turn_report.dirty_overlays.empty());

  metrics = world_api.get_world_metrics();
  assert(metrics.dirty_chunk_count == 0U);

  return 0;
}
