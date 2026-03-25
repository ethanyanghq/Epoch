#pragma once

#include <compare>
#include <cstdint>

namespace alpha {

struct SettlementId {
  uint32_t value = 0;

  auto operator<=>(const SettlementId&) const = default;
};

struct ZoneId {
  uint32_t value = 0;

  auto operator<=>(const ZoneId&) const = default;
};

struct FarmPlotId {
  uint32_t value = 0;

  auto operator<=>(const FarmPlotId&) const = default;
};

struct ProjectId {
  uint32_t value = 0;

  auto operator<=>(const ProjectId&) const = default;
};

}  // namespace alpha
