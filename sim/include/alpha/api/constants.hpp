#pragma once

#include <cstddef>
#include <cstdint>

namespace alpha {

inline constexpr uint16_t kChunkSize = 64;
inline constexpr std::size_t kChunkCellCount =
    static_cast<std::size_t>(kChunkSize) * static_cast<std::size_t>(kChunkSize);

}  // namespace alpha
