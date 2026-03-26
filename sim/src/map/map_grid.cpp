#include "alpha/map/map_grid.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "alpha/api/constants.hpp"

namespace alpha::map {
namespace {

constexpr uint64_t kElevationMacroSalt = 0x9E3779B97F4A7C15ULL;
constexpr uint64_t kElevationMidSalt = 0xBF58476D1CE4E5B9ULL;
constexpr uint64_t kElevationDetailSalt = 0x94D049BB133111EBULL;
constexpr uint64_t kWaterSalt = 0xD6E8FEB86659FD93ULL;
constexpr uint64_t kFertilitySalt = 0xA5A3564E27F32B61ULL;
constexpr uint64_t kVegetationSalt = 0xC6BC279692B5CC83ULL;

uint64_t splitmix64(uint64_t value) noexcept {
  value += 0x9E3779B97F4A7C15ULL;
  value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31U);
}

uint8_t lattice_value(const uint64_t seed, const int32_t x, const int32_t y) noexcept {
  uint64_t mixed = seed;
  mixed ^= static_cast<uint64_t>(static_cast<uint32_t>(x)) * 0x9E3779B185EBCA87ULL;
  mixed ^= static_cast<uint64_t>(static_cast<uint32_t>(y)) * 0xC2B2AE3D27D4EB4FULL;
  return static_cast<uint8_t>(splitmix64(mixed) & 0xFFU);
}

uint8_t lerp_u8(const uint8_t a, const uint8_t b, const int32_t numerator,
                const int32_t denominator) noexcept {
  const int32_t weighted =
      static_cast<int32_t>(a) * (denominator - numerator) + static_cast<int32_t>(b) * numerator;
  return static_cast<uint8_t>(weighted / denominator);
}

uint8_t sample_value_noise(const uint64_t seed, const int32_t x, const int32_t y,
                           const int32_t cell_size) noexcept {
  const int32_t lattice_x = x / cell_size;
  const int32_t lattice_y = y / cell_size;
  const int32_t offset_x = x % cell_size;
  const int32_t offset_y = y % cell_size;

  const uint8_t top_left = lattice_value(seed, lattice_x, lattice_y);
  const uint8_t top_right = lattice_value(seed, lattice_x + 1, lattice_y);
  const uint8_t bottom_left = lattice_value(seed, lattice_x, lattice_y + 1);
  const uint8_t bottom_right = lattice_value(seed, lattice_x + 1, lattice_y + 1);

  const uint8_t top = lerp_u8(top_left, top_right, offset_x, cell_size);
  const uint8_t bottom = lerp_u8(bottom_left, bottom_right, offset_x, cell_size);
  return lerp_u8(top, bottom, offset_y, cell_size);
}

uint8_t generate_elevation(const uint64_t terrain_seed, const uint16_t width, const uint16_t height,
                           const int32_t x, const int32_t y) noexcept {
  const int32_t macro = sample_value_noise(terrain_seed ^ kElevationMacroSalt, x, y, 256);
  const int32_t mid = sample_value_noise(terrain_seed ^ kElevationMidSalt, x, y, 64);
  const int32_t detail = sample_value_noise(terrain_seed ^ kElevationDetailSalt, x, y, 16);

  const int32_t center_x = static_cast<int32_t>(width) / 2;
  const int32_t center_y = static_cast<int32_t>(height) / 2;
  const int32_t edge_distance =
      std::max(std::abs(x - center_x), std::abs(y - center_y));
  const int32_t continent_bias = std::clamp(196 - edge_distance / 2, 0, 196);

  const int32_t elevation = (macro * 5 + mid * 3 + detail + continent_bias * 4) / 13;
  return static_cast<uint8_t>(std::clamp(elevation, 0, 255));
}

uint8_t generate_water(const uint64_t terrain_seed, const uint8_t elevation, const int32_t x,
                       const int32_t y) noexcept {
  const int32_t basin_signal = sample_value_noise(terrain_seed ^ kWaterSalt, x, y, 96);

  if (elevation < 28 || (elevation < 36 && basin_signal > 150)) {
    return 3;
  }

  if (elevation < 42 || (elevation < 54 && basin_signal > 180)) {
    return 2;
  }

  if (elevation < 64 || basin_signal > 235) {
    return 1;
  }

  return 0;
}

uint8_t classify_slope(const int32_t max_delta) noexcept {
  if (max_delta <= 5) {
    return 0;
  }
  if (max_delta <= 12) {
    return 1;
  }
  if (max_delta <= 24) {
    return 2;
  }
  if (max_delta <= 40) {
    return 3;
  }
  return 4;
}

uint8_t generate_slope(const MapGrid& map_grid, const int32_t x, const int32_t y) noexcept {
  const int32_t elevation = map_grid.cell(x, y).elevation;

  int32_t max_delta = 0;
  constexpr std::array<std::pair<int32_t, int32_t>, 4> kNeighborOffsets = {
      std::pair{-1, 0},
      std::pair{1, 0},
      std::pair{0, -1},
      std::pair{0, 1},
  };

  for (const auto [offset_x, offset_y] : kNeighborOffsets) {
    const int32_t neighbor_x = x + offset_x;
    const int32_t neighbor_y = y + offset_y;
    if (!map_grid.contains_cell(neighbor_x, neighbor_y)) {
      continue;
    }

    const int32_t neighbor_elevation = map_grid.cell(neighbor_x, neighbor_y).elevation;
    max_delta = std::max(max_delta, std::abs(elevation - neighbor_elevation));
  }

  return classify_slope(max_delta);
}

uint8_t generate_fertility(const uint64_t terrain_seed, const MapCell& cell, const int32_t x,
                           const int32_t y) noexcept {
  if (cell.water >= 2) {
    return 0;
  }

  const int32_t moisture = sample_value_noise(terrain_seed ^ kFertilitySalt, x, y, 64);
  int32_t fertility = 20 + moisture / 3;
  fertility += std::max(0, 120 - static_cast<int32_t>(cell.elevation)) / 4;
  fertility -= static_cast<int32_t>(cell.slope) * 14;

  if (cell.water == 1) {
    fertility += 12;
  }

  return static_cast<uint8_t>(std::clamp(fertility, 0, 100));
}

uint8_t generate_vegetation(const uint64_t terrain_seed, const MapCell& cell, const int32_t x,
                            const int32_t y) noexcept {
  if (cell.water >= 2) {
    return 0;
  }

  const int32_t vegetation_noise = sample_value_noise(terrain_seed ^ kVegetationSalt, x, y, 32);
  int32_t vegetation = 15 + static_cast<int32_t>(cell.fertility) / 2 + vegetation_noise / 4;
  vegetation -= static_cast<int32_t>(cell.slope) * 8;

  if (cell.water == 1) {
    vegetation += 10;
  }

  return static_cast<uint8_t>(std::clamp(vegetation, 0, 100));
}

}  // namespace

