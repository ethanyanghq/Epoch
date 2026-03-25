#pragma once

#include <cstddef>
#include <cstdint>

namespace alpha::map {

struct MapCell {
  uint8_t elevation = 0;
  uint8_t slope = 0;
  uint8_t water = 0;
  uint8_t fertility = 0;
  uint8_t vegetation = 0;
};

constexpr std::size_t flatten_cell_index(const uint16_t width, const int32_t x, const int32_t y) noexcept {
  return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

}  // namespace alpha::map
