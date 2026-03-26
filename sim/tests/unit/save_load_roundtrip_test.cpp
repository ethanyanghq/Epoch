#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unistd.h>

#include "alpha/api/world_api.hpp"
#include "alpha/save/save_io.hpp"

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

void assert_turn_reports_equal(const alpha::TurnReport& left, const alpha::TurnReport& right) {
  assert(left.year == right.year);
  assert(left.month == right.month);
  assert(left.phase_timings.size() == right.phase_timings.size());
  assert(left.stockpile_deltas.empty());
  assert(right.stockpile_deltas.empty());
  assert(left.population_deltas.empty());
  assert(right.population_deltas.empty());
  assert(left.completed_projects.empty());
  assert(right.completed_projects.empty());
  assert(left.newly_blocked_projects.empty());
  assert(right.newly_blocked_projects.empty());
  assert(left.newly_unblocked_projects.empty());
  assert(right.newly_unblocked_projects.empty());
  assert(left.shortage_settlements.empty());
  assert(right.shortage_settlements.empty());
  assert(left.dirty_chunks.empty());
  assert(right.dirty_chunks.empty());
  assert(left.dirty_overlays.empty());
  assert(right.dirty_overlays.empty());

  for (std::size_t index = 0; index < left.phase_timings.size(); ++index) {
    assert(left.phase_timings[index].phase_name == right.phase_timings[index].phase_name);
    assert(left.phase_timings[index].duration_ms == right.phase_timings[index].duration_ms);
  }
}

}  // namespace

int main() {
  const std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
  const std::string test_suffix = std::to_string(static_cast<long long>(::getpid()));
  const std::filesystem::path missing_dir = temp_dir / ("alpha_task5_missing_dir_" + test_suffix);
  const std::filesystem::path save_path = temp_dir / ("alpha_task5_roundtrip_" + test_suffix + ".bin");
  const std::filesystem::path json_path = save_path.string() + ".json";
  const std::filesystem::path missing_load_path = temp_dir / ("alpha_task5_missing_" + test_suffix + ".bin");

  std::filesystem::remove(save_path);
  std::filesystem::remove(json_path);
  std::filesystem::remove(missing_load_path);
  std::filesystem::remove_all(missing_dir);

  alpha::WorldApi world_api;
  const alpha::CreateWorldResult create_result = world_api.create_world({
      .terrain_seed = 11,
      .gameplay_seed = 29,
      .map_width = 1024,
      .map_height = 1024,
      .generation_config_path = "data/generation/test_world.json",
  });
  assert(create_result.ok);

  const alpha::SaveWorldResult invalid_save_result =
      world_api.save_world({.path = (missing_dir / "missing_parent.bin").string()});
  assert(!invalid_save_result.ok);
  assert(!invalid_save_result.error_message.empty());

  const alpha::LoadWorldResult missing_load_result =
      world_api.load_world({.path = missing_load_path.string()});
  assert(!missing_load_result.ok);
  assert(!missing_load_result.error_message.empty());
  assert(missing_load_result.format_version == 0U);
  assert(missing_load_result.dirty_chunks.empty());

  const alpha::TurnReport before_save_turn_report = world_api.advance_month();
  assert(before_save_turn_report.year == 1U);
  assert(before_save_turn_report.month == 2U);

  const alpha::ChunkVisualQuery chunk_query{
      .chunk = {.x = 3, .y = 4},
      .layer_index = 0,
  };
  const alpha::OverlayChunkQuery overlay_query{
      .chunk = {.x = 3, .y = 4},
      .overlay_type = alpha::OverlayType::Fertility,
  };
  const alpha::SettlementId starting_settlement_id{1};

  const alpha::ChunkVisualResult pre_save_chunk = world_api.get_chunk_visual(chunk_query);
  const alpha::OverlayChunkResult pre_save_overlay = world_api.get_overlay_chunk(overlay_query);
  const alpha::SettlementSummary pre_save_summary =
      world_api.get_settlement_summary(starting_settlement_id);

  const alpha::SaveWorldResult save_result = world_api.save_world({
      .path = save_path.string(),
      .write_json_debug_export = true,
  });
  assert(save_result.ok);
  assert(save_result.error_message.empty());
  assert(save_result.json_debug_path == json_path.string());
  assert(std::filesystem::exists(save_path));
  assert(std::filesystem::exists(json_path));
  assert(std::filesystem::file_size(save_path) > 0U);
  assert(std::filesystem::file_size(json_path) > 0U);

  alpha::WorldApi loaded_world_api;
  const alpha::LoadWorldResult load_result = loaded_world_api.load_world({
      .path = save_path.string(),
  });
  assert(load_result.ok);
  assert(load_result.error_message.empty());
  assert(load_result.format_version == alpha::save::kSaveFormatVersion);
  assert(load_result.dirty_chunks.size() == 256U);

  const alpha::WorldMetrics loaded_metrics = loaded_world_api.get_world_metrics();
  assert(loaded_metrics.settlement_count == 1U);
  assert(loaded_metrics.zone_count == 0U);
  assert(loaded_metrics.plot_count == 0U);
  assert(loaded_metrics.project_count == 0U);
  assert(loaded_metrics.road_cell_count == 0U);
  assert(loaded_metrics.dirty_chunk_count == 256U);

  const alpha::ChunkVisualResult post_load_chunk = loaded_world_api.get_chunk_visual(chunk_query);
  const alpha::OverlayChunkResult post_load_overlay = loaded_world_api.get_overlay_chunk(overlay_query);
  const alpha::SettlementSummary post_load_summary =
      loaded_world_api.get_settlement_summary(starting_settlement_id);
  assert_chunk_results_equal(pre_save_chunk, post_load_chunk);
  assert_overlay_results_equal(pre_save_overlay, post_load_overlay);
  assert_settlement_summaries_equal(pre_save_summary, post_load_summary);

  const alpha::TurnReport continued_turn_report = world_api.advance_month();
  const alpha::TurnReport resumed_turn_report = loaded_world_api.advance_month();
  assert_turn_reports_equal(continued_turn_report, resumed_turn_report);

  const alpha::ChunkVisualResult continued_chunk = world_api.get_chunk_visual(chunk_query);
  const alpha::ChunkVisualResult resumed_chunk = loaded_world_api.get_chunk_visual(chunk_query);
  const alpha::OverlayChunkResult continued_overlay = world_api.get_overlay_chunk(overlay_query);
  const alpha::OverlayChunkResult resumed_overlay = loaded_world_api.get_overlay_chunk(overlay_query);
  const alpha::SettlementSummary continued_summary =
      world_api.get_settlement_summary(starting_settlement_id);
  const alpha::SettlementSummary resumed_summary =
      loaded_world_api.get_settlement_summary(starting_settlement_id);
  assert_chunk_results_equal(continued_chunk, resumed_chunk);
  assert_overlay_results_equal(continued_overlay, resumed_overlay);
  assert_settlement_summaries_equal(continued_summary, resumed_summary);

  std::filesystem::remove(save_path);
  std::filesystem::remove(json_path);
  std::filesystem::remove_all(missing_dir);
  return 0;
}
