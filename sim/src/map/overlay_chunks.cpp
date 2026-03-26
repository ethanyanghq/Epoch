#include "alpha/map/overlay_chunks.hpp"

#include <array>
#include <string_view>

#include "alpha/api/constants.hpp"

namespace alpha::map {
namespace {

struct FertilityLegendBand {
  uint8_t upper_bound;
  std::string_view label;
};

constexpr std::array<FertilityLegendBand, 5> kFertilityLegendBands = {
    FertilityLegendBand{20, "Barren (0-20)"},
    FertilityLegendBand{40, "Poor (21-40)"},
    FertilityLegendBand{60, "Fair (41-60)"},
    FertilityLegendBand{80, "Good (61-80)"},
    FertilityLegendBand{100, "Rich (81-100)"},
};

uint8_t fertility_value_index(const uint8_t fertility) noexcept {
  for (uint8_t index = 0; index < kFertilityLegendBands.size(); ++index) {
    if (fertility <= kFertilityLegendBands[index].upper_bound) {
      return index;
    }
  }

  return static_cast<uint8_t>(kFertilityLegendBands.size() - 1);
}

std::vector<OverlayLegendEntry> build_fertility_legend() {
  std::vector<OverlayLegendEntry> legend;
  legend.reserve(kFertilityLegendBands.size());

  for (uint8_t index = 0; index < kFertilityLegendBands.size(); ++index) {
    legend.push_back({
        .value_index = index,
        .label = std::string(kFertilityLegendBands[index].label),
    });
  }

  return legend;
}

OverlayChunkResult build_fertility_overlay_chunk_result(const MapGrid& map_grid,
                                                        const OverlayChunkQuery& query) {
  OverlayChunkResult result{
      .chunk = query.chunk,
      .overlay_type = query.overlay_type,
  };

  if (!map_grid.contains_chunk(query.chunk)) {
    return result;
  }

  result.width = kChunkSize;
  result.height = kChunkSize;
  result.legend = build_fertility_legend();

  const int32_t origin_x = static_cast<int32_t>(query.chunk.x) * kChunkSize;
  const int32_t origin_y = static_cast<int32_t>(query.chunk.y) * kChunkSize;

  for (int32_t local_y = 0; local_y < kChunkSize; ++local_y) {
    for (int32_t local_x = 0; local_x < kChunkSize; ++local_x) {
      const int32_t world_x = origin_x + local_x;
      const int32_t world_y = origin_y + local_y;
      const MapCell& cell = map_grid.cell(world_x, world_y);

      result.values[static_cast<std::size_t>(local_y) * kChunkSize + static_cast<std::size_t>(local_x)] =
          fertility_value_index(cell.fertility);
    }
  }

  return result;
}

}  // namespace

OverlayChunkResult build_overlay_chunk_result(const MapGrid& map_grid, const OverlayChunkQuery& query) {
  switch (query.overlay_type) {
    case OverlayType::Fertility:
      return build_fertility_overlay_chunk_result(map_grid, query);
    default:
      return {
          .chunk = query.chunk,
          .overlay_type = query.overlay_type,
      };
  }
}

}  // namespace alpha::map
