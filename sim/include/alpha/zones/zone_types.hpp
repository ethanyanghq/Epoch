#pragma once

#include <cstdint>
#include <vector>

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"

namespace alpha::world {
struct WorldState;
}

namespace alpha::zones {

struct CellZoneState {
  ZoneId zone_id;
  SettlementId owner_settlement_id;
  ZoneType zone_type = ZoneType::Farmland;
  bool occupied = false;
};

struct ZoneState {
  ZoneId zone_id;
  SettlementId owner_settlement_id;
  ZoneType zone_type = ZoneType::Farmland;
  std::vector<uint32_t> member_cell_indices;
};

void initialize_zone_state(world::WorldState& world_state);
void rebuild_zone_state(world::WorldState& world_state);
BatchResult apply_commands(world::WorldState& world_state, const CommandBatch& batch);
OverlayChunkResult build_zone_owner_overlay_chunk_result(const world::WorldState& world_state,
                                                         const OverlayChunkQuery& query);

}  // namespace alpha::zones
