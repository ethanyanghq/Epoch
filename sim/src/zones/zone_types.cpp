#include "alpha/zones/zone_types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "alpha/api/constants.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/projects/project_progress.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::zones {
namespace {

constexpr int32_t kMinimumZoneComponentSize = 8;
constexpr int32_t kOperationalReachThresholdCenti = 3200;
constexpr uint8_t kForestedVegetationThreshold = 50;
constexpr uint8_t kZoneOwnerOverlayUnzoned = 0;

struct SettlementZoneSnapshot {
  SettlementId settlement_id;
  std::vector<ZoneId> owned_zone_ids;
};

struct ZoneComponentDraft {
  SettlementId owner_settlement_id;
  ZoneType zone_type = ZoneType::Farmland;
  std::vector<uint32_t> member_cell_indices;
};

struct ValidationResult {
  bool ok = false;
  CommandRejectReason reject_reason = CommandRejectReason::Unknown;
  std::string reject_message;
  const settlements::SettlementState* settlement = nullptr;
  std::vector<uint32_t> cell_indices;
};

uint32_t to_cell_index(const world::WorldState& world_state, const int32_t x, const int32_t y) {
  return static_cast<uint32_t>(map::flatten_cell_index(world_state.map_width, x, y));
}

CellCoord to_cell_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const int32_t width = static_cast<int32_t>(world_state.map_width);
  return {
      .x = static_cast<int32_t>(cell_index % static_cast<uint32_t>(width)),
      .y = static_cast<int32_t>(cell_index / static_cast<uint32_t>(width)),
  };
}

ChunkCoord to_chunk_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const CellCoord coord = to_cell_coord(world_state, cell_index);
  return {
      .x = static_cast<int16_t>(coord.x / kChunkSize),
      .y = static_cast<int16_t>(coord.y / kChunkSize),
  };
}

void clear_cell_zone(CellZoneState& cell_zone) noexcept {
  cell_zone.occupied = false;
  cell_zone.zone_id = {};
  cell_zone.owner_settlement_id = {};
  cell_zone.zone_type = ZoneType::Farmland;
}

const ZoneState* find_zone(const std::vector<ZoneState>& zones, const ZoneId zone_id) noexcept {
  for (const ZoneState& zone : zones) {
    if (zone.zone_id == zone_id) {
      return &zone;
    }
  }

  return nullptr;
}

std::vector<SettlementZoneSnapshot> snapshot_settlement_zones(
    const std::vector<settlements::SettlementState>& settlements) {
  std::vector<SettlementZoneSnapshot> snapshots;
  snapshots.reserve(settlements.size());

  for (const settlements::SettlementState& settlement : settlements) {
    snapshots.push_back({
        .settlement_id = settlement.settlement_id,
        .owned_zone_ids = settlement.owned_zone_ids,
    });
  }

  return snapshots;
}

