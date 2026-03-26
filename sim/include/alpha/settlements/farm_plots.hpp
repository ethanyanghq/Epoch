#pragma once

#include <cstdint>
#include <vector>

#include "alpha/api/result_types.hpp"

namespace alpha::world {
struct WorldState;
}

namespace alpha::settlements {

enum class FarmPlotStateCode : uint8_t {
  Unopened,
  Opening,
  Active,
  Fallow
};

struct FarmPlotState {
  FarmPlotId plot_id;
  ZoneId parent_zone_id;
  std::vector<uint32_t> cell_indices;
  FarmPlotStateCode state = FarmPlotStateCode::Unopened;
  uint16_t avg_fertility_tenths = 0;
  uint16_t avg_access_cost_tenths = 0;
  bool forested_flag = false;
  uint16_t labor_coverage_tenths = 0;
  int32_t current_year_required_labor = 0;
  int32_t current_year_assigned_labor = 0;
  int32_t opening_months_remaining = 0;
  WorkAmountTenths opening_work_remaining_tenths = 0;
};

void rebuild_world_farm_plots(world::WorldState& world_state);
const FarmPlotState* find_farm_plot(const world::WorldState& world_state,
                                    FarmPlotId plot_id) noexcept;
std::vector<const FarmPlotState*> collect_settlement_farm_plots(const world::WorldState& world_state,
                                                                SettlementId settlement_id);
std::vector<FarmPlotView> build_farm_plot_views(const world::WorldState& world_state,
                                                SettlementId settlement_id);
const char* farm_plot_state_name(FarmPlotStateCode state) noexcept;

}  // namespace alpha::settlements
