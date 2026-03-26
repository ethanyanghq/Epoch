#include "alpha/settlements/labor_state.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "alpha/api/constants.hpp"
#include "alpha/projects/project_progress.hpp"
#include "alpha/projects/project_types.hpp"
#include "alpha/settlements/farm_plots.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::settlements {
namespace {

constexpr ResourceAmountTenths kFoodConsumptionPerPopTenths = 1;
constexpr WorkAmountTenths kConstructionWorkPerLaborTenths = 10;
constexpr int32_t kResourceCellsPerSerf = 4;
constexpr int32_t kEstateMinimumNobles = 1;
constexpr int32_t kConstructionLaborCap = 2;
constexpr WorkAmountTenths kFallowReactivationWorkTenths = 50;
constexpr std::array<int32_t, 13> kPlotMonthlyLaborDemand = {
    0, 0, 0, 1, 2, 1, 1, 1, 1, 2, 3, 0, 0,
};

struct ForestryAllocation {
  const zones::ZoneState* zone = nullptr;
  int32_t demand = 0;
  int32_t assigned = 0;
};

struct QuarryAllocation {
  const zones::ZoneState* zone = nullptr;
  int32_t demand = 0;
  int32_t assigned = 0;
};

struct PlotAllocation {
  FarmPlotState* plot = nullptr;
  int32_t demand = 0;
  int32_t assigned = 0;
};

struct SettlementPhaseState {
  settlements::SettlementState* settlement = nullptr;
  settlements::StockpileState starting_stockpile;
  std::vector<PlotAllocation> active_plots;
  std::vector<PlotAllocation> opening_plots;
  std::vector<ForestryAllocation> forestry_zones;
  std::vector<QuarryAllocation> quarry_zones;
  projects::ConstructionLaborBudget construction_budget;
  int32_t protected_farming_demand = 0;
  int32_t total_active_farming_demand = 0;
  int32_t total_opening_demand = 0;
  int32_t total_forestry_demand = 0;
  int32_t total_quarry_demand = 0;
  int32_t total_construction_demand = 0;
  bool estate_staffed = false;
  bool food_shortage = false;
};

bool plot_priority_less(const FarmPlotState* left, const FarmPlotState* right) {
  if (left->avg_fertility_tenths != right->avg_fertility_tenths) {
    return left->avg_fertility_tenths > right->avg_fertility_tenths;
  }
  if (left->avg_access_cost_tenths != right->avg_access_cost_tenths) {
    return left->avg_access_cost_tenths < right->avg_access_cost_tenths;
  }
  if (left->forested_flag != right->forested_flag) {
    return !left->forested_flag;
  }
  if (left->cell_indices.size() != right->cell_indices.size()) {
    return left->cell_indices.size() > right->cell_indices.size();
  }
  return left->plot_id < right->plot_id;
}

bool plot_priority_reverse_less(const FarmPlotState* left, const FarmPlotState* right) {
  if (left->avg_fertility_tenths != right->avg_fertility_tenths) {
    return left->avg_fertility_tenths < right->avg_fertility_tenths;
  }
  if (left->avg_access_cost_tenths != right->avg_access_cost_tenths) {
    return left->avg_access_cost_tenths > right->avg_access_cost_tenths;
  }
  if (left->forested_flag != right->forested_flag) {
    return left->forested_flag;
  }
  if (left->cell_indices.size() != right->cell_indices.size()) {
    return left->cell_indices.size() < right->cell_indices.size();
  }
  return left->plot_id < right->plot_id;
}

ChunkCoord to_chunk_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const int32_t width = static_cast<int32_t>(world_state.map_width);
  const int32_t x = static_cast<int32_t>(cell_index % static_cast<uint32_t>(width));
  const int32_t y = static_cast<int32_t>(cell_index / static_cast<uint32_t>(width));
  return {
      .x = static_cast<int16_t>(x / kChunkSize),
      .y = static_cast<int16_t>(y / kChunkSize),
  };
}

