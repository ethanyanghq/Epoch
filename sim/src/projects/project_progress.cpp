#include "alpha/projects/project_progress.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "alpha/api/constants.hpp"
#include "alpha/projects/project_types.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::projects {
namespace {

constexpr WorkAmountTenths kWorkQuantumTenths = 10;

constexpr ResourceAmountTenths kRoadWoodPerCellTenths = 10;
constexpr ResourceAmountTenths kRoadStonePerCellTenths = 5;
constexpr ResourceAmountTenths kWarehouseWoodPerWorkQuantumTenths = 10;
constexpr ResourceAmountTenths kWarehouseStonePerWorkQuantumTenths = 5;

uint8_t priority_rank(const PriorityLabel priority) noexcept {
  switch (priority) {
    case PriorityLabel::Required:
      return 0;
    case PriorityLabel::High:
      return 1;
    case PriorityLabel::Normal:
      return 2;
    case PriorityLabel::Low:
      return 3;
    case PriorityLabel::Paused:
      return 4;
  }

  return 255;
}

bool is_blocked_now(const ProjectState& project) noexcept {
  return project.status == ProjectStatus::Blocked && !project.blocker_codes.empty();
}

CellCoord to_cell_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const int32_t width = static_cast<int32_t>(world_state.map_width);
  return {
      .x = static_cast<int32_t>(cell_index % static_cast<uint32_t>(width)),
      .y = static_cast<int32_t>(cell_index / static_cast<uint32_t>(width)),
  };
}

ChunkCoord to_chunk_coord(const world::WorldState& world_state, const uint32_t cell_index) {
  const CellCoord coord = to_cell_coord(world_state, cell_index);
  return {
      .x = static_cast<int16_t>(coord.x / kChunkSize),
      .y = static_cast<int16_t>(coord.y / kChunkSize),
  };
}

void sort_and_deduplicate_chunks(std::vector<ChunkCoord>& dirty_chunks) {
  std::sort(dirty_chunks.begin(), dirty_chunks.end(),
            [](const ChunkCoord& left, const ChunkCoord& right) {
              if (left.y != right.y) {
                return left.y < right.y;
              }
              return left.x < right.x;
            });
  dirty_chunks.erase(std::unique(dirty_chunks.begin(), dirty_chunks.end()), dirty_chunks.end());
}

void sort_and_deduplicate_projects(std::vector<ProjectId>& project_ids) {
  std::sort(project_ids.begin(), project_ids.end(),
            [](const ProjectId left, const ProjectId right) { return left < right; });
  project_ids.erase(std::unique(project_ids.begin(), project_ids.end()), project_ids.end());
}

void sort_and_deduplicate_overlays(std::vector<OverlayType>& dirty_overlays) {
  std::sort(dirty_overlays.begin(), dirty_overlays.end(),
            [](const OverlayType left, const OverlayType right) {
              return static_cast<uint8_t>(left) < static_cast<uint8_t>(right);
            });
  dirty_overlays.erase(std::unique(dirty_overlays.begin(), dirty_overlays.end()),
                       dirty_overlays.end());
}

settlements::BuildingState* find_building_slot(settlements::SettlementState& settlement,
                                               const BuildingType building_type) noexcept {
  for (settlements::BuildingState& building : settlement.buildings) {
    if (building.building_type == building_type) {
      return &building;
    }
  }

  return nullptr;
}

void mark_project_blocked(ProjectState& project, const ProjectBlockerCode blocker_code) {
  project.status = ProjectStatus::Blocked;
  project.blocker_codes.clear();
  project.blocker_codes.push_back(blocker_code);
}

void complete_project(ProjectState& project) {
  project.remaining_common_work = 0;
  project.remaining_skilled_work = 0;
  project.status = ProjectStatus::Completed;
  project.blocker_codes.clear();
}

void advance_road_project(world::WorldState& world_state, ProjectState& project,
                          const WorkAmountTenths progress, std::vector<ChunkCoord>& dirty_chunks) {
  const int32_t completed_cells_before =
      static_cast<int32_t>(project.consumed_materials.wood / kRoadWoodPerCellTenths);
  const int32_t cell_count = static_cast<int32_t>(progress / kWorkQuantumTenths);

  for (int32_t offset = 0; offset < cell_count; ++offset) {
    const uint32_t cell_index = project.route_cell_indices[static_cast<std::size_t>(
        completed_cells_before + offset)];
    mark_road_built(world_state, cell_index);
    dirty_chunks.push_back(to_chunk_coord(world_state, cell_index));
  }

  project.remaining_common_work -= progress;
  project.reserved_materials.wood -= cell_count * kRoadWoodPerCellTenths;
  project.reserved_materials.stone -= cell_count * kRoadStonePerCellTenths;
  project.consumed_materials.wood += cell_count * kRoadWoodPerCellTenths;
  project.consumed_materials.stone += cell_count * kRoadStonePerCellTenths;

  if (project.remaining_common_work <= 0) {
    complete_project(project);
  } else {
    project.status = ProjectStatus::InProgress;
    project.blocker_codes.clear();
  }
}

