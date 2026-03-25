#pragma once

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"
#include "alpha/map/map_grid.hpp"

namespace alpha::map {

ChunkVisualResult build_chunk_visual_result(const MapGrid& map_grid, const ChunkVisualQuery& query);

}  // namespace alpha::map
