#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "alpha/api/query_types.hpp"
#include "alpha/api/result_types.hpp"

namespace alpha::world {
struct WorldState;
}

namespace alpha::projects {

enum class ProjectTypeCode : uint8_t {
  PrototypeRoad = 1,
};

struct ProjectResourceState {
  ResourceAmountTenths food = 0;
  ResourceAmountTenths wood = 0;
  ResourceAmountTenths stone = 0;
};

struct ProjectState {
  ProjectId project_id;
  SettlementId owner_settlement_id;
  ProjectFamily family = ProjectFamily::Road;
  uint8_t type = static_cast<uint8_t>(ProjectTypeCode::PrototypeRoad);
  CellCoord target;
  std::vector<uint32_t> route_cell_indices;
  PriorityLabel priority = PriorityLabel::Normal;
  ProjectStatus status = ProjectStatus::Queued;
  ProjectResourceState required_materials;
  ProjectResourceState reserved_materials;
  ProjectResourceState consumed_materials;
  WorkAmountTenths remaining_common_work = 0;
  WorkAmountTenths remaining_skilled_work = 0;
  int32_t access_modifier_tenths = 0;
  std::vector<ProjectBlockerCode> blocker_codes;
};

void apply_queue_road_command(world::WorldState& world_state, uint32_t command_index,
                              const QueueRoadCommand& command, BatchResult& result);
void apply_set_project_priority_command(world::WorldState& world_state, uint32_t command_index,
                                        const SetProjectPriorityCommand& command,
                                        BatchResult& result);

ProjectListResult build_project_list_result(const world::WorldState& world_state,
                                            const ProjectListQuery& query);
uint32_t count_active_projects(const world::WorldState& world_state,
                               SettlementId settlement_id) noexcept;

const ProjectState* find_project(const std::vector<ProjectState>& projects,
                                 ProjectId project_id) noexcept;
ProjectState* find_project(std::vector<ProjectState>& projects, ProjectId project_id) noexcept;

void refresh_project_derived_state(ProjectState& project);

bool is_valid_priority_label(PriorityLabel priority) noexcept;
bool is_valid_project_family(ProjectFamily family) noexcept;
bool is_valid_project_status(ProjectStatus status) noexcept;
bool is_valid_project_blocker_code(ProjectBlockerCode blocker_code) noexcept;
bool is_valid_project_type_code(uint8_t project_type) noexcept;

std::string project_type_name(const ProjectState& project);
std::string project_status_name(ProjectStatus status);
std::string project_blocker_message(ProjectBlockerCode blocker_code);

}  // namespace alpha::projects
