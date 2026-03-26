#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "alpha/api/result_types.hpp"
#include "alpha/map/map_grid.hpp"

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

struct SettlementState {
  SettlementId settlement_id;
  CellCoord center;
  std::vector<uint32_t> footprint_cell_indices;
  int32_t population_whole = 0;
  int32_t population_fraction_tenths = 0;
  int32_t development_pressure_tenths = 0;
  StockpileState stockpile;
  std::array<BuildingState, 2> buildings{};
  bool food_shortage_flag = false;
};

SettlementState make_starting_settlement(const map::MapGrid& map_grid);
const SettlementState* find_settlement(const std::vector<SettlementState>& settlements,
                                       SettlementId settlement_id) noexcept;
SettlementSummary build_settlement_summary(const SettlementState& settlement_state) noexcept;

}  // namespace alpha::settlements
