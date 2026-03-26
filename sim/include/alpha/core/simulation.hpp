#pragma once

#include <optional>

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::core {

class Simulation final {
 public:
  CreateWorldResult create_world(const WorldCreateParams& params);
  TurnReport advance_month();
  ChunkVisualResult get_chunk_visual(const ChunkVisualQuery& query) const;
  OverlayChunkResult get_overlay_chunk(const OverlayChunkQuery& query) const;
  SettlementSummary get_settlement_summary(SettlementId settlement_id) const;
  WorldMetrics get_world_metrics() const;

  bool has_world() const noexcept;

 private:
  std::optional<world::WorldState> world_state_;
};

}  // namespace alpha::core