void append_plot_dirty_chunks(const world::WorldState& world_state, const FarmPlotState& plot,
                              std::vector<ChunkCoord>& dirty_chunks) {
  for (const uint32_t cell_index : plot.cell_indices) {
    dirty_chunks.push_back(to_chunk_coord(world_state, cell_index));
  }
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

void sort_and_deduplicate_settlements(std::vector<SettlementId>& settlement_ids) {
  std::sort(settlement_ids.begin(), settlement_ids.end(),
            [](const SettlementId left, const SettlementId right) { return left < right; });
  settlement_ids.erase(std::unique(settlement_ids.begin(), settlement_ids.end()),
                       settlement_ids.end());
}

void sort_and_deduplicate_overlays(std::vector<OverlayType>& overlays) {
  std::sort(overlays.begin(), overlays.end(),
            [](const OverlayType left, const OverlayType right) {
              return static_cast<uint8_t>(left) < static_cast<uint8_t>(right);
            });
  overlays.erase(std::unique(overlays.begin(), overlays.end()), overlays.end());
}

const zones::ZoneState* find_zone(const world::WorldState& world_state, const ZoneId zone_id) {
  for (const zones::ZoneState& zone : world_state.zones) {
    if (zone.zone_id == zone_id) {
      return &zone;
    }
  }

  return nullptr;
}

std::vector<FarmPlotState*> collect_mutable_settlement_plots(world::WorldState& world_state,
                                                             const SettlementId settlement_id) {
  std::vector<FarmPlotState*> plots;
  plots.reserve(world_state.farm_plots.size());

  for (FarmPlotState& plot : world_state.farm_plots) {
    const zones::ZoneState* zone = find_zone(world_state, plot.parent_zone_id);
    if (zone != nullptr && zone->owner_settlement_id == settlement_id) {
      plots.push_back(&plot);
    }
  }

  std::sort(plots.begin(), plots.end(),
            [](const FarmPlotState* left, const FarmPlotState* right) {
              return plot_priority_less(left, right);
            });
  return plots;
}

std::vector<const zones::ZoneState*> collect_zones_by_type(const world::WorldState& world_state,
                                                           const SettlementId settlement_id,
                                                           const ZoneType zone_type) {
  std::vector<const zones::ZoneState*> zones_by_type;
  zones_by_type.reserve(world_state.zones.size());

  for (const zones::ZoneState& zone : world_state.zones) {
    if (zone.owner_settlement_id == settlement_id && zone.zone_type == zone_type) {
      zones_by_type.push_back(&zone);
    }
  }

  std::sort(zones_by_type.begin(), zones_by_type.end(),
            [](const zones::ZoneState* left, const zones::ZoneState* right) {
              return left->zone_id < right->zone_id;
            });
  return zones_by_type;
}

bool has_building(const SettlementState& settlement, const BuildingType building_type) noexcept {
  for (const BuildingState& building : settlement.buildings) {
    if (building.building_type == building_type) {
      return building.exists;
    }
  }

  return false;
}

BuildingState* find_building(SettlementState& settlement, const BuildingType building_type) noexcept {
  for (BuildingState& building : settlement.buildings) {
    if (building.building_type == building_type) {
      return &building;
    }
  }

  return nullptr;
}

ResourceAmountTenths annual_food_need_tenths(const SettlementState& settlement) noexcept {
  return settlement.population_whole * kFoodConsumptionPerPopTenths * 12;
}

ResourceAmountTenths annual_plot_harvest_tenths(const FarmPlotState& plot, const bool estate_staffed) {
  const ResourceAmountTenths base_harvest =
      80 + static_cast<ResourceAmountTenths>((12 * static_cast<int32_t>(plot.avg_fertility_tenths) + 50) /
                                            100);
  return estate_staffed ? (base_harvest * 125) / 100 : base_harvest;
}

int32_t monthly_plot_demand(const MonthIndex month, const FarmPlotState& plot) {
  if (plot.state != FarmPlotStateCode::Active || month >= kPlotMonthlyLaborDemand.size()) {
    return 0;
  }
  return kPlotMonthlyLaborDemand[month];
}

int32_t construction_demand_for_settlement(const world::WorldState& world_state,
                                           const SettlementId settlement_id) {
  int32_t total_demand = 0;
  for (const projects::ProjectState& project : world_state.projects) {
    if (project.owner_settlement_id != settlement_id ||
        project.status == ProjectStatus::Completed ||
        project.priority == PriorityLabel::Paused) {
      continue;
    }

    total_demand += std::max<WorkAmountTenths>(0, project.remaining_common_work) /
                    kConstructionWorkPerLaborTenths;
    if (project.remaining_common_work % kConstructionWorkPerLaborTenths != 0) {
      ++total_demand;
    }
  }

  return total_demand;
}

void normalize_population(SettlementState& settlement, int32_t& whole_delta) {
  while (settlement.population_change_basis_points >= 10000) {
    settlement.population_change_basis_points -= 10000;
    ++settlement.population_whole;
    ++whole_delta;
  }

  while (settlement.population_change_basis_points <= -10000) {
    if (settlement.population_whole <= 0) {
      settlement.population_change_basis_points = 0;
      break;
    }

    settlement.population_change_basis_points += 10000;
    --settlement.population_whole;
    --whole_delta;
  }

  if (settlement.population_whole <= 0) {
    settlement.population_whole = 0;
    settlement.population_change_basis_points = 0;
  }

  settlement.population_fraction_tenths = settlement.population_change_basis_points / 1000;
}

FarmPlotState* find_best_openable_plot(const std::vector<FarmPlotState*>& plots) {
  for (FarmPlotState* plot : plots) {
    if (plot->state == FarmPlotStateCode::Fallow) {
      return plot;
    }
  }

  for (FarmPlotState* plot : plots) {
    if (plot->state == FarmPlotStateCode::Unopened) {
      return plot;
    }
  }

  return nullptr;
}

FarmPlotState* find_lowest_priority_active_plot(const std::vector<FarmPlotState*>& plots) {
  std::vector<FarmPlotState*> active_plots;
  active_plots.reserve(plots.size());

  for (FarmPlotState* plot : plots) {
    if (plot->state == FarmPlotStateCode::Active) {
      active_plots.push_back(plot);
    }
  }

  if (active_plots.empty()) {
    return nullptr;
  }

  std::sort(active_plots.begin(), active_plots.end(),
            [](const FarmPlotState* left, const FarmPlotState* right) {
              return plot_priority_reverse_less(left, right);
            });
  return active_plots.front();
}

void begin_plot_opening(FarmPlotState& plot) {
  plot.state = FarmPlotStateCode::Opening;
  plot.opening_months_remaining = plot.forested_flag ? 2 : 1;
  plot.opening_work_remaining_tenths = plot.forested_flag ? 150 : 100;
}

void begin_plot_reactivation(FarmPlotState& plot) {
  plot.state = FarmPlotStateCode::Opening;
  plot.opening_months_remaining = 1;
  plot.opening_work_remaining_tenths = kFallowReactivationWorkTenths;
}

}  // namespace