bool same_zone_ownership(const SettlementZoneSnapshot& snapshot,
                         const settlements::SettlementState& settlement) {
  return snapshot.settlement_id == settlement.settlement_id &&
         snapshot.owned_zone_ids == settlement.owned_zone_ids;
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

void sort_and_deduplicate_zone_ids(std::vector<ZoneId>& zone_ids) {
  std::sort(zone_ids.begin(), zone_ids.end(),
            [](const ZoneId left, const ZoneId right) { return left < right; });
  zone_ids.erase(std::unique(zone_ids.begin(), zone_ids.end()), zone_ids.end());
}

void append_world_dirty_chunks(world::WorldState& world_state,
                               const std::vector<ChunkCoord>& dirty_chunks) {
  world_state.dirty_chunks.insert(world_state.dirty_chunks.end(), dirty_chunks.begin(),
                                  dirty_chunks.end());
  sort_and_deduplicate_chunks(world_state.dirty_chunks);
}

std::vector<uint32_t> normalize_cell_indices(const world::WorldState& world_state,
                                             const std::vector<CellCoord>& cells,
                                             bool& all_in_bounds) {
  all_in_bounds = true;
  std::vector<uint32_t> cell_indices;
  cell_indices.reserve(cells.size());

  for (const CellCoord& cell : cells) {
    if (!world_state.map_grid.contains_cell(cell.x, cell.y)) {
      all_in_bounds = false;
      continue;
    }

    cell_indices.push_back(to_cell_index(world_state, cell.x, cell.y));
  }

  std::sort(cell_indices.begin(), cell_indices.end());
  cell_indices.erase(std::unique(cell_indices.begin(), cell_indices.end()), cell_indices.end());
  return cell_indices;
}

bool is_footprint_cell(const world::WorldState& world_state, const uint32_t cell_index) noexcept {
  for (const settlements::SettlementState& settlement : world_state.settlements) {
    if (std::binary_search(settlement.footprint_cell_indices.begin(),
                           settlement.footprint_cell_indices.end(), cell_index)) {
      return true;
    }
  }

  return false;
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
                                         const settlements::SettlementState& settlement) {
  const std::size_t cell_count = static_cast<std::size_t>(world_state.map_width) *
                                 static_cast<std::size_t>(world_state.map_height);
  std::vector<int32_t> costs(cell_count, std::numeric_limits<int32_t>::max());

  if (!world_state.map_grid.contains_cell(settlement.center.x, settlement.center.y)) {
    return costs;
  }

  using QueueEntry = std::pair<int32_t, uint32_t>;
  std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;
  const uint32_t start_index =
      to_cell_index(world_state, settlement.center.x, settlement.center.y);
  costs[start_index] = 0;
  frontier.push({0, start_index});

  constexpr std::array<std::pair<int32_t, int32_t>, 4> kNeighborOffsets = {
      std::pair{-1, 0},
      std::pair{1, 0},
      std::pair{0, -1},
      std::pair{0, 1},
  };

  while (!frontier.empty()) {
    const auto [current_cost, current_index] = frontier.top();
    frontier.pop();
    if (current_cost != costs[current_index] || current_cost > kOperationalReachThresholdCenti) {
      continue;
    }

    const CellCoord current_coord = to_cell_coord(world_state, current_index);
    for (const auto [offset_x, offset_y] : kNeighborOffsets) {
      const int32_t neighbor_x = current_coord.x + offset_x;
      const int32_t neighbor_y = current_coord.y + offset_y;
      if (!world_state.map_grid.contains_cell(neighbor_x, neighbor_y)) {
        continue;
      }

      const map::MapCell& neighbor_cell = world_state.map_grid.cell(neighbor_x, neighbor_y);
      const uint32_t neighbor_index = to_cell_index(world_state, neighbor_x, neighbor_y);
      const int32_t step_cost = movement_cost_centi(world_state, neighbor_index, neighbor_cell);
      if (step_cost == std::numeric_limits<int32_t>::max()) {
        continue;
      }

      const int32_t neighbor_cost = current_cost + step_cost;
      if (neighbor_cost > kOperationalReachThresholdCenti) {
        continue;
      }

      if (neighbor_cost >= costs[neighbor_index]) {
        continue;
      }

      costs[neighbor_index] = neighbor_cost;
      frontier.push({neighbor_cost, neighbor_index});
    }
  }

  return costs;
}

bool is_farmland_legal(const map::MapCell& cell) noexcept {
  return cell.water == 0U && cell.slope <= 2U;
}

bool is_forestry_legal(const map::MapCell& cell) noexcept {
  return cell.water == 0U && cell.slope <= 3U && cell.vegetation >= kForestedVegetationThreshold;
}

bool is_quarry_legal(const map::MapCell& cell) noexcept {
  return cell.water == 0U && cell.slope >= 1U && cell.slope <= 3U && cell.fertility <= 25U;
}

bool is_zone_target_legal(const world::WorldState& world_state,
                          const std::vector<int32_t>& reach_costs, const ZoneType zone_type,
                          const uint32_t cell_index) {
  if (cell_index >= reach_costs.size() ||
      reach_costs[cell_index] == std::numeric_limits<int32_t>::max() ||
      is_footprint_cell(world_state, cell_index)) {
    return false;
  }

  const CellCoord coord = to_cell_coord(world_state, cell_index);
  const map::MapCell& cell = world_state.map_grid.cell(coord.x, coord.y);
  switch (zone_type) {
    case ZoneType::Farmland:
      return is_farmland_legal(cell);
    case ZoneType::Forestry:
      return is_forestry_legal(cell);
    case ZoneType::Quarry:
      return is_quarry_legal(cell);
  }

  return false;
}

ValidationResult validate_zone_command_cells(const world::WorldState& world_state,
                                             const SettlementId settlement_id,
                                             const std::vector<CellCoord>& cells,
                                             const char* invalid_cell_message) {
  ValidationResult validation;
  validation.settlement = settlements::find_settlement(world_state.settlements, settlement_id);
  if (validation.settlement == nullptr) {
    validation.reject_reason = CommandRejectReason::InvalidSettlement;
    validation.reject_message = "The settlement id does not exist.";
    return validation;
  }

  bool all_in_bounds = true;
  validation.cell_indices = normalize_cell_indices(world_state, cells, all_in_bounds);
  if (!all_in_bounds || validation.cell_indices.empty()) {
    validation.reject_reason = CommandRejectReason::InvalidCells;
    validation.reject_message = invalid_cell_message;
    return validation;
  }

  validation.ok = true;
  return validation;
}

ZoneId choose_zone_id_for_component(const world::WorldState& world_state,
                                    const ZoneComponentDraft& component,
                                    std::vector<ZoneId>& consumed_old_zone_ids) {
  std::vector<std::pair<ZoneId, uint32_t>> overlaps;
  overlaps.reserve(component.member_cell_indices.size());

  for (const uint32_t cell_index : component.member_cell_indices) {
    const ZoneId previous_zone_id = world_state.zone_cells[cell_index].zone_id;
    if (previous_zone_id.value == 0U) {
      continue;
    }

    const ZoneState* previous_zone = find_zone(world_state.zones, previous_zone_id);
    if (previous_zone == nullptr || previous_zone->owner_settlement_id != component.owner_settlement_id ||
        previous_zone->zone_type != component.zone_type) {
      continue;
    }

    auto overlap_it =
        std::find_if(overlaps.begin(), overlaps.end(), [previous_zone_id](const auto& overlap) {
          return overlap.first == previous_zone_id;
        });
    if (overlap_it == overlaps.end()) {
      overlaps.push_back({previous_zone_id, 1U});
      continue;
    }

    ++overlap_it->second;
  }

  std::sort(overlaps.begin(), overlaps.end(),
            [](const auto& left, const auto& right) {
              if (left.second != right.second) {
                return left.second > right.second;
              }
              return left.first < right.first;
            });

  for (const auto& overlap : overlaps) {
    if (std::find(consumed_old_zone_ids.begin(), consumed_old_zone_ids.end(), overlap.first) ==
        consumed_old_zone_ids.end()) {
      consumed_old_zone_ids.push_back(overlap.first);
      return overlap.first;
    }
  }

  return {};
}

void rebuild_settlement_zone_ownership(world::WorldState& world_state) {
  for (settlements::SettlementState& settlement : world_state.settlements) {
    settlement.owned_zone_ids.clear();
  }

  std::sort(world_state.zones.begin(), world_state.zones.end(),
            [](const ZoneState& left, const ZoneState& right) { return left.zone_id < right.zone_id; });

  for (const ZoneState& zone : world_state.zones) {
    for (settlements::SettlementState& settlement : world_state.settlements) {
      if (settlement.settlement_id == zone.owner_settlement_id) {
        settlement.owned_zone_ids.push_back(zone.zone_id);
        break;
      }
    }
  }

  for (settlements::SettlementState& settlement : world_state.settlements) {
    sort_and_deduplicate_zone_ids(settlement.owned_zone_ids);
  }

  world_state.zone_count = static_cast<uint32_t>(world_state.zones.size());
}

std::vector<ZoneComponentDraft> build_zone_components(world::WorldState& world_state) {
  const std::size_t cell_count = world_state.zone_cells.size();
  std::vector<bool> visited(cell_count, false);
  std::vector<ZoneComponentDraft> components;

  constexpr std::array<std::pair<int32_t, int32_t>, 4> kNeighborOffsets = {
      std::pair{-1, 0},
      std::pair{1, 0},
      std::pair{0, -1},
      std::pair{0, 1},
  };

  for (uint32_t cell_index = 0; cell_index < cell_count; ++cell_index) {
    if (visited[cell_index] || !world_state.zone_cells[cell_index].occupied) {
      continue;
    }

    const CellZoneState seed = world_state.zone_cells[cell_index];
    ZoneComponentDraft component{
        .owner_settlement_id = seed.owner_settlement_id,
        .zone_type = seed.zone_type,
    };

    std::vector<uint32_t> frontier{cell_index};
    visited[cell_index] = true;

    for (std::size_t frontier_index = 0; frontier_index < frontier.size(); ++frontier_index) {
      const uint32_t current_index = frontier[frontier_index];
      component.member_cell_indices.push_back(current_index);

      const CellCoord coord = to_cell_coord(world_state, current_index);
      for (const auto [offset_x, offset_y] : kNeighborOffsets) {
        const int32_t neighbor_x = coord.x + offset_x;
        const int32_t neighbor_y = coord.y + offset_y;
        if (!world_state.map_grid.contains_cell(neighbor_x, neighbor_y)) {
          continue;
        }

        const uint32_t neighbor_index = to_cell_index(world_state, neighbor_x, neighbor_y);
        if (visited[neighbor_index]) {
          continue;
        }

        const CellZoneState& neighbor = world_state.zone_cells[neighbor_index];
        if (!neighbor.occupied || neighbor.owner_settlement_id != seed.owner_settlement_id ||
            neighbor.zone_type != seed.zone_type) {
          continue;
        }

        visited[neighbor_index] = true;
        frontier.push_back(neighbor_index);
      }
    }

    std::sort(component.member_cell_indices.begin(), component.member_cell_indices.end());
    if (component.member_cell_indices.size() < kMinimumZoneComponentSize) {
      for (const uint32_t member_index : component.member_cell_indices) {
        clear_cell_zone(world_state.zone_cells[member_index]);
      }
      continue;
    }

    components.push_back(std::move(component));
  }

  return components;
}

void assign_components_to_world(world::WorldState& world_state,
                                std::vector<ZoneComponentDraft> components) {
  std::vector<ZoneState> rebuilt_zones;
  rebuilt_zones.reserve(components.size());

  std::vector<ZoneId> consumed_old_zone_ids;
  for (const ZoneComponentDraft& component : components) {
    ZoneId zone_id = choose_zone_id_for_component(world_state, component, consumed_old_zone_ids);
    if (zone_id.value == 0U) {
      zone_id = world_state.next_zone_id;
      ++world_state.next_zone_id.value;
    }

    for (const uint32_t member_index : component.member_cell_indices) {
      world_state.zone_cells[member_index].zone_id = zone_id;
      world_state.zone_cells[member_index].owner_settlement_id = component.owner_settlement_id;
      world_state.zone_cells[member_index].zone_type = component.zone_type;
      world_state.zone_cells[member_index].occupied = true;
    }

    rebuilt_zones.push_back({
        .zone_id = zone_id,
        .owner_settlement_id = component.owner_settlement_id,
        .zone_type = component.zone_type,
        .member_cell_indices = component.member_cell_indices,
    });
  }

  for (CellZoneState& cell_zone : world_state.zone_cells) {
    if (!cell_zone.occupied) {
      clear_cell_zone(cell_zone);
    }
  }

  world_state.zones = std::move(rebuilt_zones);
  rebuild_settlement_zone_ownership(world_state);
}

bool cell_state_changed(const CellZoneState& before, const CellZoneState& after) noexcept {
  return before.occupied != after.occupied || before.owner_settlement_id != after.owner_settlement_id ||
         before.zone_type != after.zone_type;
}

bool zone_owner_overlay_changed(const CellZoneState& before, const CellZoneState& after) noexcept {
  const uint32_t before_owner = before.occupied ? before.owner_settlement_id.value : 0U;
  const uint32_t after_owner = after.occupied ? after.owner_settlement_id.value : 0U;
  return before_owner != after_owner;
}

CommandOutcome make_rejected_outcome(const uint32_t command_index,
                                     const CommandRejectReason reject_reason,
                                     const std::string& reject_message) {
  return {
      .accepted = false,
      .command_index = command_index,
      .reject_reason = reject_reason,
      .reject_message = reject_message,
  };
}

CommandOutcome make_accepted_outcome(const uint32_t command_index) {
  return {
      .accepted = true,
      .command_index = command_index,
      .reject_reason = CommandRejectReason::Unknown,
      .reject_message = {},
  };
}

}  // namespace

