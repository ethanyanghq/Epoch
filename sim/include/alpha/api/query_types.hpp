#pragma once

#include <cstdint>

#include "alpha/api/result_types.hpp"

namespace alpha {

struct ChunkVisualQuery {
  ChunkCoord chunk;
  uint8_t layer_index = 0;
};

struct OverlayChunkQuery {
  ChunkCoord chunk;
  OverlayType overlay_type = OverlayType::Fertility;
};

}  // namespace alpha
