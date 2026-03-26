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
      .vegetation = 75,
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
      .vegetation = 75,
  };

  assert(world_state.map_grid.initialize_from_cells(kMapWidth, kMapHeight, std::move(cells)));

  world_state.road_cells.assign(static_cast<std::size_t>(kMapWidth) * static_cast<std::size_t>(kMapHeight),
                                0U);
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
      std::filesystem::temp_directory_path() / ("alpha_task8_project_fixture_" + suffix + ".bin");
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

std::size_t chunk_cell_index(const alpha::ChunkCoord chunk, const alpha::CellCoord cell) {
  const int32_t local_x = cell.x - static_cast<int32_t>(chunk.x) * alpha::kChunkSize;
  const int32_t local_y = cell.y - static_cast<int32_t>(chunk.y) * alpha::kChunkSize;
  return static_cast<std::size_t>(local_y) * alpha::kChunkSize + static_cast<std::size_t>(local_x);
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

}  // namespace

int main() {
  const std::string test_suffix = std::to_string(static_cast<long long>(::getpid()));
  const std::filesystem::path fixture_path = write_test_world_save(test_suffix);

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    assert(!starting_summary.buildings[1].exists);

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
    assert(queue_result.new_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{1}});
    assert(queue_result.dirty_overlays ==
           std::vector<alpha::OverlayType>{alpha::OverlayType::ProjectBlockers});
    assert(queue_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});

    const alpha::SettlementSummary queued_summary = world_api.get_settlement_summary(kSettlementId);
    assert(queued_summary.food == 400);
    assert(queued_summary.wood == 160);
    assert(queued_summary.stone == 80);
    assert(queued_summary.active_project_count == 1U);

    const alpha::ProjectListResult projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    assert(projects.projects.size() == 1U);
    assert(projects.projects[0].project_id == alpha::ProjectId{1});
    assert(projects.projects[0].owner_settlement_id == kSettlementId);
    assert(projects.projects[0].family == alpha::ProjectFamily::Road);
    assert(projects.projects[0].type_name == "Road");
    assert(projects.projects[0].priority == alpha::PriorityLabel::High);
    assert(projects.projects[0].status == alpha::ProjectStatus::Queued);
    assert(projects.projects[0].status_name == "Queued");
    assert(projects.projects[0].required_wood == 40);
    assert(projects.projects[0].required_stone == 20);
    assert(projects.projects[0].reserved_wood == 40);
    assert(projects.projects[0].reserved_stone == 20);
    assert(projects.projects[0].remaining_common_work == 40);
    assert(projects.projects[0].remaining_skilled_work == 0);
    assert(projects.projects[0].blocker_codes.empty());
    assert(projects.projects[0].blockers.empty());
    assert(world_api.get_world_metrics().project_count == 1U);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::BatchResult queue_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueBuildingCommand{
                    .settlement_id = kSettlementId,
                    .building_type = alpha::BuildingType::WarehouseI,
                    .priority = alpha::PriorityLabel::Normal,
                },
            },
    });
    assert(queue_result.outcomes.size() == 1U);
    assert(queue_result.outcomes[0].accepted);
    assert(queue_result.new_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{1}});
    assert(queue_result.dirty_chunks.empty());
    assert(queue_result.dirty_settlements == std::vector<alpha::SettlementId>{kSettlementId});

    const alpha::ProjectListResult projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    assert(projects.projects.size() == 1U);
    assert(projects.projects[0].family == alpha::ProjectFamily::Building);
    assert(projects.projects[0].type_name == "Warehouse I");
    assert(projects.projects[0].required_wood == 40);
    assert(projects.projects[0].required_stone == 20);
    assert(projects.projects[0].reserved_wood == 40);
    assert(projects.projects[0].reserved_stone == 20);
    assert(projects.projects[0].remaining_common_work == 40);

    const alpha::SettlementSummary queued_summary = world_api.get_settlement_summary(kSettlementId);
    assert(queued_summary.wood == 160);
    assert(queued_summary.stone == 80);
    assert(!queued_summary.buildings[1].exists);

    const alpha::BatchResult invalid_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueBuildingCommand{
                    .settlement_id = kSettlementId,
                    .building_type = alpha::BuildingType::WarehouseI,
                    .priority = alpha::PriorityLabel::Normal,
                },
                alpha::QueueBuildingCommand{
                    .settlement_id = kSettlementId,
                    .building_type = alpha::BuildingType::EstateI,
                    .priority = alpha::PriorityLabel::Normal,
                },
            },
    });
    assert(invalid_result.outcomes.size() == 2U);
    assert(!invalid_result.outcomes[0].accepted);
    assert(invalid_result.outcomes[0].reject_reason ==
           alpha::CommandRejectReason::DuplicateUniqueBuilding);
    assert(!invalid_result.outcomes[1].accepted);
    assert(invalid_result.outcomes[1].reject_reason ==
           alpha::CommandRejectReason::IllegalBuildingRequest);
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
    assert(!invalid_result.outcomes[1].accepted);
    assert(invalid_result.outcomes[1].reject_reason == alpha::CommandRejectReason::IllegalRoadRoute);
    assert(!invalid_result.outcomes[2].accepted);
    assert(invalid_result.outcomes[2].reject_reason == alpha::CommandRejectReason::InvalidProject);
    assert(world_api.get_projects({.settlement_id = kSettlementId}).projects.empty());
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const std::vector<alpha::CellCoord> road_route =
        make_route({.x = starting_summary.center.x + 3, .y = starting_summary.center.y + 1}, 4);
    const alpha::BatchResult queue_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells = road_route,
                    .priority = alpha::PriorityLabel::High,
                },
                alpha::QueueBuildingCommand{
                    .settlement_id = kSettlementId,
                    .building_type = alpha::BuildingType::WarehouseI,
                    .priority = alpha::PriorityLabel::Low,
                },
            },
    });
    assert(queue_result.outcomes[0].accepted);
    assert(queue_result.outcomes[1].accepted);

    const alpha::TurnReport first_report = world_api.advance_month();
    assert(first_report.completed_projects.empty());
    assert(first_report.newly_blocked_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{2}});
    assert(first_report.newly_unblocked_projects.empty());
    assert(first_report.dirty_chunks.size() == 1U);
    assert(first_report.dirty_overlays ==
           std::vector<alpha::OverlayType>{alpha::OverlayType::ProjectBlockers});

    const alpha::ProjectListResult first_projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    assert(first_projects.projects.size() == 2U);
    assert(first_projects.projects[0].status == alpha::ProjectStatus::InProgress);
    assert(first_projects.projects[0].reserved_wood == 20);
    assert(first_projects.projects[0].reserved_stone == 10);
    assert(first_projects.projects[0].remaining_common_work == 20);
    assert(first_projects.projects[1].status == alpha::ProjectStatus::Blocked);
    const std::vector<alpha::ProjectBlockerCode> expected_blocker_codes{
        alpha::ProjectBlockerCode::WaitingForConstructionCapacity,
    };
    assert(first_projects.projects[1].blocker_codes == expected_blocker_codes);
    const std::vector<std::string> expected_blockers{
        "Higher-priority projects used this month's construction capacity.",
    };
    assert(first_projects.projects[1].blockers == expected_blockers);

    const alpha::ChunkCoord road_chunk{
        static_cast<int16_t>(road_route.front().x / alpha::kChunkSize),
        static_cast<int16_t>(road_route.front().y / alpha::kChunkSize),
    };
    const alpha::ChunkVisualResult chunk_visual =
        world_api.get_chunk_visual({.chunk = road_chunk, .layer_index = 0});
    assert(chunk_visual.cells[chunk_cell_index(road_chunk, road_route[0])].road_flag == 1U);
    assert(chunk_visual.cells[chunk_cell_index(road_chunk, road_route[1])].road_flag == 1U);
    assert(chunk_visual.cells[chunk_cell_index(road_chunk, road_route[2])].road_flag == 0U);
    assert(chunk_visual.cells[chunk_cell_index(road_chunk, road_route[3])].road_flag == 0U);

    const alpha::TurnReport second_report = world_api.advance_month();
    assert(second_report.completed_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{1}});

    const alpha::TurnReport third_report = world_api.advance_month();
    assert(third_report.newly_unblocked_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{2}});

    const alpha::TurnReport fourth_report = world_api.advance_month();
    assert(fourth_report.completed_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{2}});

    const alpha::SettlementSummary completed_summary = world_api.get_settlement_summary(kSettlementId);
    assert(completed_summary.buildings[1].exists);
    assert(completed_summary.active_project_count == 0U);
    assert(world_api.get_world_metrics().road_cell_count == 4U);
  }

  {
    alpha::WorldApi world_api = load_test_world(fixture_path);
    clear_initial_dirty_chunks(world_api);

    const alpha::SettlementSummary starting_summary = world_api.get_settlement_summary(kSettlementId);
    const std::vector<alpha::CellCoord> road_route =
        make_route({.x = starting_summary.center.x + 3, .y = starting_summary.center.y + 2}, 20);
    const alpha::CellCoord target = road_route.back();

    const alpha::BatchResult pre_road_zone_result = world_api.apply_commands({
        .commands =
            {
                alpha::ZoneCellsCommand{
                    .settlement_id = kSettlementId,
                    .zone_type = alpha::ZoneType::Farmland,
                    .cells = {target},
                },
            },
    });
    assert(pre_road_zone_result.outcomes.size() == 1U);
    assert(!pre_road_zone_result.outcomes[0].accepted);
    assert(pre_road_zone_result.outcomes[0].reject_reason ==
           alpha::CommandRejectReason::IllegalZoneTarget);

    const alpha::BatchResult road_result = world_api.apply_commands({
        .commands =
            {
                alpha::QueueRoadCommand{
                    .settlement_id = kSettlementId,
                    .route_cells = road_route,
                    .priority = alpha::PriorityLabel::Required,
                },
            },
    });
    assert(road_result.outcomes[0].accepted);

    for (int32_t month = 0; month < 10; ++month) {
      const alpha::TurnReport report = world_api.advance_month();
      if (month < 9) {
        assert(report.completed_projects.empty());
      }
    }

    const alpha::BatchResult post_road_zone_result = world_api.apply_commands({
        .commands =
            {
                alpha::ZoneCellsCommand{
                    .settlement_id = kSettlementId,
                    .zone_type = alpha::ZoneType::Farmland,
                    .cells = {target},
                },
            },
    });
    assert(post_road_zone_result.outcomes.size() == 1U);
    assert(post_road_zone_result.outcomes[0].accepted);

    const alpha::ChunkCoord road_chunk{
        static_cast<int16_t>(target.x / alpha::kChunkSize),
        static_cast<int16_t>(target.y / alpha::kChunkSize),
    };
    const alpha::ChunkVisualResult chunk_visual =
        world_api.get_chunk_visual({.chunk = road_chunk, .layer_index = 0});
    assert(chunk_visual.cells[chunk_cell_index(road_chunk, target)].road_flag == 1U);
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
                alpha::QueueBuildingCommand{
                    .settlement_id = kSettlementId,
                    .building_type = alpha::BuildingType::WarehouseI,
                    .priority = alpha::PriorityLabel::Low,
                },
            },
    });
    assert(queue_result.outcomes[0].accepted);
    assert(queue_result.outcomes[1].accepted);

    const alpha::TurnReport pre_save_report = world_api.advance_month();
    assert(pre_save_report.newly_blocked_projects == std::vector<alpha::ProjectId>{alpha::ProjectId{2}});

    const std::filesystem::path save_path =
        std::filesystem::temp_directory_path() / ("alpha_task8_project_roundtrip_" + test_suffix + ".bin");
    const std::filesystem::path json_path = save_path.string() + ".json";
    std::filesystem::remove(save_path);
    std::filesystem::remove(json_path);

    const alpha::ProjectListResult pre_save_projects =
        world_api.get_projects({.settlement_id = kSettlementId});
    const alpha::ChunkVisualResult pre_save_chunk =
        world_api.get_chunk_visual({.chunk = {.x = 8, .y = 8}, .layer_index = 0});
    const alpha::SettlementSummary pre_save_summary = world_api.get_settlement_summary(kSettlementId);

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
    const alpha::ChunkVisualResult post_load_chunk =
        loaded_world_api.get_chunk_visual({.chunk = {.x = 8, .y = 8}, .layer_index = 0});
    for (std::size_t index = 0; index < pre_save_chunk.cells.size(); ++index) {
      assert(pre_save_chunk.cells[index].road_flag == post_load_chunk.cells[index].road_flag);
    }
    const alpha::SettlementSummary post_load_summary =
        loaded_world_api.get_settlement_summary(kSettlementId);
    assert(pre_save_summary.active_project_count == post_load_summary.active_project_count);
    assert(pre_save_summary.buildings[1].exists == post_load_summary.buildings[1].exists);

    std::filesystem::remove(save_path);
    std::filesystem::remove(json_path);
  }

  std::filesystem::remove(fixture_path);
  return 0;
}
