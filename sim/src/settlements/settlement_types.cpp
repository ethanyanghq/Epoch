#include "alpha/settlements/settlement_types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "alpha/map/map_types.hpp"

namespace alpha::settlements {
namespace {

constexpr SettlementId kStartingSettlementId{1};
constexpr int32_t kSettlementFootprintRadius = 2;
constexpr int32_t kSettlementFootprintWidth = kSettlementFootprintRadius * 2 + 1;
constexpr int32_t kRequiredDryFoundingCells = 16;
constexpr ResourceAmountTenths kStartingFoodTenths = 400;
constexpr ResourceAmountTenths kStartingWoodTenths = 200;
constexpr ResourceAmountTenths kStartingStoneTenths = 100;

std::array<BuildingState, 2> make_starting_buildings() noexcept {
  return {
      BuildingState{
          .building_type = BuildingType::EstateI,
          .exists = true,
          .staffed = false,
      },
      BuildingState{
          .building_type = BuildingType::WarehouseI,
          .exists = false,
          .staffed = false,
      },
  };
}

bool is_dry_buildable_cell(const map::MapCell& cell) noexcept {
  return cell.water == 0 && cell.slope <= 2;
}

bool is_valid_footprint_center(const map::MapGrid& map_grid, const int32_t center_x,
                               const int32_t center_y) noexcept {
  int32_t dry_buildable_cells = 0;

  for (int32_t offset_y = -kSettlementFootprintRadius; offset_y <= kSettlementFootprintRadius;
       ++offset_y) {
    for (int32_t offset_x = -kSettlementFootprintRadius; offset_x <= kSettlementFootprintRadius;
         ++offset_x) {
      const int32_t world_x = center_x + offset_x;
      const int32_t world_y = center_y + offset_y;
      if (!map_grid.contains_cell(world_x, world_y)) {
        return false;
      }

      const map::MapCell& cell = map_grid.cell(world_x, world_y);
      dry_buildable_cells += is_dry_buildable_cell(cell) ? 1 : 0;
    }
  }

  return dry_buildable_cells >= kRequiredDryFoundingCells &&
         is_dry_buildable_cell(map_grid.cell(center_x, center_y));
}

CellCoord find_starting_center(const map::MapGrid& map_grid) noexcept {
  const int32_t origin_x = static_cast<int32_t>(map_grid.width()) / 2;
  const int32_t origin_y = static_cast<int32_t>(map_grid.height()) / 2;
  const int32_t max_radius = std::max(origin_x, origin_y);

  for (int32_t radius = 0; radius <= max_radius; ++radius) {
    const int32_t min_x = origin_x - radius;
    const int32_t max_x = origin_x + radius;
    const int32_t min_y = origin_y - radius;
    const int32_t max_y = origin_y + radius;

    for (int32_t y = min_y; y <= max_y; ++y) {
      for (int32_t x = min_x; x <= max_x; ++x) {
        const int32_t chebyshev_distance =
            std::max(std::abs(x - origin_x), std::abs(y - origin_y));
        if (chebyshev_distance != radius) {
          continue;
        }

        if (is_valid_footprint_center(map_grid, x, y)) {
          return {.x = x, .y = y};
        }
      }
    }
  }

  return {.x = origin_x, .y = origin_y};
}

std::vector<uint32_t> build_footprint_cell_indices(const map::MapGrid& map_grid,
                                                   const CellCoord center) {
  std::vector<uint32_t> footprint_cell_indices;
  footprint_cell_indices.reserve(
      static_cast<std::size_t>(kSettlementFootprintWidth * kSettlementFootprintWidth));

  for (int32_t offset_y = -kSettlementFootprintRadius; offset_y <= kSettlementFootprintRadius;
       ++offset_y) {
    for (int32_t offset_x = -kSettlementFootprintRadius; offset_x <= kSettlementFootprintRadius;
         ++offset_x) {
      const int32_t world_x = center.x + offset_x;
      const int32_t world_y = center.y + offset_y;
      if (!map_grid.contains_cell(world_x, world_y)) {
        continue;
      }

      footprint_cell_indices.push_back(
          static_cast<uint32_t>(map::flatten_cell_index(map_grid.width(), world_x, world_y)));
    }
  }

  return footprint_cell_indices;
}

}  // namespace

SettlementState make_starting_settlement(const map::MapGrid& map_grid) {
  const CellCoord center = find_starting_center(map_grid);

  return {
      .settlement_id = kStartingSettlementId,
      .center = center,
      .footprint_cell_indices = build_footprint_cell_indices(map_grid, center),
      .population_whole = 20,
      .population_fraction_tenths = 0,
      .development_pressure_tenths = 0,
      .stockpile =
          {
              .food = kStartingFoodTenths,
              .wood = kStartingWoodTenths,
              .stone = kStartingStoneTenths,
          },
      .buildings = make_starting_buildings(),
      .labor_state = {},
      .founding_source_settlement_id = {},
      .has_founding_source = false,
      .food_shortage_flag = false,
  };
}

const SettlementState* find_settlement(const std::vector<SettlementState>& settlements,
                                       const SettlementId settlement_id) noexcept {
  for (const SettlementState& settlement : settlements) {
    if (settlement.settlement_id == settlement_id) {
      return &settlement;
    }
  }

  return nullptr;
}

SettlementState* find_settlement(std::vector<SettlementState>& settlements,
                                 const SettlementId settlement_id) noexcept {
  for (SettlementState& settlement : settlements) {
    if (settlement.settlement_id == settlement_id) {
      return &settlement;
    }
  }

  return nullptr;
}

SettlementSummary build_settlement_summary(const SettlementState& settlement_state) noexcept {
  SettlementSummary summary{
      .settlement_id = settlement_state.settlement_id,
      .center = settlement_state.center,
      .population_whole = settlement_state.population_whole,
      .food = settlement_state.stockpile.food,
      .wood = settlement_state.stockpile.wood,
      .stone = settlement_state.stockpile.stone,
      .active_zone_count = static_cast<uint32_t>(settlement_state.owned_zone_ids.size()),
      .food_shortage_flag = settlement_state.food_shortage_flag,
  };

  for (std::size_t index = 0; index < settlement_state.buildings.size(); ++index) {
    const BuildingState& building = settlement_state.buildings[index];
    summary.buildings[index] = BuildingStateView{
        .building_type = building.building_type,
        .exists = building.exists,
        .staffed = building.staffed,
    };
  }

  return summary;
}

}  // namespace alpha::settlements
