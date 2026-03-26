#pragma once

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"
#include "alpha/map/map_grid.hpp"

namespace alpha::map {

OverlayChunkResult build_overlay_chunk_result(const MapGrid& map_grid, const OverlayChunkQuery& query);

}  // namespace alpha::map
