#include "alpha/core/simulation.hpp"

#include <array>
#include <string_view>
#include <utility>

#include "alpha/map/chunk_visuals.hpp"
#include "alpha/map/overlay_chunks.hpp"
#include "alpha/save/save_io.hpp"
#include "alpha/settlements/settlement_types.hpp"

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

  return map::build_overlay_chunk_result(world_state_->map_grid, query);
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
  summary.active_zone_count = 0;
  summary.active_project_count = 0;
  return summary;
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
