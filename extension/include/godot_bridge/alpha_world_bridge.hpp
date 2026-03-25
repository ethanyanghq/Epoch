#pragma once

#include <string_view>

#include "alpha/api/world_api.hpp"

namespace godot_bridge {

struct BridgeRegistrationInfo {
  std::string_view module_name;
  std::string_view bridge_class_name;
};

class AlphaWorldBridge final {
 public:
  alpha::CreateWorldResult create_world(const alpha::WorldCreateParams& params);
  alpha::TurnReport advance_month();
  alpha::ChunkVisualResult get_chunk_visual(const alpha::ChunkVisualQuery& query) const;
  alpha::WorldMetrics get_world_metrics() const;

 private:
  alpha::WorldApi world_api_;
};

BridgeRegistrationInfo get_bridge_registration_info() noexcept;
bool register_types() noexcept;

}  // namespace godot_bridge
