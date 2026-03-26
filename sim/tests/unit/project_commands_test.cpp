#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
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

alpha::world::WorldState make_project_test_world_state() {
  alpha::world::WorldState world_state;
  world_state.terrain_seed = 3001;
  world_state.map_width = kMapWidth;
  world_state.map_height = kMapHeight;
  world_state.generation_config_path = "data/generation/project_test_world.json";
  world_state.calendar = {
      .year = 1,
      .month = 1,
  };
  world_state.world_rng = {
      .seed = 4002,
      .stream_state = 4002,
  };

  std::vector<alpha::map::MapCell> cells(static_cast<std::size_t>(kMapWidth) *
                                             static_cast<std::size_t>(kMapHeight),
                                         make_base_cell());

  const int32_t center_x = static_cast<int32_t>(kMapWidth) / 2;
  const int32_t center_y = static_cast<int32_t>(kMapHeight) / 2;
  cells[alpha::map::flatten_cell_index(kMapWidth, center_x + 8, center_y)] = {
      .elevation = 100,
      .slope = 4,
      .water = 0,
      .fertility = 50,
      .vegetation = 20,
  };

  assert(world_state.map_grid.initialize_from_cells(kMapWidth, kMapHeight, std::move(cells)));

  world_state.settlements.push_back(alpha::settlements::make_starting_settlement(world_state.map_grid));
  alpha::zones::initialize_zone_state(world_state);
  world_state.next_project_id = {1};
  world_state.project_count = 0;
  world_state.road_cell_count = 0;
  world_state.dirty_chunks = alpha::world::make_all_chunk_coords(kMapWidth, kMapHeight);
  return world_state;
}

