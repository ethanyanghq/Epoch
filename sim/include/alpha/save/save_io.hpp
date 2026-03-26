#pragma once

#include <string>

#include "alpha/api/result_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::save {

inline constexpr uint32_t kSaveFormatVersion = 4;
inline constexpr uint32_t kRulesetVersion = 1;
inline constexpr uint32_t kGeneratorVersion = 1;

struct LoadWorldStateResult {
  bool ok = false;
  std::string error_message;
  uint32_t format_version = 0;
  world::WorldState world_state;
};

SaveWorldResult save_world(const world::WorldState& world_state, const SaveWorldParams& params);
LoadWorldStateResult load_world(const LoadWorldParams& params);

}  // namespace alpha::save
