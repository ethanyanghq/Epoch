#pragma once

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"

namespace alpha::world {
struct WorldState;
}

namespace alpha::map {

ChunkVisualResult build_chunk_visual_result(const world::WorldState& world_state,
                                            const ChunkVisualQuery& query);

}  // namespace alpha::map
