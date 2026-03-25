#pragma once

#include <cstdint>

#include "alpha/api/result_types.hpp"

namespace alpha {

struct ChunkVisualQuery {
  ChunkCoord chunk;
  uint8_t layer_index = 0;
};

}  // namespace alpha