void advance_warehouse_project(world::WorldState& world_state, ProjectState& project,
                               const WorkAmountTenths progress) {
  const int32_t work_units = static_cast<int32_t>(progress / kWorkQuantumTenths);
  project.remaining_common_work -= progress;
  project.reserved_materials.wood -= work_units * kWarehouseWoodPerWorkQuantumTenths;
  project.reserved_materials.stone -= work_units * kWarehouseStonePerWorkQuantumTenths;
  project.consumed_materials.wood += work_units * kWarehouseWoodPerWorkQuantumTenths;
  project.consumed_materials.stone += work_units * kWarehouseStonePerWorkQuantumTenths;

  if (project.remaining_common_work <= 0) {
    settlements::SettlementState* settlement =
        settlements::find_settlement(world_state.settlements, project.owner_settlement_id);
    if (settlement != nullptr) {
      settlements::BuildingState* building_slot =
          find_building_slot(*settlement, BuildingType::WarehouseI);
      if (building_slot != nullptr) {
        building_slot->exists = true;
      }
    }
    complete_project(project);
    return;
  }

  project.status = ProjectStatus::InProgress;
  project.blocker_codes.clear();
}

WorkAmountTenths budget_for_settlement(const std::vector<ConstructionLaborBudget>& labor_budgets,
                                       const SettlementId settlement_id) {
  for (const ConstructionLaborBudget& budget : labor_budgets) {
    if (budget.settlement_id == settlement_id) {
      return budget.common_work_tenths;
    }
  }

  return 0;
}

}  // namespace

bool is_road_built(const world::WorldState& world_state, const uint32_t cell_index) noexcept {
  return cell_index < world_state.road_cells.size() && world_state.road_cells[cell_index] != 0U;
}

void mark_road_built(world::WorldState& world_state, const uint32_t cell_index) noexcept {
  if (cell_index >= world_state.road_cells.size() || world_state.road_cells[cell_index] != 0U) {
    return;
  }

  world_state.road_cells[cell_index] = 1U;
  ++world_state.road_cell_count;
}

ConstructionAdvanceResult advance_monthly_construction(
    world::WorldState& world_state, const std::vector<ConstructionLaborBudget>& labor_budgets) {
  ConstructionAdvanceResult result;
  std::vector<bool> was_blocked(world_state.projects.size(), false);
  std::vector<ProjectStatus> previous_status(world_state.projects.size(), ProjectStatus::Queued);

  for (std::size_t index = 0; index < world_state.projects.size(); ++index) {
    was_blocked[index] = is_blocked_now(world_state.projects[index]);
    previous_status[index] = world_state.projects[index].status;
  }

  std::vector<settlements::SettlementState*> settlements_in_order;
  settlements_in_order.reserve(world_state.settlements.size());
  for (settlements::SettlementState& settlement : world_state.settlements) {
    settlements_in_order.push_back(&settlement);
  }
  std::sort(settlements_in_order.begin(), settlements_in_order.end(),
            [](const settlements::SettlementState* left, const settlements::SettlementState* right) {
              return left->settlement_id < right->settlement_id;
            });

  for (settlements::SettlementState* settlement : settlements_in_order) {
    std::vector<ProjectState*> settlement_projects;
    for (ProjectState& project : world_state.projects) {
      if (project.owner_settlement_id == settlement->settlement_id &&
          project.status != ProjectStatus::Completed) {
        settlement_projects.push_back(&project);
      }
    }

    std::sort(settlement_projects.begin(), settlement_projects.end(),
              [](const ProjectState* left, const ProjectState* right) {
                if (priority_rank(left->priority) != priority_rank(right->priority)) {
                  return priority_rank(left->priority) < priority_rank(right->priority);
                }
                return left->project_id < right->project_id;
              });

    WorkAmountTenths remaining_capacity =
        budget_for_settlement(labor_budgets, settlement->settlement_id);
    for (ProjectState* project : settlement_projects) {
      if (project->priority == PriorityLabel::Paused) {
        mark_project_blocked(*project, ProjectBlockerCode::PausedByPriority);
        continue;
      }

      if (project->remaining_common_work <= 0 && project->remaining_skilled_work <= 0) {
        complete_project(*project);
        continue;
      }

      WorkAmountTenths progress = std::min(project->remaining_common_work, remaining_capacity);
      progress -= progress % kWorkQuantumTenths;
      if (progress <= 0) {
        mark_project_blocked(*project, ProjectBlockerCode::WaitingForConstructionCapacity);
        continue;
      }

      switch (project->family) {
        case ProjectFamily::Road:
          advance_road_project(world_state, *project, progress, result.dirty_chunks);
          break;
        case ProjectFamily::Building:
          if (project->type == static_cast<uint8_t>(ProjectTypeCode::WarehouseI)) {
            advance_warehouse_project(world_state, *project, progress);
          } else {
            mark_project_blocked(*project, ProjectBlockerCode::Unknown);
            continue;
          }
          break;
        case ProjectFamily::Founding:
        case ProjectFamily::Expansion:
          mark_project_blocked(*project, ProjectBlockerCode::Unknown);
          continue;
      }

      remaining_capacity -= progress;
    }
  }

  for (std::size_t index = 0; index < world_state.projects.size(); ++index) {
    const ProjectState& project = world_state.projects[index];
    const bool is_blocked = is_blocked_now(project);
    if (!was_blocked[index] && is_blocked) {
      result.newly_blocked_projects.push_back(project.project_id);
    }
    if (was_blocked[index] && !is_blocked) {
      result.newly_unblocked_projects.push_back(project.project_id);
    }
    if (previous_status[index] != ProjectStatus::Completed &&
        project.status == ProjectStatus::Completed) {
      result.completed_projects.push_back(project.project_id);
    }
  }

  if (!result.completed_projects.empty() || !result.newly_blocked_projects.empty() ||
      !result.newly_unblocked_projects.empty()) {
    result.dirty_overlays.push_back(OverlayType::ProjectBlockers);
  }

  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_projects(result.completed_projects);
  sort_and_deduplicate_projects(result.newly_blocked_projects);
  sort_and_deduplicate_projects(result.newly_unblocked_projects);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  return result;
}

}  // namespace alpha::projects
