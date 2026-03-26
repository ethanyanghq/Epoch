#include "alpha/settlements/farm_plots.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "alpha/projects/project_progress.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::settlements {
namespace {

constexpr uint8_t kForestedVegetationThreshold = 50;
constexpr std::size_t kMinimumPlotSize = 12;
constexpr std::size_t kTargetPlotSize = 32;
constexpr std::size_t kLargePlotThreshold = 48;

struct PlotDraft {
  ZoneId parent_zone_id;
  std::vector<uint32_t> cell_indices;
  uint16_t avg_fertility_tenths = 0;
  uint16_t avg_access_cost_tenths = 0;
  bool forested_flag = false;
};

CellCoord to_cell_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const int32_t width = static_cast<int32_t>(world_state.map_width);
  return {
      .x = static_cast<int32_t>(cell_index % static_cast<uint32_t>(width)),
      .y = static_cast<int32_t>(cell_index / static_cast<uint32_t>(width)),
  };
}

int32_t vegetation_penalty_centi(const uint8_t vegetation) noexcept {
  if (vegetation < 25U) {
    return 0;
  }
  if (vegetation < 50U) {
    return 15;
  }
  if (vegetation < 75U) {
    return 35;
  }
  return 70;
}

int32_t slope_penalty_centi(const uint8_t slope) noexcept {
  switch (slope) {
    case 0:
      return 0;
    case 1:
      return 25;
    case 2:
      return 75;
    case 3:
      return 150;
    default:
      return std::numeric_limits<int32_t>::max();
  }
}

int32_t water_penalty_centi(const uint8_t water) noexcept {
  switch (water) {
    case 0:
      return 0;
    case 1:
      return 100;
    default:
      return std::numeric_limits<int32_t>::max();
  }
}

int32_t road_slope_penalty_centi(const uint8_t slope) noexcept {
  switch (slope) {
    case 0:
      return 0;
    case 1:
      return 5;
    case 2:
      return 15;
    case 3:
      return 35;
    default:
      return std::numeric_limits<int32_t>::max();
  }
}

int32_t road_water_penalty_centi(const uint8_t water) noexcept {
  switch (water) {
    case 0:
      return 0;
    case 1:
      return 25;
    default:
      return std::numeric_limits<int32_t>::max();
  }
}

int32_t movement_cost_centi(const world::WorldState& world_state, const uint32_t cell_index,
                            const map::MapCell& cell) noexcept {
  if (projects::is_road_built(world_state, cell_index)) {
    const int32_t slope_penalty = road_slope_penalty_centi(cell.slope);
    const int32_t water_penalty = road_water_penalty_centi(cell.water);
    if (slope_penalty == std::numeric_limits<int32_t>::max() ||
        water_penalty == std::numeric_limits<int32_t>::max()) {
      return std::numeric_limits<int32_t>::max();
    }

    return 35 + slope_penalty + water_penalty;
  }

  const int32_t slope_penalty = slope_penalty_centi(cell.slope);
  const int32_t water_penalty = water_penalty_centi(cell.water);
  if (slope_penalty == std::numeric_limits<int32_t>::max() ||
      water_penalty == std::numeric_limits<int32_t>::max()) {
    return std::numeric_limits<int32_t>::max();
  }

  return 100 + slope_penalty + vegetation_penalty_centi(cell.vegetation) + water_penalty;
}

std::vector<int32_t> compute_reach_costs(const world::WorldState& world_state,
                                         const SettlementState& settlement) {
  const std::size_t cell_count = static_cast<std::size_t>(world_state.map_width) *
                                 static_cast<std::size_t>(world_state.map_height);
  std::vector<int32_t> costs(cell_count, std::numeric_limits<int32_t>::max());
  if (!world_state.map_grid.contains_cell(settlement.center.x, settlement.center.y)) {
    return costs;
  }

  using QueueEntry = std::pair<int32_t, uint32_t>;
  std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;
  const uint32_t start_index =
      static_cast<uint32_t>(map::flatten_cell_index(world_state.map_width, settlement.center.x,
                                                    settlement.center.y));
  costs[start_index] = 0;
  frontier.push({0, start_index});

  constexpr std::array<std::pair<int32_t, int32_t>, 4> kNeighbors = {
      std::pair{-1, 0},
      std::pair{1, 0},
      std::pair{0, -1},
      std::pair{0, 1},
  };

  while (!frontier.empty()) {
    const auto [current_cost, current_index] = frontier.top();
    frontier.pop();
    if (current_cost != costs[current_index]) {
      continue;
    }

    const CellCoord coord = to_cell_coord(world_state, current_index);
    for (const auto [dx, dy] : kNeighbors) {
      const int32_t neighbor_x = coord.x + dx;
      const int32_t neighbor_y = coord.y + dy;
      if (!world_state.map_grid.contains_cell(neighbor_x, neighbor_y)) {
        continue;
      }

      const uint32_t neighbor_index =
          static_cast<uint32_t>(map::flatten_cell_index(world_state.map_width, neighbor_x, neighbor_y));
      const map::MapCell& neighbor_cell = world_state.map_grid.cell(neighbor_x, neighbor_y);
      const int32_t step_cost = movement_cost_centi(world_state, neighbor_index, neighbor_cell);
      if (step_cost == std::numeric_limits<int32_t>::max()) {
        continue;
      }

      const int32_t neighbor_cost = current_cost + step_cost;
      if (neighbor_cost >= costs[neighbor_index]) {
        continue;
      }

      costs[neighbor_index] = neighbor_cost;
      frontier.push({neighbor_cost, neighbor_index});
    }
  }

  return costs;
}

