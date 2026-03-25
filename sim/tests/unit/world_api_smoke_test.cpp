#include <cassert>
#include <cstddef>

#include "alpha/api/world_api.hpp"

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
