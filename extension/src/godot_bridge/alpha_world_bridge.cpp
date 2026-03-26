#include "godot_bridge/alpha_world_bridge.hpp"

namespace godot_bridge {

alpha::CreateWorldResult AlphaWorldBridge::create_world(const alpha::WorldCreateParams& params) {
  return world_api_.create_world(params);
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

alpha::WorldMetrics AlphaWorldBridge::get_world_metrics() const {
  return world_api_.get_world_metrics();
}

}  // namespace godot_bridge
