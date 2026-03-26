#include "alpha/projects/project_types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "alpha/api/constants.hpp"
#include "alpha/map/map_grid.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/projects/project_progress.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::projects {
namespace {

constexpr WorkAmountTenths kRoadWorkPerCellTenths = 10;
constexpr ResourceAmountTenths kRoadWoodPerCellTenths = 10;
constexpr ResourceAmountTenths kRoadStonePerCellTenths = 5;

constexpr WorkAmountTenths kWarehouseCommonWorkTenths = 40;
constexpr ResourceAmountTenths kWarehouseWoodTenths = 40;
constexpr ResourceAmountTenths kWarehouseStoneTenths = 20;

struct ValidationResult {
  bool ok = false;
  CommandRejectReason reject_reason = CommandRejectReason::Unknown;
  std::string reject_message;
};

struct RoadValidationResult {
  ValidationResult validation;
  std::vector<uint32_t> route_cell_indices;
};

uint32_t to_cell_index(const world::WorldState& world_state, const int32_t x, const int32_t y) {
  return static_cast<uint32_t>(map::flatten_cell_index(world_state.map_width, x, y));
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

void sort_and_deduplicate_settlements(std::vector<SettlementId>& dirty_settlements) {
  std::sort(dirty_settlements.begin(), dirty_settlements.end(),
            [](const SettlementId left, const SettlementId right) { return left < right; });
  dirty_settlements.erase(std::unique(dirty_settlements.begin(), dirty_settlements.end()),
                          dirty_settlements.end());
}

void sort_and_deduplicate_overlays(std::vector<OverlayType>& dirty_overlays) {
  std::sort(dirty_overlays.begin(), dirty_overlays.end(),
            [](const OverlayType left, const OverlayType right) {
              return static_cast<uint8_t>(left) < static_cast<uint8_t>(right);
            });
  dirty_overlays.erase(std::unique(dirty_overlays.begin(), dirty_overlays.end()),
                       dirty_overlays.end());
}

void append_world_dirty_chunks(world::WorldState& world_state,
                               const std::vector<ChunkCoord>& dirty_chunks) {
  world_state.dirty_chunks.insert(world_state.dirty_chunks.end(), dirty_chunks.begin(),
                                  dirty_chunks.end());
  sort_and_deduplicate_chunks(world_state.dirty_chunks);
}

CommandOutcome make_rejected_outcome(const uint32_t command_index,
                                     const CommandRejectReason reject_reason,
                                     const std::string& reject_message) {
  return {
      .accepted = false,
      .command_index = command_index,
      .reject_reason = reject_reason,
      .reject_message = reject_message,
  };
}

CommandOutcome make_accepted_outcome(const uint32_t command_index) {
  return {
      .accepted = true,
      .command_index = command_index,
      .reject_reason = CommandRejectReason::Unknown,
      .reject_message = {},
  };
}

bool is_prototype_road_cell_legal(const map::MapCell& cell) noexcept {
  if (cell.slope >= 4U || cell.water >= 2U) {
    return false;
  }
  if (cell.water == 1U) {
    return cell.slope <= 2U;
  }
  return cell.slope <= 3U;
}

bool is_four_neighbor_step(const CellCoord& from, const CellCoord& to) noexcept {
  return std::abs(from.x - to.x) + std::abs(from.y - to.y) == 1;
}

bool is_project_active(const ProjectState& project) noexcept {
  return project.status != ProjectStatus::Completed;
}

bool route_conflicts_with_existing_project(const world::WorldState& world_state,
                                           const std::vector<uint32_t>& route_cell_indices) {
  for (const ProjectState& project : world_state.projects) {
    if (project.family != ProjectFamily::Road || !is_project_active(project)) {
      continue;
    }

    for (const uint32_t cell_index : route_cell_indices) {
      if (std::find(project.route_cell_indices.begin(), project.route_cell_indices.end(),
                    cell_index) != project.route_cell_indices.end()) {
        return true;
      }
    }
  }

  return false;
}

ProjectResourceState compute_road_required_materials(const std::size_t route_length) noexcept {
  return {
      .food = 0,
      .wood = static_cast<ResourceAmountTenths>(route_length * kRoadWoodPerCellTenths),
      .stone = static_cast<ResourceAmountTenths>(route_length * kRoadStonePerCellTenths),
  };
}

ProjectResourceState warehouse_required_materials() noexcept {
  return {
      .food = 0,
      .wood = kWarehouseWoodTenths,
      .stone = kWarehouseStoneTenths,
  };
}

bool stockpile_can_cover(const settlements::StockpileState& stockpile,
                         const ProjectResourceState& materials) noexcept {
  return stockpile.food >= materials.food && stockpile.wood >= materials.wood &&
         stockpile.stone >= materials.stone;
}

void reserve_materials(settlements::SettlementState& settlement,
                       const ProjectResourceState& materials) noexcept {
  settlement.stockpile.food -= materials.food;
  settlement.stockpile.wood -= materials.wood;
  settlement.stockpile.stone -= materials.stone;
}

std::vector<ChunkCoord> route_dirty_chunks(const world::WorldState& world_state,
                                           const std::vector<uint32_t>& route_cell_indices) {
  std::vector<ChunkCoord> dirty_chunks;
  dirty_chunks.reserve(route_cell_indices.size());

  for (const uint32_t cell_index : route_cell_indices) {
    dirty_chunks.push_back(to_chunk_coord(world_state, cell_index));
  }

  sort_and_deduplicate_chunks(dirty_chunks);
  return dirty_chunks;
}

uint8_t project_type_for_building(const BuildingType building_type) noexcept {
  switch (building_type) {
    case BuildingType::WarehouseI:
      return static_cast<uint8_t>(ProjectTypeCode::WarehouseI);
    case BuildingType::EstateI:
      return 0;
  }

  return 0;
}

const settlements::BuildingState* find_building_slot(const settlements::SettlementState& settlement,
                                                     const BuildingType building_type) noexcept {
  for (const settlements::BuildingState& building : settlement.buildings) {
    if (building.building_type == building_type) {
      return &building;
    }
  }

  return nullptr;
}

bool has_active_building_project(const world::WorldState& world_state,
                                 const SettlementId settlement_id) noexcept {
  for (const ProjectState& project : world_state.projects) {
    if (project.owner_settlement_id == settlement_id && project.family == ProjectFamily::Building &&
        is_project_active(project)) {
      return true;
    }
  }

  return false;
}

bool has_active_unique_building_project(const world::WorldState& world_state,
                                        const SettlementId settlement_id,
                                        const BuildingType building_type) noexcept {
  const uint8_t type_code = project_type_for_building(building_type);
  for (const ProjectState& project : world_state.projects) {
    if (project.owner_settlement_id == settlement_id && project.family == ProjectFamily::Building &&
        project.type == type_code && is_project_active(project)) {
      return true;
    }
  }

  return false;
}

RoadValidationResult validate_queue_road_command(const world::WorldState& world_state,
                                                 const QueueRoadCommand& command) {
  RoadValidationResult validation;

  if (!is_valid_priority_label(command.priority)) {
    validation.validation.reject_reason = CommandRejectReason::InvalidPriority;
    validation.validation.reject_message = "The requested priority label is not supported.";
    return validation;
  }

  const settlements::SettlementState* settlement =
      settlements::find_settlement(world_state.settlements, command.settlement_id);
  if (settlement == nullptr) {
    validation.validation.reject_reason = CommandRejectReason::InvalidSettlement;
    validation.validation.reject_message = "The settlement id does not exist.";
    return validation;
  }

  if (command.route_cells.empty()) {
    validation.validation.reject_reason = CommandRejectReason::InvalidCells;
    validation.validation.reject_message = "QueueRoadCommand requires at least one route cell.";
    return validation;
  }

  validation.route_cell_indices.reserve(command.route_cells.size());
  for (std::size_t index = 0; index < command.route_cells.size(); ++index) {
    const CellCoord cell = command.route_cells[index];
    if (!world_state.map_grid.contains_cell(cell.x, cell.y)) {
      validation.validation.reject_reason = CommandRejectReason::InvalidCells;
      validation.validation.reject_message = "QueueRoadCommand contains out-of-bounds route cells.";
      validation.route_cell_indices.clear();
      return validation;
    }

    if (index > 0U && !is_four_neighbor_step(command.route_cells[index - 1U], cell)) {
      validation.validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.validation.reject_message =
          "QueueRoadCommand route cells must form a contiguous 4-neighbor path.";
      validation.route_cell_indices.clear();
      return validation;
    }

    const uint32_t cell_index = to_cell_index(world_state, cell.x, cell.y);
    if (std::find(validation.route_cell_indices.begin(), validation.route_cell_indices.end(),
                  cell_index) != validation.route_cell_indices.end()) {
      validation.validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.validation.reject_message =
          "QueueRoadCommand route cells must form a contiguous 4-neighbor path.";
      validation.route_cell_indices.clear();
      return validation;
    }

    const map::MapCell& map_cell = world_state.map_grid.cell(cell.x, cell.y);
    if (!is_prototype_road_cell_legal(map_cell)) {
      validation.validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.validation.reject_message =
          "QueueRoadCommand route cells contain terrain that is illegal for prototype road placement.";
      validation.route_cell_indices.clear();
      return validation;
    }

    if (is_road_built(world_state, cell_index)) {
      validation.validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.validation.reject_message =
          "QueueRoadCommand route cells already contain completed road segments.";
      validation.route_cell_indices.clear();
      return validation;
    }

    validation.route_cell_indices.push_back(cell_index);
  }

  if (route_conflicts_with_existing_project(world_state, validation.route_cell_indices)) {
    validation.validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
    validation.validation.reject_message =
        "QueueRoadCommand route cells overlap an active road project.";
    validation.route_cell_indices.clear();
    return validation;
  }

  const ProjectResourceState required_materials =
      compute_road_required_materials(validation.route_cell_indices.size());
  if (!stockpile_can_cover(settlement->stockpile, required_materials)) {
    validation.validation.reject_reason = CommandRejectReason::MissingResources;
    validation.validation.reject_message =
        "The settlement does not have enough reserved materials capacity to queue this road.";
    validation.route_cell_indices.clear();
    return validation;
  }

  validation.validation.ok = true;
  return validation;
}

ValidationResult validate_queue_building_command(const world::WorldState& world_state,
                                                 const QueueBuildingCommand& command) {
  ValidationResult validation;

  if (!is_valid_priority_label(command.priority)) {
    validation.reject_reason = CommandRejectReason::InvalidPriority;
    validation.reject_message = "The requested priority label is not supported.";
    return validation;
  }

  const settlements::SettlementState* settlement =
      settlements::find_settlement(world_state.settlements, command.settlement_id);
  if (settlement == nullptr) {
    validation.reject_reason = CommandRejectReason::InvalidSettlement;
    validation.reject_message = "The settlement id does not exist.";
    return validation;
  }

  if (command.building_type != BuildingType::WarehouseI) {
    validation.reject_reason = CommandRejectReason::IllegalBuildingRequest;
    validation.reject_message =
        "Only Warehouse I is implemented as a real Milestone 1 building project path.";
    return validation;
  }

  const settlements::BuildingState* building_slot =
      find_building_slot(*settlement, command.building_type);
  if (building_slot == nullptr) {
    validation.reject_reason = CommandRejectReason::IllegalBuildingRequest;
    validation.reject_message = "The settlement does not expose the requested building slot.";
    return validation;
  }

  if (building_slot->exists ||
      has_active_unique_building_project(world_state, command.settlement_id, command.building_type)) {
    validation.reject_reason = CommandRejectReason::DuplicateUniqueBuilding;
    validation.reject_message = "The requested unique building already exists or is already queued.";
    return validation;
  }

  if (has_active_building_project(world_state, command.settlement_id)) {
    validation.reject_reason = CommandRejectReason::IllegalBuildingRequest;
    validation.reject_message =
        "Only one active internal building project is allowed per settlement in Milestone 1.";
    return validation;
  }

  if (!stockpile_can_cover(settlement->stockpile, warehouse_required_materials())) {
    validation.reject_reason = CommandRejectReason::MissingResources;
    validation.reject_message =
        "The settlement does not have enough wood and stone to queue this building.";
    return validation;
  }

  validation.ok = true;
  return validation;
}

ProjectView build_project_view(const ProjectState& project) {
  ProjectView view{
      .project_id = project.project_id,
      .owner_settlement_id = project.owner_settlement_id,
      .family = project.family,
      .type_name = project_type_name(project),
      .priority = project.priority,
      .status = project.status,
      .status_name = project_status_name(project.status),
      .required_food = project.required_materials.food,
      .required_wood = project.required_materials.wood,
      .required_stone = project.required_materials.stone,
      .reserved_food = project.reserved_materials.food,
      .reserved_wood = project.reserved_materials.wood,
      .reserved_stone = project.reserved_materials.stone,
      .remaining_common_work = project.remaining_common_work,
      .remaining_skilled_work = project.remaining_skilled_work,
      .access_modifier_tenths = project.access_modifier_tenths,
      .blocker_codes = project.blocker_codes,
  };

  view.blockers.reserve(project.blocker_codes.size());
  for (const ProjectBlockerCode blocker_code : project.blocker_codes) {
    view.blockers.push_back(project_blocker_message(blocker_code));
  }
  return view;
}

}  // namespace

void apply_queue_road_command(world::WorldState& world_state, const uint32_t command_index,
                              const QueueRoadCommand& command, BatchResult& result) {
  const RoadValidationResult validation = validate_queue_road_command(world_state, command);
  if (!validation.validation.ok) {
    result.outcomes.push_back(make_rejected_outcome(command_index, validation.validation.reject_reason,
                                                    validation.validation.reject_message));
    return;
  }

  settlements::SettlementState* settlement =
      settlements::find_settlement(world_state.settlements, command.settlement_id);
  if (settlement == nullptr) {
    result.outcomes.push_back(make_rejected_outcome(
        command_index, CommandRejectReason::InvalidSettlement, "The settlement id does not exist."));
    return;
  }

  const ProjectResourceState required_materials =
      compute_road_required_materials(validation.route_cell_indices.size());
  reserve_materials(*settlement, required_materials);

  ProjectState project{
      .project_id = world_state.next_project_id,
      .owner_settlement_id = command.settlement_id,
      .family = ProjectFamily::Road,
      .type = static_cast<uint8_t>(ProjectTypeCode::Road),
      .target = to_cell_coord(world_state, validation.route_cell_indices.back()),
      .route_cell_indices = validation.route_cell_indices,
      .priority = command.priority,
      .status = ProjectStatus::Queued,
      .required_materials = required_materials,
      .reserved_materials = required_materials,
      .consumed_materials = {},
      .remaining_common_work = static_cast<WorkAmountTenths>(
          validation.route_cell_indices.size() * kRoadWorkPerCellTenths),
      .remaining_skilled_work = 0,
      .access_modifier_tenths = 0,
  };
  refresh_project_derived_state(project);

  ++world_state.next_project_id.value;
  result.outcomes.push_back(make_accepted_outcome(command_index));
  result.new_projects.push_back(project.project_id);

  const std::vector<ChunkCoord> dirty_chunks =
      route_dirty_chunks(world_state, project.route_cell_indices);
  result.dirty_chunks.insert(result.dirty_chunks.end(), dirty_chunks.begin(), dirty_chunks.end());
  result.dirty_overlays.push_back(OverlayType::ProjectBlockers);
  result.dirty_settlements.push_back(project.owner_settlement_id);
  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  sort_and_deduplicate_settlements(result.dirty_settlements);
  append_world_dirty_chunks(world_state, dirty_chunks);

  world_state.projects.push_back(std::move(project));
  world_state.project_count = static_cast<uint32_t>(world_state.projects.size());
}

void apply_queue_building_command(world::WorldState& world_state, const uint32_t command_index,
                                  const QueueBuildingCommand& command, BatchResult& result) {
  const ValidationResult validation = validate_queue_building_command(world_state, command);
  if (!validation.ok) {
    result.outcomes.push_back(
        make_rejected_outcome(command_index, validation.reject_reason, validation.reject_message));
    return;
  }

  settlements::SettlementState* settlement =
      settlements::find_settlement(world_state.settlements, command.settlement_id);
  if (settlement == nullptr) {
    result.outcomes.push_back(make_rejected_outcome(
        command_index, CommandRejectReason::InvalidSettlement, "The settlement id does not exist."));
    return;
  }

  const ProjectResourceState required_materials = warehouse_required_materials();
  reserve_materials(*settlement, required_materials);

  ProjectState project{
      .project_id = world_state.next_project_id,
      .owner_settlement_id = command.settlement_id,
      .family = ProjectFamily::Building,
      .type = static_cast<uint8_t>(ProjectTypeCode::WarehouseI),
      .target = settlement->center,
      .route_cell_indices = {},
      .priority = command.priority,
      .status = ProjectStatus::Queued,
      .required_materials = required_materials,
      .reserved_materials = required_materials,
      .consumed_materials = {},
      .remaining_common_work = kWarehouseCommonWorkTenths,
      .remaining_skilled_work = 0,
      .access_modifier_tenths = 0,
  };
  refresh_project_derived_state(project);

  ++world_state.next_project_id.value;
  world_state.projects.push_back(project);
  world_state.project_count = static_cast<uint32_t>(world_state.projects.size());

  result.outcomes.push_back(make_accepted_outcome(command_index));
  result.new_projects.push_back(project.project_id);
  result.dirty_overlays.push_back(OverlayType::ProjectBlockers);
  result.dirty_settlements.push_back(project.owner_settlement_id);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  sort_and_deduplicate_settlements(result.dirty_settlements);
}

void apply_set_project_priority_command(world::WorldState& world_state, const uint32_t command_index,
                                        const SetProjectPriorityCommand& command,
                                        BatchResult& result) {
  if (!is_valid_priority_label(command.priority)) {
    result.outcomes.push_back(make_rejected_outcome(
        command_index, CommandRejectReason::InvalidPriority,
        "The requested priority label is not supported."));
    return;
  }

  ProjectState* project = find_project(world_state.projects, command.project_id);
  if (project == nullptr) {
    result.outcomes.push_back(make_rejected_outcome(command_index,
                                                    CommandRejectReason::InvalidProject,
                                                    "The project id does not exist."));
    return;
  }

  project->priority = command.priority;
  refresh_project_derived_state(*project);

  result.outcomes.push_back(make_accepted_outcome(command_index));
  if (!project->route_cell_indices.empty()) {
    const std::vector<ChunkCoord> dirty_chunks =
        route_dirty_chunks(world_state, project->route_cell_indices);
    result.dirty_chunks.insert(result.dirty_chunks.end(), dirty_chunks.begin(), dirty_chunks.end());
    append_world_dirty_chunks(world_state, dirty_chunks);
  }
  result.dirty_overlays.push_back(OverlayType::ProjectBlockers);
  result.dirty_settlements.push_back(project->owner_settlement_id);
  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  sort_and_deduplicate_settlements(result.dirty_settlements);
}

ProjectListResult build_project_list_result(const world::WorldState& world_state,
                                            const ProjectListQuery& query) {
  ProjectListResult result;

  if (settlements::find_settlement(world_state.settlements, query.settlement_id) == nullptr) {
    return result;
  }

  std::vector<const ProjectState*> matching_projects;
  matching_projects.reserve(world_state.projects.size());
  for (const ProjectState& project : world_state.projects) {
    if (project.owner_settlement_id == query.settlement_id) {
      matching_projects.push_back(&project);
    }
  }

  std::sort(matching_projects.begin(), matching_projects.end(),
            [](const ProjectState* left, const ProjectState* right) {
              return left->project_id < right->project_id;
            });

  result.projects.reserve(matching_projects.size());
  for (const ProjectState* project : matching_projects) {
    result.projects.push_back(build_project_view(*project));
  }

  return result;
}

uint32_t count_active_projects(const world::WorldState& world_state,
                               const SettlementId settlement_id) noexcept {
  return static_cast<uint32_t>(std::count_if(
      world_state.projects.begin(), world_state.projects.end(),
      [settlement_id](const ProjectState& project) {
        return project.owner_settlement_id == settlement_id &&
               project.status != ProjectStatus::Completed;
      }));
}

const ProjectState* find_project(const std::vector<ProjectState>& projects,
                                 const ProjectId project_id) noexcept {
  for (const ProjectState& project : projects) {
    if (project.project_id == project_id) {
      return &project;
    }
  }

  return nullptr;
}

ProjectState* find_project(std::vector<ProjectState>& projects, const ProjectId project_id) noexcept {
  for (ProjectState& project : projects) {
    if (project.project_id == project_id) {
      return &project;
    }
  }

  return nullptr;
}

void refresh_project_derived_state(ProjectState& project) {
  project.blocker_codes.clear();
  if (project.remaining_common_work <= 0 && project.remaining_skilled_work <= 0) {
    project.status = ProjectStatus::Completed;
    return;
  }

  if (project.priority == PriorityLabel::Paused) {
    project.blocker_codes.push_back(ProjectBlockerCode::PausedByPriority);
    project.status = ProjectStatus::Blocked;
    return;
  }

  project.status = ProjectStatus::Queued;
}

bool is_valid_priority_label(const PriorityLabel priority) noexcept {
  switch (priority) {
    case PriorityLabel::Required:
    case PriorityLabel::High:
    case PriorityLabel::Normal:
    case PriorityLabel::Low:
    case PriorityLabel::Paused:
      return true;
  }

  return false;
}

bool is_valid_project_family(const ProjectFamily family) noexcept {
  switch (family) {
    case ProjectFamily::Road:
    case ProjectFamily::Building:
      return true;
    case ProjectFamily::Founding:
    case ProjectFamily::Expansion:
      return false;
  }

  return false;
}

bool is_valid_project_status(const ProjectStatus status) noexcept {
  switch (status) {
    case ProjectStatus::Queued:
    case ProjectStatus::Blocked:
    case ProjectStatus::InProgress:
    case ProjectStatus::Completed:
      return true;
  }

  return false;
}

bool is_valid_project_blocker_code(const ProjectBlockerCode blocker_code) noexcept {
  switch (blocker_code) {
    case ProjectBlockerCode::Unknown:
    case ProjectBlockerCode::WaitingForConstructionSystem:
    case ProjectBlockerCode::PausedByPriority:
    case ProjectBlockerCode::WaitingForConstructionCapacity:
      return true;
  }

  return false;
}

bool is_valid_project_type_code(const uint8_t project_type) noexcept {
  return project_type == static_cast<uint8_t>(ProjectTypeCode::Road) ||
         project_type == static_cast<uint8_t>(ProjectTypeCode::WarehouseI);
}

std::string project_type_name(const ProjectState& project) {
  if (project.family == ProjectFamily::Road &&
      project.type == static_cast<uint8_t>(ProjectTypeCode::Road)) {
    return "Road";
  }
  if (project.family == ProjectFamily::Building &&
      project.type == static_cast<uint8_t>(ProjectTypeCode::WarehouseI)) {
    return "Warehouse I";
  }

  return "Unknown";
}

std::string project_status_name(const ProjectStatus status) {
  switch (status) {
    case ProjectStatus::Queued:
      return "Queued";
    case ProjectStatus::Blocked:
      return "Blocked";
    case ProjectStatus::InProgress:
      return "In Progress";
    case ProjectStatus::Completed:
      return "Completed";
  }

  return "Unknown";
}

std::string project_blocker_message(const ProjectBlockerCode blocker_code) {
  switch (blocker_code) {
    case ProjectBlockerCode::Unknown:
      return "Unknown project blocker.";
    case ProjectBlockerCode::WaitingForConstructionSystem:
      return "Construction progression is not implemented for queued projects yet.";
    case ProjectBlockerCode::PausedByPriority:
      return "Project priority is set to Paused.";
    case ProjectBlockerCode::WaitingForConstructionCapacity:
      return "Higher-priority projects used this month's construction capacity.";
  }

  return "Unknown project blocker.";
}

}  // namespace alpha::projects
