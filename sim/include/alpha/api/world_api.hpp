#pragma once

#include "alpha/core/simulation.hpp"

namespace alpha {

class WorldApi final {
 public:
  CreateWorldResult create_world(const WorldCreateParams& params);
  LoadWorldResult load_world(const LoadWorldParams& params);
  SaveWorldResult save_world(const SaveWorldParams& params) const;
  BatchResult apply_commands(const CommandBatch& batch);
  TurnReport advance_month();
  ChunkVisualResult get_chunk_visual(const ChunkVisualQuery& query) const;
  OverlayChunkResult get_overlay_chunk(const OverlayChunkQuery& query) const;
  SettlementSummary get_settlement_summary(SettlementId settlement_id) const;
  ProjectListResult get_projects(const ProjectListQuery& query) const;
  WorldMetrics get_world_metrics() const;

 private:
  core::Simulation simulation_;
};

}  // namespace alpha
