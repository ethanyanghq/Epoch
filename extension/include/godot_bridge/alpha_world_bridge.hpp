#pragma once

#include <cstdint>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "alpha/api/world_api.hpp"

namespace godot_bridge {

class AlphaWorldBridge final : public godot::RefCounted {
  GDCLASS(AlphaWorldBridge, godot::RefCounted)

 public:
  godot::Dictionary create_world(const godot::Dictionary& params);
  godot::Dictionary load_world(const godot::Dictionary& params);
  godot::Dictionary save_world(const godot::Dictionary& params) const;
  godot::Dictionary apply_commands(const godot::Dictionary& batch);
  godot::Dictionary advance_month();
  godot::Dictionary get_chunk_visual(const godot::Dictionary& query) const;
  godot::Dictionary get_overlay_chunk(const godot::Dictionary& query) const;
  godot::Dictionary get_settlement_summary(int64_t settlement_id) const;
  godot::Dictionary get_settlement_detail(int64_t settlement_id) const;
  godot::Dictionary get_projects(const godot::Dictionary& query) const;
  godot::Dictionary get_world_metrics() const;

  static void _bind_methods();

 private:
  alpha::WorldApi world_api_;
};

void initialize_alpha_module(godot::ModuleInitializationLevel level);
void uninitialize_alpha_module(godot::ModuleInitializationLevel level);

}  // namespace godot_bridge
