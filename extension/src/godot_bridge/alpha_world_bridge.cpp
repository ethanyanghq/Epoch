#include "godot_bridge/alpha_world_bridge.hpp"

namespace godot_bridge {

alpha::CreateWorldResult AlphaWorldBridge::create_world(const alpha::WorldCreateParams& params) {
  return world_api_.create_world(params);
}

alpha::LoadWorldResult AlphaWorldBridge::load_world(const alpha::LoadWorldParams& params) {
  return world_api_.load_world(params);
}

alpha::SaveWorldResult AlphaWorldBridge::save_world(const alpha::SaveWorldParams& params) const {
  return world_api_.save_world(params);
}

alpha::BatchResult AlphaWorldBridge::apply_commands(const alpha::CommandBatch& batch) {
  return world_api_.apply_commands(batch);
}

alpha::TurnReport AlphaWorldBridge::advance_month() {
  return world_api_.advance_month();
}

alpha::ChunkVisualResult AlphaWorldBridge::get_chunk_visual(const alpha::ChunkVisualQuery& query) const {
  return world_api_.get_chunk_visual(query);
}

alpha::OverlayChunkResult AlphaWorldBridge::get_overlay_chunk(
    const alpha::OverlayChunkQuery& query) const {
  return world_api_.get_overlay_chunk(query);
}

alpha::SettlementSummary AlphaWorldBridge::get_settlement_summary(
    const alpha::SettlementId settlement_id) const {
  return world_api_.get_settlement_summary(settlement_id);
}

alpha::ProjectListResult AlphaWorldBridge::get_projects(const alpha::ProjectListQuery& query) const {
  return world_api_.get_projects(query);
}

alpha::WorldMetrics AlphaWorldBridge::get_world_metrics() const {
  return world_api_.get_world_metrics();
}

}  // namespace godot_bridge
