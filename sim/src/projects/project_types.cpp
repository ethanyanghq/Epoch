#include "alpha/projects/project_types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <string>

#include "alpha/api/constants.hpp"
#include "alpha/map/map_types.hpp"
#include "alpha/map/map_grid.hpp"
#include "alpha/settlements/settlement_types.hpp"
#include "alpha/world/world_state.hpp"

namespace alpha::projects {
namespace {

constexpr WorkAmountTenths kPrototypeRoadWorkPerCellTenths = 10;

struct RoadValidationResult {
  bool ok = false;
  CommandRejectReason reject_reason = CommandRejectReason::Unknown;
  std::string reject_message;
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

RoadValidationResult validate_queue_road_command(const world::WorldState& world_state,
                                                 const QueueRoadCommand& command) {
  RoadValidationResult validation;

  if (!is_valid_priority_label(command.priority)) {
    validation.reject_reason = CommandRejectReason::InvalidPriority;
    validation.reject_message = "The requested priority label is not supported.";
    return validation;
  }

  if (settlements::find_settlement(world_state.settlements, command.settlement_id) == nullptr) {
    validation.reject_reason = CommandRejectReason::InvalidSettlement;
    validation.reject_message = "The settlement id does not exist.";
    return validation;
  }

  if (command.route_cells.empty()) {
    validation.reject_reason = CommandRejectReason::InvalidCells;
    validation.reject_message = "QueueRoadCommand requires at least one route cell.";
    return validation;
  }

  validation.route_cell_indices.reserve(command.route_cells.size());
  for (std::size_t index = 0; index < command.route_cells.size(); ++index) {
    const CellCoord cell = command.route_cells[index];
    if (!world_state.map_grid.contains_cell(cell.x, cell.y)) {
      validation.reject_reason = CommandRejectReason::InvalidCells;
      validation.reject_message = "QueueRoadCommand contains out-of-bounds route cells.";
      validation.route_cell_indices.clear();
      return validation;
    }

    if (index > 0U && !is_four_neighbor_step(command.route_cells[index - 1U], cell)) {
      validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.reject_message =
          "QueueRoadCommand route cells must form a contiguous 4-neighbor path.";
      validation.route_cell_indices.clear();
      return validation;
    }

    const uint32_t cell_index = to_cell_index(world_state, cell.x, cell.y);
    if (std::find(validation.route_cell_indices.begin(), validation.route_cell_indices.end(),
                  cell_index) != validation.route_cell_indices.end()) {
      validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.reject_message =
          "QueueRoadCommand route cells must form a contiguous 4-neighbor path.";
      validation.route_cell_indices.clear();
      return validation;
    }

    const map::MapCell& map_cell = world_state.map_grid.cell(cell.x, cell.y);
    if (!is_prototype_road_cell_legal(map_cell)) {
      validation.reject_reason = CommandRejectReason::IllegalRoadRoute;
      validation.reject_message =
          "QueueRoadCommand route cells contain terrain that is illegal for prototype road placement.";
      validation.route_cell_indices.clear();
      return validation;
    }

    validation.route_cell_indices.push_back(cell_index);
  }

  validation.ok = true;
  return validation;
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
  if (!validation.ok) {
    result.outcomes.push_back(
        make_rejected_outcome(command_index, validation.reject_reason, validation.reject_message));
    return;
  }

  ProjectState project{
      .project_id = world_state.next_project_id,
      .owner_settlement_id = command.settlement_id,
      .family = ProjectFamily::Road,
      .type = static_cast<uint8_t>(ProjectTypeCode::PrototypeRoad),
      .target = to_cell_coord(world_state, validation.route_cell_indices.back()),
      .route_cell_indices = validation.route_cell_indices,
      .priority = command.priority,
      .status = ProjectStatus::Queued,
      .required_materials = {},
      .reserved_materials = {},
      .consumed_materials = {},
      .remaining_common_work = static_cast<WorkAmountTenths>(
          validation.route_cell_indices.size() * kPrototypeRoadWorkPerCellTenths),
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
  const std::vector<ChunkCoord> dirty_chunks =
      route_dirty_chunks(world_state, project->route_cell_indices);
  result.dirty_chunks.insert(result.dirty_chunks.end(), dirty_chunks.begin(), dirty_chunks.end());
  result.dirty_overlays.push_back(OverlayType::ProjectBlockers);
  result.dirty_settlements.push_back(project->owner_settlement_id);
  sort_and_deduplicate_chunks(result.dirty_chunks);
  sort_and_deduplicate_overlays(result.dirty_overlays);
  sort_and_deduplicate_settlements(result.dirty_settlements);
  append_world_dirty_chunks(world_state, dirty_chunks);
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
  if (project.priority == PriorityLabel::Paused) {
    project.blocker_codes.push_back(ProjectBlockerCode::PausedByPriority);
  }
  project.blocker_codes.push_back(ProjectBlockerCode::WaitingForConstructionSystem);
  project.status = ProjectStatus::Blocked;
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
      return true;
    case ProjectFamily::Building:
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
      return true;
  }

  return false;
}

bool is_valid_project_type_code(const uint8_t project_type) noexcept {
  return project_type == static_cast<uint8_t>(ProjectTypeCode::PrototypeRoad);
}

std::string project_type_name(const ProjectState& project) {
  if (project.family == ProjectFamily::Road &&
      project.type == static_cast<uint8_t>(ProjectTypeCode::PrototypeRoad)) {
    return "Prototype Road";
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
  }

  return "Unknown project blocker.";
}

}  // namespace alpha::projects
