#include "alpha/api/world_api.hpp"

namespace alpha {

CreateWorldResult WorldApi::create_world(const WorldCreateParams& params) {
  return simulation_.create_world(params);
}

LoadWorldResult WorldApi::load_world(const LoadWorldParams& params) {
  return simulation_.load_world(params);
}

SaveWorldResult WorldApi::save_world(const SaveWorldParams& params) const {
  return simulation_.save_world(params);
}

TurnReport WorldApi::advance_month() {
  return simulation_.advance_month();
}

ChunkVisualResult WorldApi::get_chunk_visual(const ChunkVisualQuery& query) const {
  return simulation_.get_chunk_visual(query);
}

OverlayChunkResult WorldApi::get_overlay_chunk(const OverlayChunkQuery& query) const {
  return simulation_.get_overlay_chunk(query);
}

SettlementSummary WorldApi::get_settlement_summary(const SettlementId settlement_id) const {
  return simulation_.get_settlement_summary(settlement_id);
}

WorldMetrics WorldApi::get_world_metrics() const {
  return simulation_.get_world_metrics();
}

}  // namespace alpha
