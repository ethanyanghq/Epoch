#include "godot_bridge/alpha_world_bridge.hpp"

namespace godot_bridge {

BridgeRegistrationInfo get_bridge_registration_info() noexcept {
  return {
      .module_name = "alpha",
      .bridge_class_name = "AlphaWorldBridge",
  };
}

bool register_types() noexcept {
  return true;
}

}  // namespace godot_bridge
