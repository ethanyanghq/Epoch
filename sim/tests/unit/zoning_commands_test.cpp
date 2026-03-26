#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

#include "alpha/api/constants.hpp"
#include "alpha/api/world_api.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/save/save_io.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"
#include "alpha/zones/zone_types.hpp"

namespace {

constexpr uint16_t kMapWidth = 1024;
constexpr uint16_t kMapHeight = 1024;
constexpr alpha::SettlementId kSettlementId{1};

alpha::map::MapCell make_base_cell() {
  return {
      .elevation = 100,
      .slope = 0,
      .water = 0,
      .fertility = 50,
      .vegetation = 20,
  };
}

std::vector<alpha::CellCoord> make_rectangle(const int32_t origin_x, const int32_t origin_y,
                                             const int32_t width, const int32_t height) {
  std::vector<alpha::CellCoord> cells;
  cells.reserve(static_cast<std::size_t>(width * height));

  for (int32_t y = origin_y; y < origin_y + height; ++y) {
    for (int32_t x = origin_x; x < origin_x + width; ++x) {
      cells.push_back({.x = x, .y = y});
    }
  }

  return cells;
}

void paint_rectangle(std::vector<alpha::map::MapCell>& cells,
                     const std::vector<alpha::CellCoord>& rectangle,
                     const alpha::map::MapCell& cell_value) {
  for (const alpha::CellCoord& coord : rectangle) {
    const std::size_t index = alpha::map::flatten_cell_index(kMapWidth, coord.x, coord.y);
    cells[index] = cell_value;
  }
}

alpha::world::WorldState make_zoning_test_world_state() {
  alpha::world::WorldState world_state;
  world_state.terrain_seed = 1001;
  world_state.map_width = kMapWidth;
  world_state.map_height = kMapHeight;
  world_state.generation_config_path = "data/generation/zoning_test_world.json";
  world_state.calendar = {
      .year = 1,
      .month = 1,
  };
  world_state.world_rng = {
      .seed = 2002,
      .stream_state = 2002,
  };

  std::vector<alpha::map::MapCell> cells(static_cast<std::size_t>(kMapWidth) *
                                         static_cast<std::size_t>(kMapHeight),
                                         make_base_cell());

  const int32_t center_x = static_cast<int32_t>(kMapWidth) / 2;
  const int32_t center_y = static_cast<int32_t>(kMapHeight) / 2;

  const std::vector<alpha::CellCoord> forestry_cells =
      make_rectangle(center_x + 8, center_y + 3, 4, 2);
  const std::vector<alpha::CellCoord> quarry_cells =
      make_rectangle(center_x + 3, center_y + 8, 4, 2);

  paint_rectangle(cells, forestry_cells,
                  {
                      .elevation = 100,
                      .slope = 1,
                      .water = 0,
                      .fertility = 50,
                      .vegetation = 80,
                  });
  paint_rectangle(cells, quarry_cells,
                  {
                      .elevation = 100,
                      .slope = 2,
                      .water = 0,
                      .fertility = 10,
                      .vegetation = 10,
                  });

  assert(world_state.map_grid.initialize_from_cells(kMapWidth, kMapHeight, std::move(cells)));

  world_state.settlements.push_back(alpha::settlements::make_starting_settlement(world_state.map_grid));
  alpha::zones::initialize_zone_state(world_state);
  world_state.dirty_chunks = alpha::world::make_all_chunk_coords(kMapWidth, kMapHeight);
  return world_state;
}

std::filesystem::path write_test_world_save(const std::string& suffix) {
  const std::filesystem::path save_path =
      std::filesystem::temp_directory_path() / ("alpha_task6_zoning_fixture_" + suffix + ".bin");
  std::filesystem::remove(save_path);

  const alpha::SaveWorldResult save_result = alpha::save::save_world(
      make_zoning_test_world_state(),
      {
          .path = save_path.string(),
      });
  assert(save_result.ok);
  return save_path;
}

alpha::WorldApi load_test_world(const std::filesystem::path& save_path) {
  alpha::WorldApi world_api;
  const alpha::LoadWorldResult load_result = world_api.load_world({
      .path = save_path.string(),
  });
  assert(load_result.ok);
  assert(load_result.format_version == alpha::save::kSaveFormatVersion);
  return world_api;
}

void clear_initial_dirty_chunks(alpha::WorldApi& world_api) {
  const alpha::TurnReport report = world_api.advance_month();
  assert(report.year == 1U);
  assert(report.month == 2U);
  assert(report.dirty_chunks.empty());
}

void assert_batch_results_equal(const alpha::BatchResult& left, const alpha::BatchResult& right) {
  assert(left.outcomes.size() == right.outcomes.size());
  assert(left.dirty_chunks == right.dirty_chunks);
  assert(left.dirty_overlays == right.dirty_overlays);
  assert(left.dirty_settlements == right.dirty_settlements);
  assert(left.new_projects == right.new_projects);

  for (std::size_t index = 0; index < left.outcomes.size(); ++index) {
    assert(left.outcomes[index].accepted == right.outcomes[index].accepted);
    assert(left.outcomes[index].command_index == right.outcomes[index].command_index);
    assert(left.outcomes[index].reject_reason == right.outcomes[index].reject_reason);
    assert(left.outcomes[index].reject_message == right.outcomes[index].reject_message);
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
}

void assert_overlay_results_equal(const alpha::OverlayChunkResult& left,
                                  const alpha::OverlayChunkResult& right) {
  assert(left.chunk == right.chunk);
  assert(left.overlay_type == right.overlay_type);
  assert(left.width == right.width);
  assert(left.height == right.height);
  assert(left.values == right.values);
  assert(left.legend.size() == right.legend.size());
  for (std::size_t index = 0; index < left.legend.size(); ++index) {
    assert(left.legend[index].value_index == right.legend[index].value_index);
    assert(left.legend[index].label == right.legend[index].label);
  }
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream stream(path);
  assert(stream.good());
  return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

int overlay_value_at(const alpha::OverlayChunkResult& overlay, const alpha::CellCoord coord) {
  const int32_t local_x = coord.x % alpha::kChunkSize;
  const int32_t local_y = coord.y % alpha::kChunkSize;
  const std::size_t index = static_cast<std::size_t>(local_y) * alpha::kChunkSize +
                            static_cast<std::size_t>(local_x);
  return overlay.values[index];
}

alpha::CommandBatch make_valid_paint_batch(const alpha::CellCoord center) {
  return {
      .commands =
          {
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Farmland,
                  .cells = make_rectangle(center.x + 3, center.y + 3, 4, 2),
              },
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Forestry,
                  .cells = make_rectangle(center.x + 8, center.y + 3, 4, 2),
              },
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Quarry,
                  .cells = make_rectangle(center.x + 3, center.y + 8, 4, 2),
              },
          },
  };
}

}  // namespace

