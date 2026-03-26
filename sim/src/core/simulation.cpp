#include "alpha/core/simulation.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "alpha/map/chunk_visuals.hpp"
#include "alpha/map/overlay_chunks.hpp"
#include "alpha/projects/project_types.hpp"
#include "alpha/save/save_io.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/zones/zone_types.hpp"

namespace alpha::core {
namespace {

constexpr std::array<std::string_view, 5> kVisiblePhaseNames = {
    "Season & Labor",
    "Production",
    "Logistics",
    "Consumption",
    "Construction & Settlement Update",
};

std::vector<PhaseTiming> make_default_phase_timings() {
  std::vector<PhaseTiming> phase_timings;
  phase_timings.reserve(kVisiblePhaseNames.size());

  for (const std::string_view phase_name : kVisiblePhaseNames) {
    phase_timings.push_back(PhaseTiming{.phase_name = std::string(phase_name), .duration_ms = 0});
  }

  return phase_timings;
}

void sort_and_deduplicate_chunks(std::vector<ChunkCoord>& dirty_chunks) {
  std::sort(dirty_chunks.begin(), dirty_chunks.end(),
            [](const ChunkCoord& left, const ChunkCoord& right) {
              if (left.y != right.y) {
                return left.y < right.y;
              }
              return left.x < right.x;
            });
  dirty_chunks.erase(std::unique(dirty_chunks.begin(), dirty_chunks.end()), dirty_chunks.end());
}

void sort_and_deduplicate_settlements(std::vector<SettlementId>& dirty_settlements) {
  std::sort(dirty_settlements.begin(), dirty_settlements.end(),
            [](const SettlementId left, const SettlementId right) { return left < right; });
  dirty_settlements.erase(std::unique(dirty_settlements.begin(), dirty_settlements.end()),
                          dirty_settlements.end());
}

void sort_and_deduplicate_overlays(std::vector<OverlayType>& dirty_overlays) {
  std::sort(dirty_overlays.begin(), dirty_overlays.end(),
            [](const OverlayType left, const OverlayType right) {
              return static_cast<uint8_t>(left) < static_cast<uint8_t>(right);
            });
  dirty_overlays.erase(std::unique(dirty_overlays.begin(), dirty_overlays.end()),
                       dirty_overlays.end());
}

void merge_partial_batch_result(BatchResult& result, BatchResult partial_result,
                                const uint32_t command_index) {
  for (CommandOutcome& outcome : partial_result.outcomes) {
    outcome.command_index = command_index;
    result.outcomes.push_back(std::move(outcome));
  }

  result.dirty_chunks.insert(result.dirty_chunks.end(), partial_result.dirty_chunks.begin(),
                             partial_result.dirty_chunks.end());
  result.dirty_overlays.insert(result.dirty_overlays.end(), partial_result.dirty_overlays.begin(),
                               partial_result.dirty_overlays.end());
  result.dirty_settlements.insert(result.dirty_settlements.end(),
                                  partial_result.dirty_settlements.begin(),
                                  partial_result.dirty_settlements.end());
  result.new_projects.insert(result.new_projects.end(), partial_result.new_projects.begin(),
                             partial_result.new_projects.end());
}

}  // namespace

CreateWorldResult Simulation::create_world(const WorldCreateParams& params) {
  if (!world::is_supported_world_size(params.map_width, params.map_height)) {
    return {
        .ok = false,
        .error_message = "Alpha currently supports only 1024x1024 worlds.",
    };
  }

  world::WorldState world_state;
  world_state.terrain_seed = params.terrain_seed;
  world_state.map_width = params.map_width;
  world_state.map_height = params.map_height;
  world_state.generation_config_path = params.generation_config_path;
  world_state.world_rng = {
      .seed = params.gameplay_seed,
      .stream_state = params.gameplay_seed,
  };
  world_state.dirty_chunks = world::make_all_chunk_coords(params.map_width, params.map_height);

  if (!world_state.map_grid.initialize(params.map_width, params.map_height, params.terrain_seed)) {
    return {
        .ok = false,
        .error_message = "Failed to initialize the authoritative map grid.",
    };
  }

  world_state.settlements.push_back(settlements::make_starting_settlement(world_state.map_grid));
  zones::initialize_zone_state(world_state);
  world_state.next_project_id = {1};
  world_state.project_count = 0;
  world_state_ = std::move(world_state);

  return {
      .ok = true,
      .dirty_chunks = world_state_->dirty_chunks,
  };
}

LoadWorldResult Simulation::load_world(const LoadWorldParams& params) {
  save::LoadWorldStateResult load_result = save::load_world(params);
  if (!load_result.ok) {
    return {
        .ok = false,
        .error_message = load_result.error_message,
        .format_version = load_result.format_version,
    };
  }

  world_state_ = std::move(load_result.world_state);
  return {
      .ok = true,
      .format_version = load_result.format_version,
      .dirty_chunks = world_state_->dirty_chunks,
  };
}

SaveWorldResult Simulation::save_world(const SaveWorldParams& params) const {
  if (!world_state_.has_value()) {
    return {
        .ok = false,
        .error_message = "No world is loaded to save.",
    };
  }

  return save::save_world(*world_state_, params);
}

BatchResult Simulation::apply_commands(const CommandBatch& batch) {
  if (!world_state_.has_value()) {
    BatchResult result;
    result.outcomes.reserve(batch.commands.size());
    for (uint32_t command_index = 0; command_index < batch.commands.size(); ++command_index) {
      result.outcomes.push_back({
          .accepted = false,
          .command_index = command_index,
          .reject_reason = CommandRejectReason::Unknown,
          .reject_message = "No world is loaded.",
      });
    }
    return result;
  }

  BatchResult result;
  result.outcomes.reserve(batch.commands.size());

  for (uint32_t command_index = 0; command_index < batch.commands.size(); ++command_index) {
    const CommandVariant& command = batch.commands[command_index];
    std::visit(
        [&](const auto& typed_command) {
          using CommandType = std::decay_t<decltype(typed_command)>;

          if constexpr (std::is_same_v<CommandType, ZoneCellsCommand> ||
                        std::is_same_v<CommandType, RemoveZoneCellsCommand>) {
            CommandBatch single_command_batch;
            single_command_batch.commands.push_back(typed_command);
            merge_partial_batch_result(result, zones::apply_commands(*world_state_, single_command_batch),
                                       command_index);
          } else if constexpr (std::is_same_v<CommandType, QueueRoadCommand>) {
            projects::apply_queue_road_command(*world_state_, command_index, typed_command, result);
          } else if constexpr (std::is_same_v<CommandType, SetProjectPriorityCommand>) {
            projects::apply_set_project_priority_command(*world_state_, command_index, typed_command,
                                                         result);
          } else {
            result.outcomes.push_back({
                .accepted = false,
                .command_index = command_index,
                .reject_reason = CommandRejectReason::Unknown,
                .reject_message =
                    "This command type is not implemented in the current Milestone 1 slice.",
            });
          }
        },
        command);
  }

  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_settlements(result.dirty_settlements);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  return result;
}

TurnReport Simulation::advance_month() {
  if (!world_state_.has_value()) {
    return {};
  }

  world::advance_calendar(world_state_->calendar);
  world::clear_dirty_chunks(*world_state_);

  return {
      .year = world_state_->calendar.year,
      .month = world_state_->calendar.month,
      .phase_timings = make_default_phase_timings(),
  };
}

ChunkVisualResult Simulation::get_chunk_visual(const ChunkVisualQuery& query) const {
  if (!world_state_.has_value()) {
    return {
        .chunk = query.chunk,
    };
  }

  return map::build_chunk_visual_result(world_state_->map_grid, query);
}

OverlayChunkResult Simulation::get_overlay_chunk(const OverlayChunkQuery& query) const {
  if (!world_state_.has_value()) {
    return {
        .chunk = query.chunk,
        .overlay_type = query.overlay_type,
    };
  }

  switch (query.overlay_type) {
    case OverlayType::Fertility:
      return map::build_overlay_chunk_result(world_state_->map_grid, query);
    case OverlayType::ZoneOwner:
      return zones::build_zone_owner_overlay_chunk_result(*world_state_, query);
    default:
      return {
          .chunk = query.chunk,
          .overlay_type = query.overlay_type,
      };
  }
}

SettlementSummary Simulation::get_settlement_summary(const SettlementId settlement_id) const {
  SettlementSummary summary{
      .settlement_id = settlement_id,
      .buildings =
          {
              BuildingStateView{
                  .building_type = BuildingType::EstateI,
              },
              BuildingStateView{
                  .building_type = BuildingType::WarehouseI,
              },
          },
  };

  if (!world_state_.has_value()) {
    return summary;
  }

  const settlements::SettlementState* settlement =
      settlements::find_settlement(world_state_->settlements, settlement_id);
  if (settlement == nullptr) {
    return summary;
  }

  summary = settlements::build_settlement_summary(*settlement);
  summary.active_project_count = projects::count_active_projects(*world_state_, settlement_id);
  return summary;
}

ProjectListResult Simulation::get_projects(const ProjectListQuery& query) const {
  if (!world_state_.has_value()) {
    return {};
  }

  return projects::build_project_list_result(*world_state_, query);
}

WorldMetrics Simulation::get_world_metrics() const {
  if (!world_state_.has_value()) {
    return {};
  }

  return world::build_world_metrics(*world_state_);
}

bool Simulation::has_world() const noexcept {
  return world_state_.has_value();
}

}  // namespace alpha::core
