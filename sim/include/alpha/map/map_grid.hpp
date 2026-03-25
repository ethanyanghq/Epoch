#pragma once

#include <cstdint>
#include <vector>

#include "alpha/api/result_types.hpp"
#include "alpha/map/map_types.hpp"

namespace alpha::map {

class MapGrid final {
 public:
  bool initialize(uint16_t width, uint16_t height, uint64_t terrain_seed);

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] uint16_t width() const noexcept;
  [[nodiscard]] uint16_t height() const noexcept;

  [[nodiscard]] bool contains_cell(int32_t x, int32_t y) const noexcept;
  [[nodiscard]] bool contains_chunk(ChunkCoord chunk) const noexcept;

  [[nodiscard]] const MapCell& cell(int32_t x, int32_t y) const;

 private:
  [[nodiscard]] std::size_t index(int32_t x, int32_t y) const noexcept;

  uint16_t width_ = 0;
  uint16_t height_ = 0;
  std::vector<MapCell> cells_;
};

}  // namespace alpha::map