std::filesystem::path write_test_world_save(const std::string& suffix) {
  const std::filesystem::path save_path =
      std::filesystem::temp_directory_path() / ("alpha_task7_project_fixture_" + suffix + ".bin");
  std::filesystem::remove(save_path);

  const alpha::SaveWorldResult save_result = alpha::save::save_world(
      make_project_test_world_state(),
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

std::vector<alpha::CellCoord> make_route(const alpha::CellCoord start, const int32_t length) {
  std::vector<alpha::CellCoord> route;
  route.reserve(static_cast<std::size_t>(length));
  for (int32_t step = 0; step < length; ++step) {
    route.push_back({.x = start.x + step, .y = start.y});
  }
  return route;
}

void assert_project_lists_equal(const alpha::ProjectListResult& left,
                                const alpha::ProjectListResult& right) {
  assert(left.projects.size() == right.projects.size());

  for (std::size_t index = 0; index < left.projects.size(); ++index) {
    const alpha::ProjectView& lhs = left.projects[index];
    const alpha::ProjectView& rhs = right.projects[index];
    assert(lhs.project_id == rhs.project_id);
    assert(lhs.owner_settlement_id == rhs.owner_settlement_id);
    assert(lhs.family == rhs.family);
    assert(lhs.type_name == rhs.type_name);
    assert(lhs.priority == rhs.priority);
    assert(lhs.status == rhs.status);
    assert(lhs.status_name == rhs.status_name);
    assert(lhs.required_food == rhs.required_food);
    assert(lhs.required_wood == rhs.required_wood);
    assert(lhs.required_stone == rhs.required_stone);
    assert(lhs.reserved_food == rhs.reserved_food);
    assert(lhs.reserved_wood == rhs.reserved_wood);
    assert(lhs.reserved_stone == rhs.reserved_stone);
    assert(lhs.remaining_common_work == rhs.remaining_common_work);
    assert(lhs.remaining_skilled_work == rhs.remaining_skilled_work);
    assert(lhs.access_modifier_tenths == rhs.access_modifier_tenths);
    assert(lhs.blocker_codes == rhs.blocker_codes);
    assert(lhs.blockers == rhs.blockers);
  }
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

}  // namespace

int main() {
  const std::string test_suffix = std::to_string(static_cast<long long>(::getpid()));
  const std::filesystem::path fixture_path = write_test_world_save(test_suffix);

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const std::vector<alpha::CellCoord> valid_route =
        make_route({.x = starting_summary.center.x + 3, .y = starting_summary.center.y}, 4);

    const alpha::BatchResult queue_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells = valid_route,
                    .priority = alpha::PriorityLabel::High,
                },
            },
    });

    assert(queue_result.outcomes.size() == 1U);
    assert(queue_result.outcomes[0].accepted);
    assert(queue_result.outcomes[0].command_index == 0U);
    assert(queue_result.outcomes[0].reject_reason == alpha::CommandRejectReason::Unknown);
    assert(queue_result.outcomes[0].reject_message.empty());
    assert(queue_result.new_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{1}});
    const std::vector<alpha::ChunkCoord> expected_dirty_chunks{
        alpha::ChunkCoord{
            static_cast<int16_t>(valid_route.front().x / alpha::kChunkSize),
            static_cast<int16_t>(valid_route.front().y / alpha::kChunkSize),
        },
    };
    assert(queue_result.dirty_chunks == expected_dirty_chunks);
    assert(queue_result.dirty_overlays ==
           std::vector<alpha::OverlayType>{alpha::OverlayType::ProjectBlockers});
    assert(queue_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});

    const alpha::SettlementSummary queued_summary = world_api.get_settlement_summary(kSettlementId);
    assert(queued_summary.active_project_count == 1U);

    const alpha::ProjectListResult projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    assert(projects.projects.size() == 1U);
    assert(projects.projects[0].project_id == alpha::ProjectId{1});
    assert(projects.projects[0].owner_settlement_id == kSettlementId);
    assert(projects.projects[0].family == alpha::ProjectFamily::Road);
    assert(projects.projects[0].type_name == "Prototype Road");
    assert(projects.projects[0].priority == alpha::PriorityLabel::High);
    assert(projects.projects[0].status == alpha::ProjectStatus::Blocked);
    assert(projects.projects[0].status_name == "Blocked");
    assert(projects.projects[0].remaining_common_work == 40);
    assert(projects.projects[0].remaining_skilled_work == 0);
    const std::vector<alpha::ProjectBlockerCode> expected_blocker_codes{
        alpha::ProjectBlockerCode::WaitingForConstructionSystem,
    };
    assert(projects.projects[0].blocker_codes == expected_blocker_codes);
    const std::vector<std::string> expected_blockers{
        "Construction progression is not implemented for queued projects yet.",
    };
    assert(projects.projects[0].blockers == expected_blockers);

    const alpha::WorldMetrics metrics = world_api.get_world_metrics();
    assert(metrics.project_count == 1U);
    assert(metrics.road_cell_count == 0U);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const alpha::BatchResult invalid_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells =
                        {
                            {.x = starting_summary.center.x + 3, .y = starting_summary.center.y},
                            {.x = starting_summary.center.x + 5, .y = starting_summary.center.y},
                        },
                    .priority = alpha::PriorityLabel::Normal,
                },
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells =
                        {
                            {.x = starting_summary.center.x + 8, .y = starting_summary.center.y},
                        },
                    .priority = alpha::PriorityLabel::Normal,
                },
                alpha::SetProjectPriorityCommand{
                    .project_id = alpha::ProjectId{999},
                    .priority = alpha::PriorityLabel::Low,
                },
            },
    });

    assert(invalid_result.outcomes.size() == 3U);
    assert(!invalid_result.outcomes[0].accepted);
    assert(invalid_result.outcomes[0].reject_reason == alpha::CommandRejectReason::IllegalRoadRoute);
    assert(invalid_result.outcomes[0].reject_message ==
           "QueueRoadCommand route cells must form a contiguous 4-neighbor path.");
    assert(!invalid_result.outcomes[1].accepted);
    assert(invalid_result.outcomes[1].reject_reason == alpha::CommandRejectReason::IllegalRoadRoute);
    assert(invalid_result.outcomes[1].reject_message ==
           "QueueRoadCommand route cells contain terrain that is illegal for prototype road placement.");
    assert(!invalid_result.outcomes[2].accepted);
    assert(invalid_result.outcomes[2].reject_reason == alpha::CommandRejectReason::InvalidProject);
    assert(invalid_result.outcomes[2].reject_message == "The project id does not exist.");
    assert(invalid_result.dirty_chunks.empty());
    assert(invalid_result.dirty_overlays.empty());
    assert(invalid_result.dirty_settlements.empty());
    assert(invalid_result.new_projects.empty());
    assert(world_api.get_projects({.settlement_id = kSettlementId}).projects.empty());
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const std::vector<alpha::CellCoord> valid_route =
        make_route({.x = starting_summary.center.x + 3, .y = starting_summary.center.y + 1}, 3);
    const alpha::BatchResult queue_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells = valid_route,
                    .priority = alpha::PriorityLabel::Normal,
                },
            },
    });
    assert(queue_result.outcomes[0].accepted);

    const alpha::BatchResult priority_result = world_api.apply_commands({
        .commands =
            {
                alpha::SetProjectPriorityCommand{
                    .project_id = alpha::ProjectId{1},
                    .priority = alpha::PriorityLabel::Paused,
                },
            },
    });
    assert(priority_result.outcomes.size() == 1U);
    assert(priority_result.outcomes[0].accepted);
    assert(priority_result.dirty_overlays ==
           std::vector<alpha::OverlayType>{alpha::OverlayType::ProjectBlockers});
    assert(priority_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});

    const alpha::ProjectListResult projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    assert(projects.projects.size() == 1U);
    assert(projects.projects[0].priority == alpha::PriorityLabel::Paused);
    const std::vector<alpha::ProjectBlockerCode> expected_paused_blocker_codes{
        alpha::ProjectBlockerCode::PausedByPriority,
        alpha::ProjectBlockerCode::WaitingForConstructionSystem,
    };
    assert(projects.projects[0].blocker_codes == expected_paused_blocker_codes);
    const std::vector<std::string> expected_paused_blockers{
        "Project priority is set to Paused.",
        "Construction progression is not implemented for queued projects yet.",
    };
    assert(projects.projects[0].blockers == expected_paused_blockers);
  }

  {
    alpha::WorldApi first_world_api = load_test_world(fixture_path);
    alpha::WorldApi second_world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(first_world_api);
    clear_initial_dirty_chunks(second_world_api);

    const alpha::SettlementSummary first_summary = first_world_api.get_settlement_summary(kSettlementId);
    const alpha::CommandBatch identical_batch{
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells =
                        make_route({.x = first_summary.center.x + 3, .y = first_summary.center.y + 2}, 5),
                    .priority = alpha::PriorityLabel::Required,
                },
                alpha::SetProjectPriorityCommand{
                    .project_id = alpha::ProjectId{1},
                    .priority = alpha::PriorityLabel::Low,
                },
            },
    };

    const alpha::BatchResult first_result = first_world_api.apply_commands(identical_batch);
    const alpha::BatchResult second_result = second_world_api.apply_commands(identical_batch);
    assert_batch_results_equal(first_result, second_result);

    const alpha::ProjectListResult first_projects =
        first_world_api.get_projects({.settlement_id = kSettlementId});
    const alpha::ProjectListResult second_projects =
        second_world_api.get_projects({.settlement_id = kSettlementId});
    assert_project_lists_equal(first_projects, second_projects);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const alpha::BatchResult queue_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells =
                        make_route({.x = starting_summary.center.x + 3, .y = starting_summary.center.y + 3}, 4),
                    .priority = alpha::PriorityLabel::High,
                },
            },
    });
    assert(queue_result.outcomes[0].accepted);

    const std::filesystem::path save_path =
        std::filesystem::temp_directory_path() / ("alpha_task7_project_roundtrip_" + test_suffix + ".bin");
    const std::filesystem::path json_path = save_path.string() + ".json";
    std::filesystem::remove(save_path);
    std::filesystem::remove(json_path);

    const alpha::ProjectListResult pre_save_projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    const alpha::SaveWorldResult save_result = world_api.save_world({
        .path = save_path.string(),
        .write_json_debug_export = true,
    });
    assert(save_result.ok);
    assert(save_result.json_debug_path == json_path.string());

    alpha::WorldApi loaded_world_api;
    const alpha::LoadWorldResult load_result = loaded_world_api.load_world({
        .path = save_path.string(),
    });
    assert(load_result.ok);
    assert(load_result.format_version == alpha::save::kSaveFormatVersion);

    const alpha::ProjectListResult post_load_projects =
        loaded_world_api.get_projects({.settlement_id = kSettlementId});
    assert_project_lists_equal(pre_save_projects, post_load_projects);
    assert(loaded_world_api.get_settlement_summary(kSettlementId).active_project_count == 1U);
    assert(loaded_world_api.get_world_metrics().project_count == 1U);

    std::filesystem::remove(save_path);
    std::filesystem::remove(json_path);
  }

  std::filesystem::remove(fixture_path);
  return 0;
}
