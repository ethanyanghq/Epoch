#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "alpha/api/result_types.hpp"
#include "alpha/map/map_grid.hpp"
#include "alpha/world/ids.hpp"

namespace alpha::settlements {

struct StockpileState {
  ResourceAmountTenths food = 0;
  ResourceAmountTenths wood = 0;
  ResourceAmountTenths stone = 0;
};

struct BuildingState {
  BuildingType building_type = BuildingType::EstateI;
  bool exists = false;
  bool staffed = false;
};

struct LaborState {
  int32_t last_serf_fill = 0;
  int32_t last_artisan_fill = 0;
  int32_t last_noble_fill = 0;
  int32_t reassignment_budget_tenths = 0;
  int32_t protected_food_floor_serfs = 0;
};

struct SettlementState {
  SettlementId settlement_id;
  CellCoord center;
  std::vector<uint32_t> footprint_cell_indices;
  int32_t population_whole = 0;
  int32_t population_fraction_tenths = 0;
  int32_t development_pressure_tenths = 0;
  StockpileState stockpile;
  std::array<BuildingState, 2> buildings{};
  LaborState labor_state;
  SettlementId founding_source_settlement_id;
  bool has_founding_source = false;
  bool food_shortage_flag = false;
  std::vector<ZoneId> owned_zone_ids;
};

SettlementState make_starting_settlement(const map::MapGrid& map_grid);
const SettlementState* find_settlement(const std::vector<SettlementState>& settlements,
                                       SettlementId settlement_id) noexcept;
SettlementState* find_settlement(std::vector<SettlementState>& settlements,
                                 SettlementId settlement_id) noexcept;
SettlementSummary build_settlement_summary(const SettlementState& settlement_state) noexcept;

}  // namespace alpha::settlements