void initialize_zone_state(world::WorldState& world_state) {
  const std::size_t cell_count = static_cast<std::size_t>(world_state.map_width) *
                                 static_cast<std::size_t>(world_state.map_height);
  world_state.zone_cells.assign(cell_count, {});
  world_state.zones.clear();
  world_state.next_zone_id = {1};
  rebuild_settlement_zone_ownership(world_state);
}

void rebuild_zone_state(world::WorldState& world_state) {
  assign_components_to_world(world_state, build_zone_components(world_state));
}

BatchResult apply_commands(world::WorldState& world_state, const CommandBatch& batch) {
  BatchResult result;
  result.outcomes.reserve(batch.commands.size());

  const std::vector<CellZoneState> before_zone_cells = world_state.zone_cells;
  const std::vector<SettlementZoneSnapshot> before_settlement_zones =
      snapshot_settlement_zones(world_state.settlements);

  for (uint32_t command_index = 0; command_index < batch.commands.size(); ++command_index) {
    const CommandVariant& command = batch.commands[command_index];
    std::visit(
        [&](const auto& typed_command) {
          using CommandType = std::decay_t<decltype(typed_command)>;

          if constexpr (std::is_same_v<CommandType, ZoneCellsCommand>) {
            ValidationResult validation = validate_zone_command_cells(
                world_state, typed_command.settlement_id, typed_command.cells,
                "ZoneCellsCommand contains out-of-bounds target cells.");
            if (!validation.ok) {
              result.outcomes.push_back(make_rejected_outcome(
                  command_index, validation.reject_reason, validation.reject_message));
              return;
            }

            const std::vector<int32_t> reach_costs =
                compute_reach_costs(world_state, *validation.settlement);
            for (const uint32_t cell_index : validation.cell_indices) {
              if (!is_zone_target_legal(world_state, reach_costs, typed_command.zone_type,
                                        cell_index)) {
                result.outcomes.push_back(make_rejected_outcome(
                    command_index, CommandRejectReason::IllegalZoneTarget,
                    "ZoneCellsCommand contains cells that are illegal for the requested zone type."));
                return;
              }
            }

            for (const uint32_t cell_index : validation.cell_indices) {
              CellZoneState& cell_zone = world_state.zone_cells[cell_index];
              cell_zone.occupied = true;
              cell_zone.owner_settlement_id = typed_command.settlement_id;
              cell_zone.zone_type = typed_command.zone_type;
            }

            rebuild_zone_state(world_state);
            result.outcomes.push_back(make_accepted_outcome(command_index));
          } else if constexpr (std::is_same_v<CommandType, RemoveZoneCellsCommand>) {
            ValidationResult validation = validate_zone_command_cells(
                world_state, typed_command.settlement_id, typed_command.cells,
                "RemoveZoneCellsCommand contains out-of-bounds target cells.");
            if (!validation.ok) {
              result.outcomes.push_back(make_rejected_outcome(
                  command_index, validation.reject_reason, validation.reject_message));
              return;
            }

            for (const uint32_t cell_index : validation.cell_indices) {
              CellZoneState& cell_zone = world_state.zone_cells[cell_index];
              if (cell_zone.occupied && cell_zone.owner_settlement_id == typed_command.settlement_id) {
                clear_cell_zone(cell_zone);
              }
            }

            rebuild_zone_state(world_state);
            result.outcomes.push_back(make_accepted_outcome(command_index));
          } else {
            result.outcomes.push_back(make_rejected_outcome(
                command_index, CommandRejectReason::Unknown,
                "This command type is not implemented in the current Milestone 1 slice."));
          }
        },
        command);
  }

  bool zone_overlay_dirty = false;
  for (std::size_t cell_index = 0; cell_index < before_zone_cells.size(); ++cell_index) {
    if (!cell_state_changed(before_zone_cells[cell_index], world_state.zone_cells[cell_index])) {
      continue;
    }

    result.dirty_chunks.push_back(to_chunk_coord(world_state, static_cast<uint32_t>(cell_index)));
    zone_overlay_dirty =
        zone_overlay_dirty || zone_owner_overlay_changed(before_zone_cells[cell_index],
                                                         world_state.zone_cells[cell_index]);
  }

  sort_and_deduplicate_chunks(result.dirty_chunks);
  if (zone_overlay_dirty) {
    result.dirty_overlays.push_back(OverlayType::ZoneOwner);
  }

  for (std::size_t settlement_index = 0;
       settlement_index < before_settlement_zones.size() &&
       settlement_index < world_state.settlements.size();
       ++settlement_index) {
    if (!same_zone_ownership(before_settlement_zones[settlement_index],
                             world_state.settlements[settlement_index])) {
      result.dirty_settlements.push_back(world_state.settlements[settlement_index].settlement_id);
    }
  }
  sort_and_deduplicate_settlements(result.dirty_settlements);

  append_world_dirty_chunks(world_state, result.dirty_chunks);
  return result;
}

