#pragma once

#include <vector>

#include "alpha/api/result_types.hpp"

namespace alpha::projects {
struct ConstructionAdvanceResult;
struct ConstructionLaborBudget;
}

namespace alpha::world {
struct WorldState;
}

namespace alpha::settlements {

struct MonthlySettlementSimulationResult {
  std::vector<ResourceDelta> stockpile_deltas;
  std::vector<PopulationDelta> population_deltas;
  std::vector<SettlementId> shortage_settlements;
  std::vector<OverlayType> dirty_overlays;
  std::vector<ChunkCoord> dirty_chunks;
  std::vector<projects::ConstructionLaborBudget> construction_budgets;
  uint32_t season_and_labor_ticks = 0;
  uint32_t production_ticks = 0;
  uint32_t logistics_ticks = 0;
  uint32_t consumption_ticks = 0;
  uint32_t settlement_update_ticks = 0;
};

MonthlySettlementSimulationResult advance_monthly_settlements(world::WorldState& world_state);

}  // namespace alpha::settlements
