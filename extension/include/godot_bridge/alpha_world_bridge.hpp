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
  alpha::LoadWorldResult load_world(const alpha::LoadWorldParams& params);
  alpha::SaveWorldResult save_world(const alpha::SaveWorldParams& params) const;
  alpha::BatchResult apply_commands(const alpha::CommandBatch& batch);
  alpha::TurnReport advance_month();
  alpha::ChunkVisualResult get_chunk_visual(const alpha::ChunkVisualQuery& query) const;
  alpha::OverlayChunkResult get_overlay_chunk(const alpha::OverlayChunkQuery& query) const;
  alpha::SettlementSummary get_settlement_summary(alpha::SettlementId settlement_id) const;
  alpha::SettlementDetail get_settlement_detail(alpha::SettlementId settlement_id) const;
  alpha::ProjectListResult get_projects(const alpha::ProjectListQuery& query) const;
  alpha::WorldMetrics get_world_metrics() const;

 private:
  alpha::WorldApi world_api_;
};

BridgeRegistrationInfo get_bridge_registration_info() noexcept;
bool register_types() noexcept;

}  // namespace godot_bridge
