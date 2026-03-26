#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>

#include "alpha/api/world_api.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/save/save_io.hpp"
#include "alpha/settlements/farm_plots.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"
#include "alpha/zones/zone_types.hpp"

namespace {

constexpr alpha::SettlementId kSettlementId{1};
constexpr uint16_t kMapWidth = 1024;
constexpr uint16_t kMapHeight = 1024;

alpha::CommandBatch make_paint_batch(const alpha::CellCoord center);

void assert_turn_reports_equal(const alpha::TurnReport& left, const alpha::TurnReport& right) {
  assert(left.year == right.year);
  assert(left.month == right.month);
  assert(left.phase_timings.size() == right.phase_timings.size());
  assert(left.stockpile_deltas.size() == right.stockpile_deltas.size());
  assert(left.population_deltas.size() == right.population_deltas.size());
  assert(left.completed_projects == right.completed_projects);
  assert(left.newly_blocked_projects == right.newly_blocked_projects);
  assert(left.newly_unblocked_projects == right.newly_unblocked_projects);
  assert(left.shortage_settlements == right.shortage_settlements);
  assert(left.dirty_chunks == right.dirty_chunks);
  assert(left.dirty_overlays == right.dirty_overlays);

  for (std::size_t index = 0; index < left.phase_timings.size(); ++index) {
    assert(left.phase_timings[index].phase_name == right.phase_timings[index].phase_name);
    assert(left.phase_timings[index].duration_ms == right.phase_timings[index].duration_ms);
  }

  for (std::size_t index = 0; index < left.stockpile_deltas.size(); ++index) {
    assert(left.stockpile_deltas[index].settlement_id == right.stockpile_deltas[index].settlement_id);
    assert(left.stockpile_deltas[index].food_delta == right.stockpile_deltas[index].food_delta);
    assert(left.stockpile_deltas[index].wood_delta == right.stockpile_deltas[index].wood_delta);
    assert(left.stockpile_deltas[index].stone_delta == right.stockpile_deltas[index].stone_delta);
  }

  for (std::size_t index = 0; index < left.population_deltas.size(); ++index) {
    assert(left.population_deltas[index].settlement_id ==
           right.population_deltas[index].settlement_id);
    assert(left.population_deltas[index].whole_delta == right.population_deltas[index].whole_delta);
    assert(left.population_deltas[index].fractional_delta_tenths ==
           right.population_deltas[index].fractional_delta_tenths);
    assert(left.population_deltas[index].starvation_occurred ==
           right.population_deltas[index].starvation_occurred);
  }
}

void assert_settlement_details_equal(const alpha::SettlementDetail& left,
                                     const alpha::SettlementDetail& right) {
  assert(left.summary.settlement_id == right.summary.settlement_id);
  assert(left.summary.center == right.summary.center);
  assert(left.summary.population_whole == right.summary.population_whole);
  assert(left.summary.food == right.summary.food);
  assert(left.summary.wood == right.summary.wood);
  assert(left.summary.stone == right.summary.stone);
  assert(left.summary.food_shortage_flag == right.summary.food_shortage_flag);
  assert(left.role_fill.serfs == right.role_fill.serfs);
  assert(left.role_fill.artisans == right.role_fill.artisans);
  assert(left.role_fill.nobles == right.role_fill.nobles);
  assert(left.labor_demand.protected_base == right.labor_demand.protected_base);
  assert(left.labor_demand.extra_roles == right.labor_demand.extra_roles);
  assert(left.labor_demand.idle == right.labor_demand.idle);
  assert(left.transport_capacity == right.transport_capacity);
  assert(left.development_pressure_tenths == right.development_pressure_tenths);
  assert(left.farm_plots.size() == right.farm_plots.size());

  for (std::size_t index = 0; index < left.summary.buildings.size(); ++index) {
    assert(left.summary.buildings[index].building_type == right.summary.buildings[index].building_type);
    assert(left.summary.buildings[index].exists == right.summary.buildings[index].exists);
    assert(left.summary.buildings[index].staffed == right.summary.buildings[index].staffed);
  }

  for (std::size_t index = 0; index < left.farm_plots.size(); ++index) {
    assert(left.farm_plots[index].plot_id == right.farm_plots[index].plot_id);
    assert(left.farm_plots[index].parent_zone_id == right.farm_plots[index].parent_zone_id);
    assert(left.farm_plots[index].size == right.farm_plots[index].size);
    assert(left.farm_plots[index].avg_fertility_tenths ==
           right.farm_plots[index].avg_fertility_tenths);
    assert(left.farm_plots[index].avg_access_cost_tenths ==
           right.farm_plots[index].avg_access_cost_tenths);
    assert(left.farm_plots[index].labor_coverage_tenths ==
           right.farm_plots[index].labor_coverage_tenths);
    assert(left.farm_plots[index].state_name == right.farm_plots[index].state_name);
  }
}

alpha::map::MapCell make_base_cell() {
  return {
      .elevation = 100,
      .slope = 0,
      .water = 0,
      .fertility = 50,
      .vegetation = 20,
  };
}

alpha::world::WorldState make_test_world_state() {
  alpha::world::WorldState world_state;
  world_state.terrain_seed = 5001;
  world_state.map_width = kMapWidth;
  world_state.map_height = kMapHeight;
  world_state.generation_config_path = "data/generation/monthly_sim_test_world.json";
  world_state.calendar = {
      .year = 1,
      .month = 1,
  };
  world_state.world_rng = {
      .seed = 6002,
      .stream_state = 6002,
  };

  std::vector<alpha::map::MapCell> cells(static_cast<std::size_t>(kMapWidth) *
                                             static_cast<std::size_t>(kMapHeight),
                                         make_base_cell());
  const int32_t center_x = static_cast<int32_t>(kMapWidth) / 2;
  const int32_t center_y = static_cast<int32_t>(kMapHeight) / 2;

  for (int32_t y = center_y + 3; y < center_y + 6; ++y) {
    for (int32_t x = center_x + 3; x < center_x + 7; ++x) {
      cells[alpha::map::flatten_cell_index(kMapWidth, x, y)] = {
          .elevation = 100,
          .slope = 0,
          .water = 0,
          .fertility = 80,
          .vegetation = 20,
      };
    }
  }

  for (int32_t y = center_y + 3; y < center_y + 5; ++y) {
    for (int32_t x = center_x + 8; x < center_x + 12; ++x) {
      cells[alpha::map::flatten_cell_index(kMapWidth, x, y)] = {
          .elevation = 100,
          .slope = 1,
          .water = 0,
          .fertility = 40,
          .vegetation = 80,
      };
    }
  }

  for (int32_t y = center_y + 8; y < center_y + 10; ++y) {
    for (int32_t x = center_x + 3; x < center_x + 7; ++x) {
      cells[alpha::map::flatten_cell_index(kMapWidth, x, y)] = {
          .elevation = 100,
          .slope = 2,
          .water = 0,
          .fertility = 10,
          .vegetation = 10,
      };
    }
  }

  assert(world_state.map_grid.initialize_from_cells(kMapWidth, kMapHeight, std::move(cells)));
  world_state.road_cells.assign(static_cast<std::size_t>(kMapWidth) * static_cast<std::size_t>(kMapHeight),
                                0U);
  world_state.settlements.push_back(alpha::settlements::make_starting_settlement(world_state.map_grid));
  alpha::zones::initialize_zone_state(world_state);
  world_state.dirty_chunks = alpha::world::make_all_chunk_coords(kMapWidth, kMapHeight);
  return world_state;
}

std::filesystem::path write_fixture(const std::string& suffix) {
  const std::filesystem::path save_path =
      std::filesystem::temp_directory_path() / ("alpha_task9_monthly_fixture_" + suffix + ".bin");
  std::filesystem::remove(save_path);
  const alpha::SaveWorldResult save_result =
      alpha::save::save_world(make_test_world_state(), {.path = save_path.string()});
  assert(save_result.ok);
  return save_path;
}

std::filesystem::path write_fallow_fixture(const std::string& suffix) {
  alpha::world::WorldState world_state = make_test_world_state();
  const alpha::SettlementSummary summary =
      alpha::settlements::build_settlement_summary(world_state.settlements.front());
  const alpha::BatchResult batch_result = alpha::zones::apply_commands(
      world_state, make_paint_batch(summary.center));
  assert(batch_result.outcomes.size() == 3U);
  for (const alpha::CommandOutcome& outcome : batch_result.outcomes) {
    assert(outcome.accepted);
  }

  alpha::settlements::rebuild_world_farm_plots(world_state);
  assert(world_state.farm_plots.size() == 1U);
  world_state.farm_plots[0].state = alpha::settlements::FarmPlotStateCode::Fallow;
  world_state.farm_plots[0].opening_months_remaining = 0;
  world_state.farm_plots[0].opening_work_remaining_tenths = 0;
  world_state.calendar.month = 1;
  world_state.dirty_chunks = alpha::world::make_all_chunk_coords(kMapWidth, kMapHeight);

  const std::filesystem::path save_path =
      std::filesystem::temp_directory_path() / ("alpha_task9_fallow_fixture_" + suffix + ".bin");
  std::filesystem::remove(save_path);
  const alpha::SaveWorldResult save_result =
      alpha::save::save_world(world_state, {.path = save_path.string()});
  assert(save_result.ok);
  return save_path;
}

alpha::WorldApi load_world(const std::filesystem::path& save_path) {
  alpha::WorldApi world_api;
  const alpha::LoadWorldResult load_result = world_api.load_world({.path = save_path.string()});
  assert(load_result.ok);
  assert(load_result.format_version == alpha::save::kSaveFormatVersion);
  return world_api;
}

alpha::CommandBatch make_paint_batch(const alpha::CellCoord center) {
  return {
      .commands =
          {
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Farmland,
                  .cells =
                      {
                          {.x = center.x + 3, .y = center.y + 3},
                          {.x = center.x + 4, .y = center.y + 3},
                          {.x = center.x + 5, .y = center.y + 3},
                          {.x = center.x + 6, .y = center.y + 3},
                          {.x = center.x + 3, .y = center.y + 4},
                          {.x = center.x + 4, .y = center.y + 4},
                          {.x = center.x + 5, .y = center.y + 4},
                          {.x = center.x + 6, .y = center.y + 4},
                          {.x = center.x + 3, .y = center.y + 5},
                          {.x = center.x + 4, .y = center.y + 5},
                          {.x = center.x + 5, .y = center.y + 5},
                          {.x = center.x + 6, .y = center.y + 5},
                      },
              },
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Forestry,
                  .cells =
                      {
                          {.x = center.x + 8, .y = center.y + 3},
                          {.x = center.x + 9, .y = center.y + 3},
                          {.x = center.x + 10, .y = center.y + 3},
                          {.x = center.x + 11, .y = center.y + 3},
                          {.x = center.x + 8, .y = center.y + 4},
                          {.x = center.x + 9, .y = center.y + 4},
                          {.x = center.x + 10, .y = center.y + 4},
                          {.x = center.x + 11, .y = center.y + 4},
                      },
              },
              alpha::ZoneCellsCommand{
                  .settlement_id = kSettlementId,
                  .zone_type = alpha::ZoneType::Quarry,
                  .cells =
                      {
                          {.x = center.x + 3, .y = center.y + 8},
                          {.x = center.x + 4, .y = center.y + 8},
                          {.x = center.x + 5, .y = center.y + 8},
                          {.x = center.x + 6, .y = center.y + 8},
                          {.x = center.x + 3, .y = center.y + 9},
                          {.x = center.x + 4, .y = center.y + 9},
                          {.x = center.x + 5, .y = center.y + 9},
                          {.x = center.x + 6, .y = center.y + 9},
                      },
              },
          },
  };
}

}  // namespace