OverlayChunkResult build_zone_owner_overlay_chunk_result(const world::WorldState& world_state,
                                                         const OverlayChunkQuery& query) {
  OverlayChunkResult result{
      .chunk = query.chunk,
      .overlay_type = query.overlay_type,
  };

  if (!world_state.map_grid.contains_chunk(query.chunk)) {
    return result;
  }

  result.width = kChunkSize;
  result.height = kChunkSize;
  result.legend = {
      OverlayLegendEntry{
          .value_index = kZoneOwnerOverlayUnzoned,
          .label = "Unzoned",
      },
  };

  for (const settlements::SettlementState& settlement : world_state.settlements) {
    result.legend.push_back({
        .value_index = static_cast<uint8_t>(std::min(settlement.settlement_id.value, 255U)),
        .label = "Settlement " + std::to_string(settlement.settlement_id.value),
    });
  }

  const int32_t origin_x = static_cast<int32_t>(query.chunk.x) * kChunkSize;
  const int32_t origin_y = static_cast<int32_t>(query.chunk.y) * kChunkSize;
  for (int32_t local_y = 0; local_y < kChunkSize; ++local_y) {
    for (int32_t local_x = 0; local_x < kChunkSize; ++local_x) {
      const int32_t world_x = origin_x + local_x;
      const int32_t world_y = origin_y + local_y;
      const uint32_t cell_index = to_cell_index(world_state, world_x, world_y);
      const CellZoneState& cell_zone = world_state.zone_cells[cell_index];
      const uint8_t owner_value =
          cell_zone.occupied ? static_cast<uint8_t>(std::min(cell_zone.owner_settlement_id.value, 255U))
                             : kZoneOwnerOverlayUnzoned;
      result.values[static_cast<std::size_t>(local_y) * kChunkSize +
                    static_cast<std::size_t>(local_x)] = owner_value;
    }
  }

  return result;
}

}  // namespace alpha::zones