MonthlySettlementSimulationResult advance_monthly_settlements(world::WorldState& world_state) {
  MonthlySettlementSimulationResult result;
  std::vector<SettlementPhaseState> phase_states;
  phase_states.reserve(world_state.settlements.size());

  std::vector<SettlementState*> settlements_in_order;
  settlements_in_order.reserve(world_state.settlements.size());
  for (SettlementState& settlement : world_state.settlements) {
    settlements_in_order.push_back(&settlement);
  }
  std::sort(settlements_in_order.begin(), settlements_in_order.end(),
            [](const SettlementState* left, const SettlementState* right) {
              return left->settlement_id < right->settlement_id;
            });

  for (SettlementState* settlement : settlements_in_order) {
    SettlementPhaseState phase_state;
    phase_state.settlement = settlement;
    phase_state.starting_stockpile = settlement->stockpile;
    phase_state.construction_budget.settlement_id = settlement->settlement_id;

    BuildingState* estate_building = find_building(*settlement, BuildingType::EstateI);
    const bool estate_exists = estate_building != nullptr && estate_building->exists;
    const int32_t noble_demand = estate_exists ? kEstateMinimumNobles : 0;
    phase_state.estate_staffed = noble_demand > 0 && settlement->population_whole > 0;
    if (estate_building != nullptr) {
      estate_building->staffed = phase_state.estate_staffed;
    }

    std::vector<FarmPlotState*> settlement_plots =
        collect_mutable_settlement_plots(world_state, settlement->settlement_id);
    for (FarmPlotState* plot : settlement_plots) {
      const int32_t demand = monthly_plot_demand(world_state.calendar.month, *plot);
      if (plot->state == FarmPlotStateCode::Active) {
        phase_state.total_active_farming_demand += demand;
        phase_state.active_plots.push_back({
            .plot = plot,
            .demand = demand,
            .assigned = 0,
        });
      } else if (plot->state == FarmPlotStateCode::Opening) {
        phase_state.total_opening_demand += 1;
        phase_state.opening_plots.push_back({
            .plot = plot,
            .demand = 1,
            .assigned = 0,
        });
      }
    }

    ResourceAmountTenths protected_food_target = annual_food_need_tenths(*settlement);
    ResourceAmountTenths protected_food_coverage = 0;
    for (const PlotAllocation& plot_allocation : phase_state.active_plots) {
      if (protected_food_coverage >= protected_food_target) {
        break;
      }

      protected_food_coverage +=
          annual_plot_harvest_tenths(*plot_allocation.plot, phase_state.estate_staffed);
      phase_state.protected_farming_demand += plot_allocation.demand;
    }

    const std::vector<const zones::ZoneState*> forestry_zones =
        collect_zones_by_type(world_state, settlement->settlement_id, ZoneType::Forestry);
    for (const zones::ZoneState* zone : forestry_zones) {
      const int32_t demand = static_cast<int32_t>(
          (zone->member_cell_indices.size() + static_cast<std::size_t>(kResourceCellsPerSerf - 1)) /
          static_cast<std::size_t>(kResourceCellsPerSerf));
      phase_state.total_forestry_demand += demand;
      phase_state.forestry_zones.push_back({
          .zone = zone,
          .demand = demand,
          .assigned = 0,
      });
    }

    const std::vector<const zones::ZoneState*> quarry_zones =
        collect_zones_by_type(world_state, settlement->settlement_id, ZoneType::Quarry);
    for (const zones::ZoneState* zone : quarry_zones) {
      const int32_t demand = static_cast<int32_t>(
          (zone->member_cell_indices.size() + static_cast<std::size_t>(kResourceCellsPerSerf - 1)) /
          static_cast<std::size_t>(kResourceCellsPerSerf));
      phase_state.total_quarry_demand += demand;
      phase_state.quarry_zones.push_back({
          .zone = zone,
          .demand = demand,
          .assigned = 0,
      });
    }

    phase_state.total_construction_demand =
        construction_demand_for_settlement(world_state, settlement->settlement_id);

    const int32_t noble_fill = std::min(settlement->population_whole, noble_demand);
    int32_t remaining_serfs = std::max(0, settlement->population_whole - noble_fill);
    int32_t assigned_farming = 0;

    for (PlotAllocation& plot_allocation : phase_state.active_plots) {
      plot_allocation.assigned = std::min(plot_allocation.demand, remaining_serfs);
      remaining_serfs -= plot_allocation.assigned;
      assigned_farming += plot_allocation.assigned;
    }

    for (PlotAllocation& plot_allocation : phase_state.opening_plots) {
      if (remaining_serfs <= 0) {
        break;
      }

      plot_allocation.assigned = std::min(plot_allocation.demand, remaining_serfs);
      remaining_serfs -= plot_allocation.assigned;
    }

    for (ForestryAllocation& forestry_allocation : phase_state.forestry_zones) {
      if (remaining_serfs <= 0) {
        break;
      }

      forestry_allocation.assigned = std::min(forestry_allocation.demand, remaining_serfs);
      remaining_serfs -= forestry_allocation.assigned;
    }

    for (QuarryAllocation& quarry_allocation : phase_state.quarry_zones) {
      if (remaining_serfs <= 0) {
        break;
      }

      quarry_allocation.assigned = std::min(quarry_allocation.demand, remaining_serfs);
      remaining_serfs -= quarry_allocation.assigned;
    }

    const int32_t construction_fill =
        std::min({phase_state.total_construction_demand, remaining_serfs, kConstructionLaborCap});
    remaining_serfs -= construction_fill;
    phase_state.construction_budget.common_work_tenths =
        construction_fill * kConstructionWorkPerLaborTenths;
    phase_state.construction_budget.skilled_work_tenths = 0;

    settlement->labor_state.last_noble_fill = noble_fill;
    settlement->labor_state.last_artisan_fill = 0;
    settlement->labor_state.last_serf_fill = settlement->population_whole - noble_fill;
    settlement->labor_state.reassignment_budget_tenths = settlement->population_whole * 10;
    settlement->labor_state.protected_food_floor_serfs = phase_state.protected_farming_demand;
    settlement->labor_state.protected_base_demand = noble_demand + phase_state.protected_farming_demand;
    settlement->labor_state.extra_role_demand =
        std::max(0, phase_state.total_active_farming_demand - phase_state.protected_farming_demand) +
        phase_state.total_opening_demand + phase_state.total_forestry_demand +
        phase_state.total_quarry_demand + phase_state.total_construction_demand;
    settlement->labor_state.idle_fill = remaining_serfs;

    result.construction_budgets.push_back(phase_state.construction_budget);
    result.season_and_labor_ticks +=
        1U + static_cast<uint32_t>(phase_state.active_plots.size() + phase_state.opening_plots.size() +
                                   phase_state.forestry_zones.size() + phase_state.quarry_zones.size());
    phase_states.push_back(std::move(phase_state));
  }

  for (SettlementPhaseState& phase_state : phase_states) {
    SettlementState& settlement = *phase_state.settlement;

    for (PlotAllocation& plot_allocation : phase_state.active_plots) {
      if (plot_allocation.demand > 0 &&
          world_state.calendar.month >= 3 && world_state.calendar.month <= 10) {
        plot_allocation.plot->current_year_required_labor += plot_allocation.demand;
        plot_allocation.plot->current_year_assigned_labor += plot_allocation.assigned;
        if (plot_allocation.plot->current_year_required_labor > 0) {
          plot_allocation.plot->labor_coverage_tenths = static_cast<uint16_t>(
              std::min(1000, (plot_allocation.plot->current_year_assigned_labor * 1000) /
                                 plot_allocation.plot->current_year_required_labor));
        }
      }

      if (world_state.calendar.month == 10 && plot_allocation.plot->state == FarmPlotStateCode::Active) {
        const ResourceAmountTenths base_harvest =
            annual_plot_harvest_tenths(*plot_allocation.plot, false);
        ResourceAmountTenths final_harvest =
            (base_harvest * static_cast<ResourceAmountTenths>(plot_allocation.plot->labor_coverage_tenths)) /
            1000;
        if (phase_state.estate_staffed) {
          final_harvest = (final_harvest * 125) / 100;
        }
        settlement.stockpile.food += final_harvest;
      }
    }

    for (PlotAllocation& plot_allocation : phase_state.opening_plots) {
      if (plot_allocation.assigned <= 0) {
        continue;
      }

      plot_allocation.plot->opening_months_remaining =
          std::max(0, plot_allocation.plot->opening_months_remaining - 1);
      plot_allocation.plot->opening_work_remaining_tenths =
          std::max<WorkAmountTenths>(0, plot_allocation.plot->opening_work_remaining_tenths -
                                            (plot_allocation.assigned * 100));
      if (plot_allocation.plot->opening_months_remaining == 0 &&
          plot_allocation.plot->opening_work_remaining_tenths == 0) {
        plot_allocation.plot->state = FarmPlotStateCode::Active;
        append_plot_dirty_chunks(world_state, *plot_allocation.plot, result.dirty_chunks);
        result.dirty_overlays.push_back(OverlayType::FarmlandPlots);
      }
    }

    for (const ForestryAllocation& forestry_allocation : phase_state.forestry_zones) {
      if (forestry_allocation.assigned <= 0 || forestry_allocation.demand <= 0) {
        continue;
      }

      const ResourceAmountTenths full_output =
          static_cast<ResourceAmountTenths>(forestry_allocation.zone->member_cell_indices.size() * 10U);
      settlement.stockpile.wood += (full_output * forestry_allocation.assigned) /
                                   forestry_allocation.demand;
    }

    for (const QuarryAllocation& quarry_allocation : phase_state.quarry_zones) {
      if (quarry_allocation.assigned <= 0 || quarry_allocation.demand <= 0) {
        continue;
      }

      const ResourceAmountTenths full_output =
          static_cast<ResourceAmountTenths>(quarry_allocation.zone->member_cell_indices.size() * 6U);
      settlement.stockpile.stone += (full_output * quarry_allocation.assigned) /
                                    quarry_allocation.demand;
    }

    result.production_ticks +=
        1U + static_cast<uint32_t>(phase_state.active_plots.size() + phase_state.opening_plots.size() +
                                   phase_state.forestry_zones.size() + phase_state.quarry_zones.size());
  }

  result.logistics_ticks = static_cast<uint32_t>(std::max<std::size_t>(1, phase_states.size()));

  for (SettlementPhaseState& phase_state : phase_states) {
    SettlementState& settlement = *phase_state.settlement;
    const ResourceAmountTenths food_required =
        settlement.population_whole * kFoodConsumptionPerPopTenths;
    const ResourceAmountTenths food_consumed = std::min(settlement.stockpile.food, food_required);
    settlement.stockpile.food -= food_consumed;

    const bool food_shortage = food_consumed < food_required;
    const int32_t coverage_tenths =
        food_required > 0 ? static_cast<int32_t>((food_consumed * 1000) / food_required) : 1000;
    settlement.food_shortage_flag = food_shortage;
    phase_state.food_shortage = food_shortage;

    if (food_shortage) {
      result.shortage_settlements.push_back(settlement.settlement_id);
      result.dirty_overlays.push_back(OverlayType::LogisticsShortage);
    }

    const int32_t basis_points_before = settlement.population_change_basis_points;
    bool starvation = false;
    if (!food_shortage) {
      settlement.population_change_basis_points += settlement.population_whole * 20;
    } else if (coverage_tenths < 750) {
      starvation = true;
      const int32_t starvation_rate =
          coverage_tenths >= 500 ? 100 : (coverage_tenths >= 250 ? 300 : 600);
      settlement.population_change_basis_points -= settlement.population_whole * starvation_rate;
    }

    int32_t whole_delta = 0;
    normalize_population(settlement, whole_delta);
    const int32_t basis_points_delta = settlement.population_change_basis_points - basis_points_before +
                                       (whole_delta * 10000);
    const int32_t fractional_delta_tenths =
        basis_points_delta >= 0 ? basis_points_delta / 1000 : -((-basis_points_delta) / 1000);

    if (whole_delta != 0 || fractional_delta_tenths != 0 || starvation) {
      result.population_deltas.push_back({
          .settlement_id = settlement.settlement_id,
          .whole_delta = whole_delta,
          .fractional_delta_tenths = fractional_delta_tenths,
          .starvation_occurred = starvation,
      });
    }

    result.consumption_ticks += 1U;
  }

  for (SettlementPhaseState& phase_state : phase_states) {
    SettlementState& settlement = *phase_state.settlement;
    const bool review_month = world_state.calendar.month == 11;
    if (review_month) {
      std::vector<FarmPlotState*> plots =
          collect_mutable_settlement_plots(world_state, settlement.settlement_id);

      bool changed_plot_state = false;
      bool undercovered_active_plot = false;
      for (FarmPlotState* plot : plots) {
        if (plot->state == FarmPlotStateCode::Active && plot->labor_coverage_tenths < 1000) {
          undercovered_active_plot = true;
          break;
        }
      }

      if (undercovered_active_plot) {
        FarmPlotState* retreat_plot = find_lowest_priority_active_plot(plots);
        if (retreat_plot != nullptr) {
          retreat_plot->state = FarmPlotStateCode::Fallow;
          retreat_plot->opening_months_remaining = 0;
          retreat_plot->opening_work_remaining_tenths = 0;
          append_plot_dirty_chunks(world_state, *retreat_plot, result.dirty_chunks);
          changed_plot_state = true;
        }
      } else {
        FarmPlotState* open_plot = find_best_openable_plot(plots);
        if (open_plot != nullptr) {
          if (open_plot->state == FarmPlotStateCode::Fallow) {
            begin_plot_reactivation(*open_plot);
          } else {
            begin_plot_opening(*open_plot);
          }
          append_plot_dirty_chunks(world_state, *open_plot, result.dirty_chunks);
          changed_plot_state = true;
        }
      }

      for (FarmPlotState* plot : plots) {
        plot->current_year_required_labor = 0;
        plot->current_year_assigned_labor = 0;
        plot->labor_coverage_tenths = 0;
      }

      if (changed_plot_state) {
        result.dirty_overlays.push_back(OverlayType::FarmlandPlots);
      }
    }

    result.stockpile_deltas.push_back({
        .settlement_id = settlement.settlement_id,
        .food_delta = settlement.stockpile.food - phase_state.starting_stockpile.food,
        .wood_delta = settlement.stockpile.wood - phase_state.starting_stockpile.wood,
        .stone_delta = settlement.stockpile.stone - phase_state.starting_stockpile.stone,
    });

    result.settlement_update_ticks +=
        1U + static_cast<uint32_t>(phase_state.active_plots.size() + phase_state.opening_plots.size());
  }

  sort_and_deduplicate_settlements(result.shortage_settlements);
  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  return result;
}

}  // namespace alpha::settlements