uint16_t compute_avg_fertility_tenths(const world::WorldState& world_state,
                                      const std::vector<uint32_t>& cell_indices) {
  if (cell_indices.empty()) {
    return 0;
  }

  int32_t fertility_sum = 0;
  for (const uint32_t cell_index : cell_indices) {
    const CellCoord coord = to_cell_coord(world_state, cell_index);
    fertility_sum += world_state.map_grid.cell(coord.x, coord.y).fertility;
  }

  return static_cast<uint16_t>((fertility_sum * 10) /
                               static_cast<int32_t>(cell_indices.size()));
}

uint16_t compute_avg_access_cost_tenths(const world::WorldState& world_state,
                                        const std::vector<int32_t>& reach_costs,
                                        const std::vector<uint32_t>& cell_indices) {
  if (cell_indices.empty()) {
    return 0;
  }

  int64_t access_sum = 0;
  for (const uint32_t cell_index : cell_indices) {
    if (cell_index >= reach_costs.size() || reach_costs[cell_index] == std::numeric_limits<int32_t>::max()) {
      continue;
    }
    access_sum += reach_costs[cell_index] / 10;
  }

  return static_cast<uint16_t>(access_sum / static_cast<int64_t>(cell_indices.size()));
}

bool compute_forested_flag(const world::WorldState& world_state,
                           const std::vector<uint32_t>& cell_indices) {
  for (const uint32_t cell_index : cell_indices) {
    const CellCoord coord = to_cell_coord(world_state, cell_index);
    if (world_state.map_grid.cell(coord.x, coord.y).vegetation >= kForestedVegetationThreshold) {
      return true;
    }
  }

  return false;
}

bool plot_priority_less(const FarmPlotState& left, const FarmPlotState& right) {
  if (left.avg_fertility_tenths != right.avg_fertility_tenths) {
    return left.avg_fertility_tenths > right.avg_fertility_tenths;
  }
  if (left.avg_access_cost_tenths != right.avg_access_cost_tenths) {
    return left.avg_access_cost_tenths < right.avg_access_cost_tenths;
  }
  if (left.forested_flag != right.forested_flag) {
    return !left.forested_flag;
  }
  if (left.cell_indices.size() != right.cell_indices.size()) {
    return left.cell_indices.size() > right.cell_indices.size();
  }
  return left.plot_id < right.plot_id;
}

bool plot_view_priority_less(const FarmPlotView& left, const FarmPlotView& right) {
  if (left.avg_fertility_tenths != right.avg_fertility_tenths) {
    return left.avg_fertility_tenths > right.avg_fertility_tenths;
  }
  if (left.avg_access_cost_tenths != right.avg_access_cost_tenths) {
    return left.avg_access_cost_tenths < right.avg_access_cost_tenths;
  }
  if (left.size != right.size) {
    return left.size > right.size;
  }
  return left.plot_id < right.plot_id;
}

int32_t overlap_count(const std::vector<uint32_t>& left, const std::vector<uint32_t>& right) {
  std::size_t left_index = 0;
  std::size_t right_index = 0;
  int32_t overlap = 0;

  while (left_index < left.size() && right_index < right.size()) {
    if (left[left_index] == right[right_index]) {
      ++overlap;
      ++left_index;
      ++right_index;
      continue;
    }
    if (left[left_index] < right[right_index]) {
      ++left_index;
    } else {
      ++right_index;
    }
  }

  return overlap;
}

