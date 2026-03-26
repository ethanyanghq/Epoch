#pragma once

#include <cstdint>
#include <vector>

#include "alpha/api/result_types.hpp"

namespace alpha::world {
struct WorldState;
}

namespace alpha::projects {

struct ConstructionLaborBudget {
  SettlementId settlement_id;
  WorkAmountTenths common_work_tenths = 0;
  WorkAmountTenths skilled_work_tenths = 0;
};

struct ConstructionAdvanceResult {
  std::vector<ProjectId> completed_projects;
  std::vector<ProjectId> newly_blocked_projects;
  std::vector<ProjectId> newly_unblocked_projects;
  std::vector<ChunkCoord> dirty_chunks;
  std::vector<OverlayType> dirty_overlays;
};

bool is_road_built(const world::WorldState& world_state, uint32_t cell_index) noexcept;
void mark_road_built(world::WorldState& world_state, uint32_t cell_index) noexcept;
ConstructionAdvanceResult advance_monthly_construction(
    world::WorldState& world_state, const std::vector<ConstructionLaborBudget>& labor_budgets);

}  // namespace alpha::projects