int main() {
  const std::string test_suffix = std::to_string(static_cast<long long>(::getpid()));
  const std::filesystem::path fixture_path = write_fixture(test_suffix);
  const std::filesystem::path fallow_fixture_path = write_fallow_fixture(test_suffix);

  {
    alpha::WorldApi first_world = load_world(fixture_path);
    alpha::WorldApi second_world = load_world(fixture_path);

    const alpha::SettlementSummary first_summary = first_world.get_settlement_summary(kSettlementId);
    const alpha::SettlementSummary second_summary = second_world.get_settlement_summary(kSettlementId);
    assert(first_summary.center == second_summary.center);

    const alpha::CommandBatch paint_batch = make_paint_batch(first_summary.center);
    const alpha::BatchResult first_batch = first_world.apply_commands(paint_batch);
    const alpha::BatchResult second_batch = second_world.apply_commands(paint_batch);
    assert(first_batch.outcomes.size() == second_batch.outcomes.size());
    for (std::size_t index = 0; index < first_batch.outcomes.size(); ++index) {
      assert(first_batch.outcomes[index].accepted == second_batch.outcomes[index].accepted);
      assert(first_batch.outcomes[index].command_index == second_batch.outcomes[index].command_index);
      assert(first_batch.outcomes[index].reject_reason == second_batch.outcomes[index].reject_reason);
      assert(first_batch.outcomes[index].reject_message == second_batch.outcomes[index].reject_message);
    }
    assert(first_batch.dirty_chunks == second_batch.dirty_chunks);
    assert(first_batch.dirty_overlays == second_batch.dirty_overlays);
    assert(first_batch.dirty_settlements == second_batch.dirty_settlements);

    for (int month = 0; month < 12; ++month) {
      const alpha::TurnReport first_report = first_world.advance_month();
      const alpha::TurnReport second_report = second_world.advance_month();
      assert_turn_reports_equal(first_report, second_report);
    }

    const alpha::SettlementDetail first_detail = first_world.get_settlement_detail(kSettlementId);
    const alpha::SettlementDetail second_detail = second_world.get_settlement_detail(kSettlementId);
    assert_settlement_details_equal(first_detail, second_detail);
    assert(first_detail.summary.active_zone_count == 3U);
    assert(first_detail.farm_plots.size() == 1U);
    assert(first_detail.farm_plots[0].state_name == "Active");
    assert(first_detail.role_fill.nobles == 1);
  }

  {
    alpha::WorldApi world_api = load_world(fixture_path);
    alpha::TurnReport last_report;
    for (int month = 0; month < 21; ++month) {
      last_report = world_api.advance_month();
    }

    const alpha::SettlementSummary summary = world_api.get_settlement_summary(kSettlementId);
    assert(summary.food_shortage_flag);
    assert(last_report.shortage_settlements == std::vector<alpha::SettlementId>{kSettlementId});
    assert(!last_report.population_deltas.empty());
    assert(last_report.population_deltas[0].starvation_occurred);
  }

  {
    alpha::WorldApi world_api = load_world(fallow_fixture_path);
    alpha::SettlementDetail detail = world_api.get_settlement_detail(kSettlementId);
    assert(detail.farm_plots.size() == 1U);
    assert(detail.farm_plots[0].state_name == "Fallow");

    for (int month = 0; month < 11; ++month) {
      world_api.advance_month();
    }

    detail = world_api.get_settlement_detail(kSettlementId);
    assert(detail.farm_plots.size() == 1U);
    assert(detail.farm_plots[0].state_name == "Opening");

    world_api.advance_month();
    detail = world_api.get_settlement_detail(kSettlementId);
    assert(detail.farm_plots[0].state_name == "Active");
  }

  std::filesystem::remove(fixture_path);
  std::filesystem::remove(fallow_fixture_path);

  return 0;
}