bool MapGrid::initialize(const uint16_t width, const uint16_t height, const uint64_t terrain_seed) {
  if (width == 0 || height == 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  cells_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), {});

  for (int32_t y = 0; y < height_; ++y) {
    for (int32_t x = 0; x < width_; ++x) {
      cells_[index(x, y)].elevation = generate_elevation(terrain_seed, width_, height_, x, y);
    }
  }

  for (int32_t y = 0; y < height_; ++y) {
    for (int32_t x = 0; x < width_; ++x) {
      MapCell& cell = cells_[index(x, y)];
      cell.water = generate_water(terrain_seed, cell.elevation, x, y);
      cell.slope = generate_slope(*this, x, y);
    }
  }

  for (int32_t y = 0; y < height_; ++y) {
    for (int32_t x = 0; x < width_; ++x) {
      MapCell& cell = cells_[index(x, y)];
      cell.fertility = generate_fertility(terrain_seed, cell, x, y);
      cell.vegetation = generate_vegetation(terrain_seed, cell, x, y);
    }
  }

  return true;
}

bool MapGrid::initialize_from_cells(const uint16_t width, const uint16_t height,
                                    std::vector<MapCell> cells) {
  if (width == 0 || height == 0) {
    return false;
  }

  const std::size_t expected_cell_count =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (cells.size() != expected_cell_count) {
    return false;
  }

  width_ = width;
  height_ = height;
  cells_ = std::move(cells);
  return true;
}

bool MapGrid::empty() const noexcept {
  return cells_.empty();
}

uint16_t MapGrid::width() const noexcept {
  return width_;
}

uint16_t MapGrid::height() const noexcept {
  return height_;
}

bool MapGrid::contains_cell(const int32_t x, const int32_t y) const noexcept {
  return x >= 0 && y >= 0 && x < width_ && y < height_;
}

bool MapGrid::contains_chunk(const ChunkCoord chunk) const noexcept {
  if (chunk.x < 0 || chunk.y < 0 || empty()) {
    return false;
  }

  const int32_t chunk_count_x = (static_cast<int32_t>(width_) + kChunkSize - 1) / kChunkSize;
  const int32_t chunk_count_y = (static_cast<int32_t>(height_) + kChunkSize - 1) / kChunkSize;
  return chunk.x < chunk_count_x && chunk.y < chunk_count_y;
}

const MapCell& MapGrid::cell(const int32_t x, const int32_t y) const {
  return cells_[index(x, y)];
}

std::size_t MapGrid::index(const int32_t x, const int32_t y) const noexcept {
  return flatten_cell_index(width_, x, y);
}

}  // namespace alpha::map