std::vector<PlotDraft> build_zone_plot_drafts(const world::WorldState& world_state,
                                              const std::vector<int32_t>& reach_costs,
                                              const zones::ZoneState& zone) {
  std::vector<PlotDraft> drafts;
  if (zone.zone_type != ZoneType::Farmland || zone.member_cell_indices.size() < kMinimumPlotSize) {
    return drafts;
  }

  std::set<uint32_t> remaining(zone.member_cell_indices.begin(), zone.member_cell_indices.end());
  std::set<uint32_t> queued;

  while (remaining.size() >= kMinimumPlotSize) {
    const std::size_t target_size =
        remaining.size() > kLargePlotThreshold ? kTargetPlotSize : remaining.size();

    std::vector<uint32_t> plot_cells;
    std::queue<uint32_t> frontier;
    frontier.push(*remaining.begin());
    queued.insert(*remaining.begin());

    while (!frontier.empty() && plot_cells.size() < target_size) {
      const uint32_t current = frontier.front();
      frontier.pop();

      auto current_it = remaining.find(current);
      if (current_it == remaining.end()) {
        continue;
      }

      plot_cells.push_back(current);
      remaining.erase(current_it);

      const CellCoord coord = to_cell_coord(world_state, current);
      constexpr std::array<std::pair<int32_t, int32_t>, 4> kNeighbors = {
          std::pair{-1, 0},
          std::pair{1, 0},
          std::pair{0, -1},
          std::pair{0, 1},
      };

      for (const auto [dx, dy] : kNeighbors) {
        const int32_t neighbor_x = coord.x + dx;
        const int32_t neighbor_y = coord.y + dy;
        if (!world_state.map_grid.contains_cell(neighbor_x, neighbor_y)) {
          continue;
        }

        const uint32_t neighbor_index =
            static_cast<uint32_t>(map::flatten_cell_index(world_state.map_width, neighbor_x, neighbor_y));
        if (remaining.contains(neighbor_index) && !queued.contains(neighbor_index)) {
          frontier.push(neighbor_index);
          queued.insert(neighbor_index);
        }
      }
    }

    queued.clear();
    if (plot_cells.size() < kMinimumPlotSize) {
      continue;
    }

    std::sort(plot_cells.begin(), plot_cells.end());
    drafts.push_back({
        .parent_zone_id = zone.zone_id,
        .cell_indices = std::move(plot_cells),
    });
  }

  for (PlotDraft& draft : drafts) {
    draft.avg_fertility_tenths = compute_avg_fertility_tenths(world_state, draft.cell_indices);
    draft.avg_access_cost_tenths = compute_avg_access_cost_tenths(world_state, reach_costs, draft.cell_indices);
    draft.forested_flag = compute_forested_flag(world_state, draft.cell_indices);
  }

  return drafts;
}

std::vector<const FarmPlotState*> collect_old_zone_plots(const world::WorldState& world_state,
                                                         const ZoneId zone_id) {
  std::vector<const FarmPlotState*> old_plots;
  for (const FarmPlotState& plot : world_state.farm_plots) {
    if (plot.parent_zone_id == zone_id) {
      old_plots.push_back(&plot);
    }
  }

  std::sort(old_plots.begin(), old_plots.end(),
            [](const FarmPlotState* left, const FarmPlotState* right) {
              return left->plot_id < right->plot_id;
            });
  return old_plots;
}

FarmPlotState make_new_plot_state(const PlotDraft& draft, FarmPlotId next_plot_id) {
  return {
      .plot_id = next_plot_id,
      .parent_zone_id = draft.parent_zone_id,
      .cell_indices = draft.cell_indices,
      .state = FarmPlotStateCode::Unopened,
      .avg_fertility_tenths = draft.avg_fertility_tenths,
      .avg_access_cost_tenths = draft.avg_access_cost_tenths,
      .forested_flag = draft.forested_flag,
      .labor_coverage_tenths = 0,
      .current_year_required_labor = 0,
      .current_year_assigned_labor = 0,
      .opening_months_remaining = 0,
      .opening_work_remaining_tenths = 0,
  };
}

}  // namespace

