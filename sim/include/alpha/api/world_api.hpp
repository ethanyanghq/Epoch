#pragma once

#include "alpha/core/simulation.hpp"

namespace alpha {

class WorldApi final {
 public:
  CreateWorldResult create_world(const WorldCreateParams& params);
  TurnReport advance_month();
  ChunkVisualResult get_chunk_visual(const ChunkVisualQuery& query) const;
  OverlayChunkResult get_overlay_chunk(const OverlayChunkQuery& query) const;
  WorldMetrics get_world_metrics() const;

 private:
  core::Simulation simulation_;
};

}  // namespace alpha
