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

void assert_overlay_results_equal(const alpha::OverlayChunkResult& left,
                                  const alpha::OverlayChunkResult& right) {
  assert(left.chunk.x == right.chunk.x);
  assert(left.chunk.y == right.chunk.y);
  assert(left.overlay_type == right.overlay_type);
  assert(left.width == right.width);
  assert(left.height == right.height);
  assert(left.legend.size() == right.legend.size());

  for (std::size_t index = 0; index < left.values.size(); ++index) {
    assert(left.values[index] == right.values[index]);
  }

  for (std::size_t index = 0; index < left.legend.size(); ++index) {
    assert(left.legend[index].value_index == right.legend[index].value_index);
    assert(left.legend[index].label == right.legend[index].label);
  }
}

void assert_settlement_summaries_equal(const alpha::SettlementSummary& left,
                                       const alpha::SettlementSummary& right) {
  assert(left.settlement_id == right.settlement_id);
  assert(left.center.x == right.center.x);
  assert(left.center.y == right.center.y);
  assert(left.population_whole == right.population_whole);
  assert(left.food == right.food);
  assert(left.wood == right.wood);
  assert(left.stone == right.stone);
  assert(left.active_zone_count == right.active_zone_count);
  assert(left.active_project_count == right.active_project_count);
  assert(left.food_shortage_flag == right.food_shortage_flag);

  for (std::size_t index = 0; index < left.buildings.size(); ++index) {
    assert(left.buildings[index].building_type == right.buildings[index].building_type);
    assert(left.buildings[index].exists == right.buildings[index].exists);
    assert(left.buildings[index].staffed == right.buildings[index].staffed);
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

  const alpha::OverlayChunkResult fertility_overlay = world_api.get_overlay_chunk({
      .chunk = {.x = 0, .y = 0},
      .overlay_type = alpha::OverlayType::Fertility,
  });
  assert(fertility_overlay.chunk.x == 0);
  assert(fertility_overlay.chunk.y == 0);
  assert(fertility_overlay.overlay_type == alpha::OverlayType::Fertility);
  assert(fertility_overlay.width == alpha::kChunkSize);
  assert(fertility_overlay.height == alpha::kChunkSize);
  assert(fertility_overlay.legend.size() == 5U);
  assert(fertility_overlay.legend[0].value_index == 0U);
  assert(fertility_overlay.legend[0].label == "Barren (0-20)");
  assert(fertility_overlay.legend[1].value_index == 1U);
  assert(fertility_overlay.legend[1].label == "Poor (21-40)");
  assert(fertility_overlay.legend[2].value_index == 2U);
  assert(fertility_overlay.legend[2].label == "Fair (41-60)");
  assert(fertility_overlay.legend[3].value_index == 3U);
  assert(fertility_overlay.legend[3].label == "Good (61-80)");
  assert(fertility_overlay.legend[4].value_index == 4U);
  assert(fertility_overlay.legend[4].label == "Rich (81-100)");
  bool saw_non_zero_overlay_value = false;
  for (const uint8_t value : fertility_overlay.values) {
    assert(value <= 4U);
    saw_non_zero_overlay_value = saw_non_zero_overlay_value || value != 0U;
  }
  assert(saw_non_zero_overlay_value);

  const alpha::OverlayChunkResult repeated_overlay = world_api.get_overlay_chunk({
      .chunk = {.x = 0, .y = 0},
      .overlay_type = alpha::OverlayType::Fertility,
  });
  assert_overlay_results_equal(fertility_overlay, repeated_overlay);

  const alpha::SettlementId starting_settlement_id{1};
  const alpha::SettlementSummary invalid_summary =
      world_api.get_settlement_summary(alpha::SettlementId{999});
  assert(invalid_summary.settlement_id.value == 999U);
  assert(invalid_summary.center.x == 0);
  assert(invalid_summary.center.y == 0);
  assert(invalid_summary.population_whole == 0);
  assert(invalid_summary.food == 0);
  assert(invalid_summary.wood == 0);
  assert(invalid_summary.stone == 0);
  assert(invalid_summary.active_zone_count == 0U);
  assert(invalid_summary.active_project_count == 0U);
  assert(!invalid_summary.food_shortage_flag);
  assert(invalid_summary.buildings.size() == 2U);
  assert(invalid_summary.buildings[0].building_type == alpha::BuildingType::EstateI);
  assert(!invalid_summary.buildings[0].exists);
  assert(!invalid_summary.buildings[0].staffed);
  assert(invalid_summary.buildings[1].building_type == alpha::BuildingType::WarehouseI);
  assert(!invalid_summary.buildings[1].exists);
  assert(!invalid_summary.buildings[1].staffed);

  const alpha::SettlementSummary settlement_summary =
      world_api.get_settlement_summary(starting_settlement_id);
  assert(settlement_summary.settlement_id == starting_settlement_id);
  assert(settlement_summary.center.x >= 0);
  assert(settlement_summary.center.x < 1024);
  assert(settlement_summary.center.y >= 0);
  assert(settlement_summary.center.y < 1024);
  assert(settlement_summary.population_whole == 20);
  assert(settlement_summary.food == 400);
  assert(settlement_summary.wood == 200);
  assert(settlement_summary.stone == 100);
  assert(settlement_summary.active_zone_count == 0U);
  assert(settlement_summary.active_project_count == 0U);
  assert(!settlement_summary.food_shortage_flag);
  assert(settlement_summary.buildings[0].building_type == alpha::BuildingType::EstateI);
  assert(settlement_summary.buildings[0].exists);
  assert(!settlement_summary.buildings[0].staffed);
  assert(settlement_summary.buildings[1].building_type == alpha::BuildingType::WarehouseI);
  assert(settlement_summary.buildings[1].exists);
  assert(!settlement_summary.buildings[1].staffed);

  const alpha::SettlementSummary repeated_settlement_summary =
      world_api.get_settlement_summary(starting_settlement_id);
  assert_settlement_summaries_equal(settlement_summary, repeated_settlement_summary);

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

  const alpha::OverlayChunkResult second_world_overlay = second_world_api.get_overlay_chunk({
      .chunk = {.x = 0, .y = 0},
      .overlay_type = alpha::OverlayType::Fertility,
  });
  assert_overlay_results_equal(fertility_overlay, second_world_overlay);

  const alpha::SettlementSummary second_world_settlement_summary =
      second_world_api.get_settlement_summary(starting_settlement_id);
  assert_settlement_summaries_equal(settlement_summary, second_world_settlement_summary);

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

  const alpha::OverlayChunkResult invalid_overlay_chunk = world_api.get_overlay_chunk({
      .chunk = {.x = 16, .y = 0},
      .overlay_type = alpha::OverlayType::Fertility,
  });
  assert(invalid_overlay_chunk.chunk.x == 16);
  assert(invalid_overlay_chunk.chunk.y == 0);
  assert(invalid_overlay_chunk.overlay_type == alpha::OverlayType::Fertility);
  assert(invalid_overlay_chunk.width == 0U);
  assert(invalid_overlay_chunk.height == 0U);
  assert(invalid_overlay_chunk.legend.empty());
  for (const uint8_t value : invalid_overlay_chunk.values) {
    assert(value == 0U);
  }

  const alpha::OverlayChunkResult unsupported_overlay = world_api.get_overlay_chunk({
      .chunk = {.x = 0, .y = 0},
      .overlay_type = alpha::OverlayType::Slope,
  });
  assert(unsupported_overlay.chunk.x == 0);
  assert(unsupported_overlay.chunk.y == 0);
  assert(unsupported_overlay.overlay_type == alpha::OverlayType::Slope);
  assert(unsupported_overlay.width == 0U);
  assert(unsupported_overlay.height == 0U);
  assert(unsupported_overlay.legend.empty());
  for (const uint8_t value : unsupported_overlay.values) {
    assert(value == 0U);
  }

  alpha::WorldMetrics metrics = world_api.get_world_metrics();
  assert(metrics.settlement_count == 1U);
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