void rebuild_world_farm_plots(world::WorldState& world_state) {
  std::vector<const zones::ZoneState*> farmland_zones;
  farmland_zones.reserve(world_state.zones.size());
  for (const zones::ZoneState& zone : world_state.zones) {
    if (zone.zone_type == ZoneType::Farmland) {
      farmland_zones.push_back(&zone);
    }
  }

  std::sort(farmland_zones.begin(), farmland_zones.end(),
            [](const zones::ZoneState* left, const zones::ZoneState* right) {
              return left->zone_id < right->zone_id;
            });

  std::vector<FarmPlotState> rebuilt_plots;
  rebuilt_plots.reserve(world_state.farm_plots.size());

  for (const zones::ZoneState* zone : farmland_zones) {
    const SettlementState* settlement = find_settlement(world_state.settlements, zone->owner_settlement_id);
    const std::vector<int32_t> reach_costs =
        settlement != nullptr ? compute_reach_costs(world_state, *settlement) : std::vector<int32_t>{};
    std::vector<PlotDraft> drafts = build_zone_plot_drafts(world_state, reach_costs, *zone);
    std::vector<const FarmPlotState*> old_plots = collect_old_zone_plots(world_state, zone->zone_id);
    std::vector<bool> old_plot_consumed(old_plots.size(), false);

    for (const PlotDraft& draft : drafts) {
      int32_t best_overlap = 0;
      std::size_t best_index = std::numeric_limits<std::size_t>::max();

      for (std::size_t index = 0; index < old_plots.size(); ++index) {
        if (old_plot_consumed[index]) {
          continue;
        }

        const int32_t overlap = overlap_count(old_plots[index]->cell_indices, draft.cell_indices);
        if (overlap > best_overlap ||
            (overlap == best_overlap && overlap > 0 &&
             old_plots[index]->plot_id < old_plots[best_index]->plot_id)) {
          best_overlap = overlap;
          best_index = index;
        }
      }

      if (best_index != std::numeric_limits<std::size_t>::max() && best_overlap > 0) {
        FarmPlotState plot = *old_plots[best_index];
        plot.parent_zone_id = draft.parent_zone_id;
        plot.cell_indices = draft.cell_indices;
        plot.avg_fertility_tenths = draft.avg_fertility_tenths;
        plot.avg_access_cost_tenths = draft.avg_access_cost_tenths;
        plot.forested_flag = draft.forested_flag;
        rebuilt_plots.push_back(std::move(plot));
        old_plot_consumed[best_index] = true;
        continue;
      }

      rebuilt_plots.push_back(make_new_plot_state(draft, world_state.next_farm_plot_id));
      ++world_state.next_farm_plot_id.value;
    }
  }

  std::sort(rebuilt_plots.begin(), rebuilt_plots.end(),
            [](const FarmPlotState& left, const FarmPlotState& right) {
              return left.plot_id < right.plot_id;
            });

  uint32_t max_plot_id = 0;
  for (const FarmPlotState& plot : rebuilt_plots) {
    max_plot_id = std::max(max_plot_id, plot.plot_id.value);
  }

  world_state.farm_plots = std::move(rebuilt_plots);
  world_state.plot_count = static_cast<uint32_t>(world_state.farm_plots.size());
  world_state.next_farm_plot_id = FarmPlotId{max_plot_id + 1U};
}

const FarmPlotState* find_farm_plot(const world::WorldState& world_state,
                                    const FarmPlotId plot_id) noexcept {
  for (const FarmPlotState& plot : world_state.farm_plots) {
    if (plot.plot_id == plot_id) {
      return &plot;
    }
  }

  return nullptr;
}

std::vector<const FarmPlotState*> collect_settlement_farm_plots(const world::WorldState& world_state,
                                                                const SettlementId settlement_id) {
  std::vector<const FarmPlotState*> plots;
  plots.reserve(world_state.farm_plots.size());

  for (const FarmPlotState& plot : world_state.farm_plots) {
    const zones::ZoneState* parent_zone = nullptr;
    for (const zones::ZoneState& zone : world_state.zones) {
      if (zone.zone_id == plot.parent_zone_id) {
        parent_zone = &zone;
        break;
      }
    }

    if (parent_zone != nullptr && parent_zone->owner_settlement_id == settlement_id) {
      plots.push_back(&plot);
    }
  }

  std::sort(plots.begin(), plots.end(),
            [](const FarmPlotState* left, const FarmPlotState* right) {
              return plot_priority_less(*left, *right);
            });
  return plots;
}

std::vector<FarmPlotView> build_farm_plot_views(const world::WorldState& world_state,
                                                const SettlementId settlement_id) {
  std::vector<FarmPlotView> views;
  const std::vector<const FarmPlotState*> plots =
      collect_settlement_farm_plots(world_state, settlement_id);
  views.reserve(plots.size());

  for (const FarmPlotState* plot : plots) {
    views.push_back({
        .plot_id = plot->plot_id,
        .parent_zone_id = plot->parent_zone_id,
        .size = static_cast<uint16_t>(plot->cell_indices.size()),
        .avg_fertility_tenths = plot->avg_fertility_tenths,
        .avg_access_cost_tenths = plot->avg_access_cost_tenths,
        .labor_coverage_tenths = plot->labor_coverage_tenths,
        .state_name = farm_plot_state_name(plot->state),
    });
  }

  std::sort(views.begin(), views.end(), plot_view_priority_less);
  return views;
}

const char* farm_plot_state_name(const FarmPlotStateCode state) noexcept {
  switch (state) {
    case FarmPlotStateCode::Unopened:
      return "Unopened";
    case FarmPlotStateCode::Opening:
      return "Opening";
    case FarmPlotStateCode::Active:
      return "Active";
    case FarmPlotStateCode::Fallow:
      return "Fallow";
  }

  return "Unknown";
}

}  // namespace alpha::settlements
