#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "alpha/api/result_types.hpp"

namespace alpha {

struct ZoneCellsCommand {
  SettlementId settlement_id;
  ZoneType zone_type = ZoneType::Farmland;
  std::vector<CellCoord> cells;
};

struct RemoveZoneCellsCommand {
  SettlementId settlement_id;
  std::vector<CellCoord> cells;
};

struct QueueRoadCommand {
  SettlementId settlement_id;
  std::vector<CellCoord> route_cells;
  PriorityLabel priority = PriorityLabel::Normal;
};

struct QueueBuildingCommand {
  SettlementId settlement_id;
  BuildingType building_type = BuildingType::EstateI;
  PriorityLabel priority = PriorityLabel::Normal;
};

struct QueueFoundingCommand {
  SettlementId source_settlement_id;
  CellCoord target_center;
  PriorityLabel priority = PriorityLabel::Normal;
};

struct SetProjectPriorityCommand {
  ProjectId project_id;
  PriorityLabel priority = PriorityLabel::Normal;
};

using CommandVariant =
    std::variant<ZoneCellsCommand, RemoveZoneCellsCommand, QueueRoadCommand,
                 QueueBuildingCommand, QueueFoundingCommand, SetProjectPriorityCommand>;

struct CommandBatch {
  std::vector<CommandVariant> commands;
};

struct ChunkVisualQuery {
  ChunkCoord chunk;
  uint8_t layer_index = 0;
};

struct OverlayChunkQuery {
  ChunkCoord chunk;
  OverlayType overlay_type = OverlayType::Fertility;
};

}  // namespace alpha
