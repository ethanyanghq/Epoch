#include "godot_bridge/alpha_world_bridge.hpp"

namespace godot_bridge {

alpha::CreateWorldResult AlphaWorldBridge::create_world(const alpha::WorldCreateParams& params) {
  return world_api_.create_world(params);
}

alpha::TurnReport AlphaWorldBridge::advance_month() {
  return world_api_.advance_month();
}

alpha::WorldMetrics AlphaWorldBridge::get_world_metrics() const {
  return world_api_.get_world_metrics();
}

}  // namespace godot_bridge