int main() {
  const std::string test_suffix = std::to_string(static_cast<long long>(::getpid()));
  const std::filesystem::path fixture_path = write_test_world_save(test_suffix);

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const alpha::CommandBatch paint_batch = make_valid_paint_batch(starting_summary.center);
    const alpha::BatchResult paint_result = world_api.apply_commands(paint_batch);

    assert(paint_result.outcomes.size() == 3U);
    for (std::size_t index = 0; index < paint_result.outcomes.size(); ++index) {
      assert(paint_result.outcomes[index].accepted);
      assert(paint_result.outcomes[index].command_index == index);
      assert(paint_result.outcomes[index].reject_reason == alpha::CommandRejectReason::Unknown);
      assert(paint_result.outcomes[index].reject_message.empty());
    }
    assert(paint_result.dirty_chunks.size() == 1U);
    const alpha::ChunkCoord expected_dirty_chunk{
        static_cast<int16_t>((starting_summary.center.x + 3) / alpha::kChunkSize),
        static_cast<int16_t>((starting_summary.center.y + 3) / alpha::kChunkSize),
    };
    assert(paint_result.dirty_chunks[0] == expected_dirty_chunk);
    assert(paint_result.dirty_overlays == std::vector<alpha::OverlayType>{alpha::OverlayType::ZoneOwner});
    assert(paint_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});
    assert(paint_result.new_projects.empty());

    const alpha::SettlementSummary painted_summary = world_api.get_settlement_summary(kSettlementId);
    assert(painted_summary.active_zone_count == 3U);

    const alpha::WorldMetrics painted_metrics = world_api.get_world_metrics();
    assert(painted_metrics.settlement_count == 1U);
    assert(painted_metrics.zone_count == 3U);
    assert(painted_metrics.plot_count == 0U);
    assert(painted_metrics.project_count == 0U);
    assert(painted_metrics.road_cell_count == 0U);
    assert(painted_metrics.dirty_chunk_count == 1U);

    const alpha::OverlayChunkResult zone_overlay = world_api.get_overlay_chunk({
        .chunk = paint_result.dirty_chunks[0],
        .overlay_type = alpha::OverlayType::ZoneOwner,
    });
    assert(zone_overlay.width == alpha::kChunkSize);
    assert(zone_overlay.height == alpha::kChunkSize);
    assert(zone_overlay.legend.size() == 2U);
    assert(zone_overlay.legend[0].label == "Unzoned");
    assert(zone_overlay.legend[1].label == "Settlement 1");
    assert(overlay_value_at(zone_overlay, {starting_summary.center.x + 3, starting_summary.center.y + 3}) == 1);
    assert(overlay_value_at(zone_overlay, {starting_summary.center.x + 8, starting_summary.center.y + 3}) == 1);
    assert(overlay_value_at(zone_overlay, {starting_summary.center.x + 3, starting_summary.center.y + 8}) == 1);
    assert(overlay_value_at(zone_overlay, {starting_summary.center.x + 7, starting_summary.center.y + 7}) == 0);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const alpha::CommandBatch invalid_batch{
        .commands =
            {
                alpha::ZoneCellsCommand{
                    .settlement_id = kSettlementId,
                    .zone_type = alpha::ZoneType::Quarry,
                    .cells = {{.x = starting_summary.center.x + 12, .y = starting_summary.center.y + 3}},
                },
                alpha::ZoneCellsCommand{
                    .settlement_id = alpha::SettlementId{999},
                    .zone_type = alpha::ZoneType::Farmland,
                    .cells = make_rectangle(starting_summary.center.x + 3, starting_summary.center.y + 3, 4, 2),
                },
                alpha::ZoneCellsCommand{
                    .settlement_id = kSettlementId,
                    .zone_type = alpha::ZoneType::Farmland,
                    .cells = {{.x = -1, .y = 0}},
                },
            },
    };

    const alpha::BatchResult invalid_result = world_api.apply_commands(invalid_batch);
    assert(invalid_result.outcomes.size() == 3U);
    assert(!invalid_result.outcomes[0].accepted);
    assert(invalid_result.outcomes[0].reject_reason == alpha::CommandRejectReason::IllegalZoneTarget);
    assert(invalid_result.outcomes[0].reject_message ==
           "ZoneCellsCommand contains cells that are illegal for the requested zone type.");
    assert(!invalid_result.outcomes[1].accepted);
    assert(invalid_result.outcomes[1].reject_reason == alpha::CommandRejectReason::InvalidSettlement);
    assert(invalid_result.outcomes[1].reject_message == "The settlement id does not exist.");
    assert(!invalid_result.outcomes[2].accepted);
    assert(invalid_result.outcomes[2].reject_reason == alpha::CommandRejectReason::InvalidCells);
    assert(invalid_result.outcomes[2].reject_message == "ZoneCellsCommand contains out-of-bounds target cells.");
    assert(invalid_result.dirty_chunks.empty());
    assert(invalid_result.dirty_overlays.empty());
    assert(invalid_result.dirty_settlements.empty());
    assert(invalid_result.new_projects.empty());

    const alpha::SettlementSummary summary = world_api.get_settlement_summary(kSettlementId);
    assert(summary.active_zone_count == 0U);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const alpha::CommandBatch paint_batch = make_valid_paint_batch(starting_summary.center);
    const alpha::BatchResult paint_result = world_api.apply_commands(paint_batch);
    assert(paint_result.outcomes[0].accepted);

    const alpha::BatchResult remove_result = world_api.apply_commands({
        .commands =
            {
                alpha::RemoveZoneCellsCommand{
                    .settlement_id = kSettlementId,
                    .cells = {{.x = starting_summary.center.x + 3, .y = starting_summary.center.y + 3}},
                },
            },
    });
    assert(remove_result.outcomes.size() == 1U);
    assert(remove_result.outcomes[0].accepted);
    assert(remove_result.dirty_chunks == paint_result.dirty_chunks);
    assert(remove_result.dirty_overlays == std::vector<alpha::OverlayType>{alpha::OverlayType::ZoneOwner});
    assert(remove_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});

    const alpha::SettlementSummary removed_summary = world_api.get_settlement_summary(kSettlementId);
    assert(removed_summary.active_zone_count == 2U);

    const alpha::WorldMetrics removed_metrics = world_api.get_world_metrics();
    assert(removed_metrics.zone_count == 2U);
  }

  {
    alpha::WorldApi first_world = load_test_world(fixture_path);
    alpha::WorldApi second_world = load_test_world(fixture_path);
    clear_initial_dirty_chunks(first_world);
    clear_initial_dirty_chunks(second_world);

    const alpha::SettlementSummary first_summary_before = first_world.get_settlement_summary(kSettlementId);
    const alpha::SettlementSummary second_summary_before = second_world.get_settlement_summary(kSettlementId);
    assert_settlement_summaries_equal(first_summary_before, second_summary_before);

    const alpha::CommandBatch paint_batch = make_valid_paint_batch(first_summary_before.center);
    const alpha::BatchResult first_result = first_world.apply_commands(paint_batch);
    const alpha::BatchResult second_result = second_world.apply_commands(paint_batch);
    assert_batch_results_equal(first_result, second_result);

    const alpha::SettlementSummary first_summary_after = first_world.get_settlement_summary(kSettlementId);
    const alpha::SettlementSummary second_summary_after = second_world.get_settlement_summary(kSettlementId);
    assert_settlement_summaries_equal(first_summary_after, second_summary_after);

    const alpha::WorldMetrics first_metrics = first_world.get_world_metrics();
    const alpha::WorldMetrics second_metrics = second_world.get_world_metrics();
    assert(first_metrics.zone_count == second_metrics.zone_count);
    assert(first_metrics.dirty_chunk_count == second_metrics.dirty_chunk_count);

    const alpha::OverlayChunkResult first_overlay = first_world.get_overlay_chunk({
        .chunk = first_result.dirty_chunks[0],
        .overlay_type = alpha::OverlayType::ZoneOwner,
    });
    const alpha::OverlayChunkResult second_overlay = second_world.get_overlay_chunk({
        .chunk = second_result.dirty_chunks[0],
        .overlay_type = alpha::OverlayType::ZoneOwner,
    });
    assert_overlay_results_equal(first_overlay, second_overlay);

    const std::filesystem::path first_save_path =
        std::filesystem::temp_directory_path() / ("alpha_task6_zoning_first_" + test_suffix + ".bin");
    const std::filesystem::path second_save_path =
        std::filesystem::temp_directory_path() / ("alpha_task6_zoning_second_" + test_suffix + ".bin");
    const std::filesystem::path first_json_path = first_save_path.string() + ".json";
    const std::filesystem::path second_json_path = second_save_path.string() + ".json";
    std::filesystem::remove(first_save_path);
    std::filesystem::remove(second_save_path);
    std::filesystem::remove(first_json_path);
    std::filesystem::remove(second_json_path);

    const alpha::SaveWorldResult first_save_result = first_world.save_world({
        .path = first_save_path.string(),
        .write_json_debug_export = true,
    });
    const alpha::SaveWorldResult second_save_result = second_world.save_world({
        .path = second_save_path.string(),
        .write_json_debug_export = true,
    });
    assert(first_save_result.ok);
    assert(second_save_result.ok);
    assert(read_file(first_save_path) == read_file(second_save_path));
    assert(read_file(first_json_path) == read_file(second_json_path));

    alpha::WorldApi reloaded_world;
    const alpha::LoadWorldResult reload_result = reloaded_world.load_world({
        .path = first_save_path.string(),
    });
    assert(reload_result.ok);

    const alpha::SettlementSummary reloaded_summary = reloaded_world.get_settlement_summary(kSettlementId);
    assert_settlement_summaries_equal(first_summary_after, reloaded_summary);

    const alpha::OverlayChunkResult reloaded_overlay = reloaded_world.get_overlay_chunk({
        .chunk = first_result.dirty_chunks[0],
        .overlay_type = alpha::OverlayType::ZoneOwner,
    });
    assert_overlay_results_equal(first_overlay, reloaded_overlay);

    std::filesystem::remove(first_save_path);
    std::filesystem::remove(second_save_path);
    std::filesystem::remove(first_json_path);
    std::filesystem::remove(second_json_path);
  }

  std::filesystem::remove(fixture_path);
  return 0;
}
