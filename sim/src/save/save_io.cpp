#include "alpha/save/save_io.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

#include "alpha/map/map_types.hpp"
#include "alpha/projects/project_types.hpp"
#include "alpha/zones/zone_types.hpp"

namespace alpha::save {
namespace {

template <typename Integer>
bool write_integer(std::ostream& stream, Integer value) {
  using UnsignedInteger = std::make_unsigned_t<Integer>;

  UnsignedInteger unsigned_value = static_cast<UnsignedInteger>(value);
  std::array<char, sizeof(Integer)> bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] = static_cast<char>((unsigned_value >> (index * 8U)) & 0xFFU);
  }

  stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return stream.good();
}

template <typename Integer>
bool read_integer(std::istream& stream, Integer& value) {
  using UnsignedInteger = std::make_unsigned_t<Integer>;

  std::array<unsigned char, sizeof(Integer)> bytes{};
  stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!stream.good()) {
    return false;
  }

  UnsignedInteger unsigned_value = 0;
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    unsigned_value |= static_cast<UnsignedInteger>(bytes[index]) << (index * 8U);
  }

  value = static_cast<Integer>(unsigned_value);
  return true;
}

bool write_bool(std::ostream& stream, const bool value) {
  return write_integer<uint8_t>(stream, value ? 1U : 0U);
}

bool read_bool(std::istream& stream, bool& value) {
  uint8_t encoded = 0;
  if (!read_integer(stream, encoded)) {
    return false;
  }

  if (encoded > 1U) {
    return false;
  }

  value = encoded != 0U;
  return true;
}

bool write_string(std::ostream& stream, const std::string& value) {
  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(value.size()))) {
    return false;
  }

  if (!value.empty()) {
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
  }
  return stream.good();
}

bool read_string(std::istream& stream, std::string& value) {
  uint32_t length = 0;
  if (!read_integer(stream, length)) {
    return false;
  }

  value.assign(length, '\0');
  if (length == 0) {
    return true;
  }

  stream.read(value.data(), static_cast<std::streamsize>(length));
  return stream.good();
}

bool write_u32_vector(std::ostream& stream, const std::vector<uint32_t>& values) {
  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(values.size()))) {
    return false;
  }

  for (const uint32_t value : values) {
    if (!write_integer<uint32_t>(stream, value)) {
      return false;
    }
  }

  return true;
}

bool read_u32_vector(std::istream& stream, std::vector<uint32_t>& values) {
  uint32_t count = 0;
  if (!read_integer(stream, count)) {
    return false;
  }

  values.assign(count, 0);
  for (uint32_t index = 0; index < count; ++index) {
    if (!read_integer(stream, values[index])) {
      return false;
    }
  }

  return true;
}

bool write_u32_values(std::ostream& stream, const std::vector<uint32_t>& values) {
  for (const uint32_t value : values) {
    if (!write_integer<uint32_t>(stream, value)) {
      return false;
    }
  }

  return true;
}

bool read_n_u32_values(std::istream& stream, const uint32_t count, std::vector<uint32_t>& values) {
  values.assign(count, 0U);
  for (uint32_t index = 0; index < count; ++index) {
    if (!read_integer<uint32_t>(stream, values[index])) {
      return false;
    }
  }

  return true;
}

bool write_u16_vector(std::ostream& stream, const std::vector<uint16_t>& values) {
  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(values.size()))) {
    return false;
  }

  for (const uint16_t value : values) {
    if (!write_integer<uint16_t>(stream, value)) {
      return false;
    }
  }

  return true;
}

bool read_u16_vector(std::istream& stream, std::vector<uint16_t>& values) {
  uint32_t count = 0;
  if (!read_integer(stream, count)) {
    return false;
  }

  values.assign(count, 0);
  for (uint32_t index = 0; index < count; ++index) {
    if (!read_integer(stream, values[index])) {
      return false;
    }
  }

  return true;
}

bool write_byte_array(std::ostream& stream, const std::vector<uint8_t>& values) {
  if (!values.empty()) {
    stream.write(reinterpret_cast<const char*>(values.data()),
                 static_cast<std::streamsize>(values.size()));
  }
  return stream.good();
}

bool read_byte_array(std::istream& stream, const std::size_t count, std::vector<uint8_t>& values) {
  values.assign(count, 0);
  if (count == 0) {
    return true;
  }

  stream.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(count));
  return stream.good();
}

bool is_supported_building_type(const uint8_t encoded_type) noexcept {
  return encoded_type <= static_cast<uint8_t>(BuildingType::WarehouseI);
}

bool is_supported_zone_type(const uint8_t encoded_type) noexcept {
  return encoded_type <= static_cast<uint8_t>(ZoneType::Quarry);
}

bool is_supported_farm_plot_state(const uint8_t encoded_state) noexcept {
  return encoded_state <= static_cast<uint8_t>(settlements::FarmPlotStateCode::Fallow);
}

const char* building_type_name(const BuildingType building_type) noexcept {
  switch (building_type) {
    case BuildingType::EstateI:
      return "EstateI";
    case BuildingType::WarehouseI:
      return "WarehouseI";
  }

  return "Unknown";
}

const char* zone_type_name(const ZoneType zone_type) noexcept {
  switch (zone_type) {
    case ZoneType::Farmland:
      return "Farmland";
    case ZoneType::Forestry:
      return "Forestry";
    case ZoneType::Quarry:
      return "Quarry";
  }

  return "Unknown";
}

const char* project_family_name(const ProjectFamily family) noexcept {
  switch (family) {
    case ProjectFamily::Road:
      return "Road";
    case ProjectFamily::Building:
      return "Building";
    case ProjectFamily::Founding:
      return "Founding";
    case ProjectFamily::Expansion:
      return "Expansion";
  }

  return "Unknown";
}

const char* priority_label_name(const PriorityLabel priority) noexcept {
  switch (priority) {
    case PriorityLabel::Required:
      return "Required";
    case PriorityLabel::High:
      return "High";
    case PriorityLabel::Normal:
      return "Normal";
    case PriorityLabel::Low:
      return "Low";
    case PriorityLabel::Paused:
      return "Paused";
  }

  return "Unknown";
}

const char* project_blocker_code_name(const ProjectBlockerCode blocker_code) noexcept {
  switch (blocker_code) {
    case ProjectBlockerCode::Unknown:
      return "Unknown";
    case ProjectBlockerCode::WaitingForConstructionSystem:
      return "WaitingForConstructionSystem";
    case ProjectBlockerCode::PausedByPriority:
      return "PausedByPriority";
    case ProjectBlockerCode::WaitingForConstructionCapacity:
      return "WaitingForConstructionCapacity";
  }

  return "Unknown";
}

bool write_project_resource_state(std::ostream& stream,
                                  const projects::ProjectResourceState& resource_state) {
  return write_integer<int32_t>(stream, resource_state.food) &&
         write_integer<int32_t>(stream, resource_state.wood) &&
         write_integer<int32_t>(stream, resource_state.stone);
}

bool read_project_resource_state(std::istream& stream,
                                 projects::ProjectResourceState& resource_state) {
  return read_integer(stream, resource_state.food) && read_integer(stream, resource_state.wood) &&
         read_integer(stream, resource_state.stone);
}

std::string escape_json_string(std::string_view value) {
  std::string escaped;
  escaped.reserve(value.size());

  for (const char character : value) {
    switch (character) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += character;
        break;
    }
  }

  return escaped;
}

std::filesystem::path build_json_debug_path(const std::string& binary_path) {
  std::filesystem::path json_path(binary_path);
  json_path += ".json";
  return json_path;
}

std::string build_json_debug_export(const world::WorldState& world_state) {
  const std::size_t cell_count = static_cast<std::size_t>(world_state.map_width) *
                                 static_cast<std::size_t>(world_state.map_height);

  std::vector<uint8_t> elevation;
  std::vector<uint8_t> slope;
  std::vector<uint8_t> water;
  std::vector<uint8_t> fertility;
  std::vector<uint8_t> vegetation;
  elevation.reserve(cell_count);
  slope.reserve(cell_count);
  water.reserve(cell_count);
  fertility.reserve(cell_count);
  vegetation.reserve(cell_count);

  for (int32_t y = 0; y < world_state.map_height; ++y) {
    for (int32_t x = 0; x < world_state.map_width; ++x) {
      const map::MapCell& cell = world_state.map_grid.cell(x, y);
      elevation.push_back(cell.elevation);
      slope.push_back(cell.slope);
      water.push_back(cell.water);
      fertility.push_back(cell.fertility);
      vegetation.push_back(cell.vegetation);
    }
  }

  std::ostringstream json;
  json << "{\n";
  json << "  \"save_header\": {\n";
  json << "    \"format_version\": " << kSaveFormatVersion << ",\n";
  json << "    \"ruleset_version\": " << kRulesetVersion << ",\n";
  json << "    \"generator_version\": " << kGeneratorVersion << ",\n";
  json << "    \"map_width\": " << world_state.map_width << ",\n";
  json << "    \"map_height\": " << world_state.map_height << "\n";
  json << "  },\n";
  json << "  \"calendar_state\": {\n";
  json << "    \"year\": " << world_state.calendar.year << ",\n";
  json << "    \"month_index\": " << world_state.calendar.month << "\n";
  json << "  },\n";
  json << "  \"map_snapshot\": {\n";
  json << "    \"terrain_seed\": " << world_state.terrain_seed << ",\n";
  json << "    \"generation_config_id\": \""
       << escape_json_string(world_state.generation_config_path) << "\",\n";

  auto write_json_byte_array = [&json](std::string_view field_name,
                                       const std::vector<uint8_t>& values,
                                       const bool trailing_comma) {
    json << "    \"" << field_name << "\": [";
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index != 0) {
        json << ", ";
      }
      json << static_cast<uint32_t>(values[index]);
    }
    json << "]";
    if (trailing_comma) {
      json << ",";
    }
    json << "\n";
  };

  write_json_byte_array("elevation", elevation, true);
  write_json_byte_array("slope", slope, true);
  write_json_byte_array("water", water, true);
  write_json_byte_array("fertility", fertility, true);
  write_json_byte_array("vegetation", vegetation, false);
  json << "  },\n";
  json << "  \"world_rng_state\": {\n";
  json << "    \"seed\": " << world_state.world_rng.seed << ",\n";
  json << "    \"stream_state\": " << world_state.world_rng.stream_state << "\n";
  json << "  },\n";
  json << "  \"settlement_snapshots\": [\n";

  std::vector<const settlements::SettlementState*> sorted_settlements;
  sorted_settlements.reserve(world_state.settlements.size());
  for (const settlements::SettlementState& settlement : world_state.settlements) {
    sorted_settlements.push_back(&settlement);
  }
  std::sort(sorted_settlements.begin(), sorted_settlements.end(),
            [](const settlements::SettlementState* left, const settlements::SettlementState* right) {
              return left->settlement_id < right->settlement_id;
            });

  for (std::size_t settlement_index = 0; settlement_index < sorted_settlements.size();
       ++settlement_index) {
    const settlements::SettlementState& settlement = *sorted_settlements[settlement_index];
    std::vector<uint32_t> footprint = settlement.footprint_cell_indices;
    std::sort(footprint.begin(), footprint.end());

    json << "    {\n";
    json << "      \"id\": " << settlement.settlement_id.value << ",\n";
    json << "      \"center_x\": " << settlement.center.x << ",\n";
    json << "      \"center_y\": " << settlement.center.y << ",\n";
    json << "      \"footprint_cell_indices\": [";
    for (std::size_t index = 0; index < footprint.size(); ++index) {
      if (index != 0) {
        json << ", ";
      }
      json << footprint[index];
    }
    json << "],\n";
    json << "      \"population_whole\": " << settlement.population_whole << ",\n";
    json << "      \"population_fraction_tenths\": " << settlement.population_fraction_tenths << ",\n";
    json << "      \"population_change_basis_points\": "
         << settlement.population_change_basis_points << ",\n";
    json << "      \"development_pressure_tenths\": "
         << settlement.development_pressure_tenths << ",\n";
    json << "      \"stockpile\": {\n";
    json << "        \"food_tenths\": " << settlement.stockpile.food << ",\n";
    json << "        \"wood_tenths\": " << settlement.stockpile.wood << ",\n";
    json << "        \"stone_tenths\": " << settlement.stockpile.stone << "\n";
    json << "      },\n";
    json << "      \"buildings\": [\n";
    for (std::size_t building_index = 0; building_index < settlement.buildings.size();
         ++building_index) {
      const settlements::BuildingState& building = settlement.buildings[building_index];
      json << "        {\n";
      json << "          \"building_type\": \""
           << building_type_name(building.building_type) << "\",\n";
      json << "          \"exists\": " << (building.exists ? "true" : "false") << ",\n";
      json << "          \"staffed\": " << (building.staffed ? "true" : "false") << "\n";
      json << "        }";
      if (building_index + 1U != settlement.buildings.size()) {
        json << ",";
      }
      json << "\n";
    }
    json << "      ],\n";
    json << "      \"labor_state\": {\n";
    json << "        \"last_serf_fill\": " << settlement.labor_state.last_serf_fill << ",\n";
    json << "        \"last_artisan_fill\": " << settlement.labor_state.last_artisan_fill << ",\n";
    json << "        \"last_noble_fill\": " << settlement.labor_state.last_noble_fill << ",\n";
    json << "        \"reassignment_budget_tenths\": "
         << settlement.labor_state.reassignment_budget_tenths << ",\n";
    json << "        \"protected_food_floor_serfs\": "
         << settlement.labor_state.protected_food_floor_serfs << ",\n";
    json << "        \"protected_base_demand\": "
         << settlement.labor_state.protected_base_demand << ",\n";
    json << "        \"extra_role_demand\": "
         << settlement.labor_state.extra_role_demand << ",\n";
    json << "        \"idle_fill\": " << settlement.labor_state.idle_fill << "\n";
    json << "      },\n";
    json << "      \"founding_source_settlement_id\": "
         << settlement.founding_source_settlement_id.value << ",\n";
    json << "      \"has_founding_source\": "
         << (settlement.has_founding_source ? "true" : "false") << ",\n";
    json << "      \"food_shortage_flag\": "
         << (settlement.food_shortage_flag ? "true" : "false") << "\n";
    json << "    }";
    if (settlement_index + 1U != sorted_settlements.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ],\n";
  json << "  \"zone_snapshots\": [\n";

  std::vector<const zones::ZoneState*> sorted_zones;
  sorted_zones.reserve(world_state.zones.size());
  for (const zones::ZoneState& zone : world_state.zones) {
    sorted_zones.push_back(&zone);
  }
  std::sort(sorted_zones.begin(), sorted_zones.end(),
            [](const zones::ZoneState* left, const zones::ZoneState* right) {
              return left->zone_id < right->zone_id;
            });

  for (std::size_t zone_index = 0; zone_index < sorted_zones.size(); ++zone_index) {
    const zones::ZoneState& zone = *sorted_zones[zone_index];
    json << "    {\n";
    json << "      \"id\": " << zone.zone_id.value << ",\n";
    json << "      \"owner_settlement_id\": " << zone.owner_settlement_id.value << ",\n";
    json << "      \"zone_type\": \"" << zone_type_name(zone.zone_type) << "\",\n";
    json << "      \"member_cell_indices\": [";
    for (std::size_t index = 0; index < zone.member_cell_indices.size(); ++index) {
      if (index != 0) {
        json << ", ";
      }
      json << zone.member_cell_indices[index];
    }
    json << "]\n";
    json << "    }";
    if (zone_index + 1U != sorted_zones.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ],\n";
  json << "  \"farm_plot_snapshots\": [\n";

  std::vector<const settlements::FarmPlotState*> sorted_plots;
  sorted_plots.reserve(world_state.farm_plots.size());
  for (const settlements::FarmPlotState& plot : world_state.farm_plots) {
    sorted_plots.push_back(&plot);
  }
  std::sort(sorted_plots.begin(), sorted_plots.end(),
            [](const settlements::FarmPlotState* left, const settlements::FarmPlotState* right) {
              return left->plot_id < right->plot_id;
            });

  for (std::size_t plot_index = 0; plot_index < sorted_plots.size(); ++plot_index) {
    const settlements::FarmPlotState& plot = *sorted_plots[plot_index];
    json << "    {\n";
    json << "      \"id\": " << plot.plot_id.value << ",\n";
    json << "      \"parent_zone_id\": " << plot.parent_zone_id.value << ",\n";
    json << "      \"cell_indices\": [";
    for (std::size_t index = 0; index < plot.cell_indices.size(); ++index) {
      if (index != 0) {
        json << ", ";
      }
      json << plot.cell_indices[index];
    }
    json << "],\n";
    json << "      \"state\": \"" << settlements::farm_plot_state_name(plot.state) << "\",\n";
    json << "      \"avg_fertility_tenths\": " << plot.avg_fertility_tenths << ",\n";
    json << "      \"avg_access_cost_tenths\": " << plot.avg_access_cost_tenths << ",\n";
    json << "      \"forested_flag\": " << (plot.forested_flag ? "true" : "false") << ",\n";
    json << "      \"labor_coverage_tenths\": " << plot.labor_coverage_tenths << ",\n";
    json << "      \"current_year_required_labor\": " << plot.current_year_required_labor << ",\n";
    json << "      \"current_year_assigned_labor\": " << plot.current_year_assigned_labor << ",\n";
    json << "      \"opening_months_remaining\": " << plot.opening_months_remaining << ",\n";
    json << "      \"opening_work_remaining_tenths\": " << plot.opening_work_remaining_tenths << "\n";
    json << "    }";
    if (plot_index + 1U != sorted_plots.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ],\n";
  json << "  \"road_snapshot\": {\n";
  json << "    \"road_cell_indices\": [";
  bool first_road_cell = true;
  for (uint32_t cell_index = 0; cell_index < world_state.road_cells.size(); ++cell_index) {
    if (world_state.road_cells[cell_index] == 0U) {
      continue;
    }
    if (!first_road_cell) {
      json << ", ";
    }
    json << cell_index;
    first_road_cell = false;
  }
  json << "]\n";
  json << "  },\n";
  json << "  \"project_snapshots\": [\n";

  std::vector<const projects::ProjectState*> sorted_projects;
  sorted_projects.reserve(world_state.projects.size());
  for (const projects::ProjectState& project : world_state.projects) {
    sorted_projects.push_back(&project);
  }
  std::sort(sorted_projects.begin(), sorted_projects.end(),
            [](const projects::ProjectState* left, const projects::ProjectState* right) {
              return left->project_id < right->project_id;
            });

  for (std::size_t project_index = 0; project_index < sorted_projects.size(); ++project_index) {
    const projects::ProjectState& project = *sorted_projects[project_index];
    json << "    {\n";
    json << "      \"id\": " << project.project_id.value << ",\n";
    json << "      \"owner_settlement_id\": " << project.owner_settlement_id.value << ",\n";
    json << "      \"family\": \"" << project_family_name(project.family) << "\",\n";
    json << "      \"type\": " << static_cast<uint32_t>(project.type) << ",\n";
    json << "      \"type_name\": \"" << escape_json_string(projects::project_type_name(project))
         << "\",\n";
    json << "      \"target_x\": " << project.target.x << ",\n";
    json << "      \"target_y\": " << project.target.y << ",\n";
    json << "      \"route_cell_indices\": [";
    for (std::size_t route_index = 0; route_index < project.route_cell_indices.size();
         ++route_index) {
      if (route_index != 0U) {
        json << ", ";
      }
      json << project.route_cell_indices[route_index];
    }
    json << "],\n";
    json << "      \"priority\": \"" << priority_label_name(project.priority) << "\",\n";
    json << "      \"status\": \"" << escape_json_string(projects::project_status_name(project.status))
         << "\",\n";
    json << "      \"required_materials\": {\n";
    json << "        \"food_tenths\": " << project.required_materials.food << ",\n";
    json << "        \"wood_tenths\": " << project.required_materials.wood << ",\n";
    json << "        \"stone_tenths\": " << project.required_materials.stone << "\n";
    json << "      },\n";
    json << "      \"reserved_materials\": {\n";
    json << "        \"food_tenths\": " << project.reserved_materials.food << ",\n";
    json << "        \"wood_tenths\": " << project.reserved_materials.wood << ",\n";
    json << "        \"stone_tenths\": " << project.reserved_materials.stone << "\n";
    json << "      },\n";
    json << "      \"consumed_materials\": {\n";
    json << "        \"food_tenths\": " << project.consumed_materials.food << ",\n";
    json << "        \"wood_tenths\": " << project.consumed_materials.wood << ",\n";
    json << "        \"stone_tenths\": " << project.consumed_materials.stone << "\n";
    json << "      },\n";
    json << "      \"remaining_common_work_tenths\": " << project.remaining_common_work << ",\n";
    json << "      \"remaining_skilled_work_tenths\": " << project.remaining_skilled_work << ",\n";
    json << "      \"access_modifier_tenths\": " << project.access_modifier_tenths << ",\n";
    json << "      \"blocker_codes\": [";
    for (std::size_t blocker_index = 0; blocker_index < project.blocker_codes.size();
         ++blocker_index) {
      if (blocker_index != 0U) {
        json << ", ";
      }
      json << "\"" << project_blocker_code_name(project.blocker_codes[blocker_index]) << "\"";
    }
    json << "]\n";
    json << "    }";
    if (project_index + 1U != sorted_projects.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ]\n";
  json << "}\n";
  return json.str();
}

bool serialize_world(std::ostream& stream, const world::WorldState& world_state) {
  if (!write_integer<uint32_t>(stream, kSaveFormatVersion) ||
      !write_integer<uint32_t>(stream, kRulesetVersion) ||
      !write_integer<uint32_t>(stream, kGeneratorVersion) ||
      !write_integer<uint16_t>(stream, world_state.map_width) ||
      !write_integer<uint16_t>(stream, world_state.map_height) ||
      !write_integer<uint16_t>(stream, world_state.calendar.year) ||
      !write_integer<uint16_t>(stream, world_state.calendar.month) ||
      !write_integer<uint64_t>(stream, world_state.terrain_seed) ||
      !write_string(stream, world_state.generation_config_path)) {
    return false;
  }

  std::vector<uint8_t> elevation;
  std::vector<uint8_t> slope;
  std::vector<uint8_t> water;
  std::vector<uint8_t> fertility;
  std::vector<uint8_t> vegetation;
  const std::size_t cell_count = static_cast<std::size_t>(world_state.map_width) *
                                 static_cast<std::size_t>(world_state.map_height);
  elevation.reserve(cell_count);
  slope.reserve(cell_count);
  water.reserve(cell_count);
  fertility.reserve(cell_count);
  vegetation.reserve(cell_count);

  for (int32_t y = 0; y < world_state.map_height; ++y) {
    for (int32_t x = 0; x < world_state.map_width; ++x) {
      const map::MapCell& cell = world_state.map_grid.cell(x, y);
      elevation.push_back(cell.elevation);
      slope.push_back(cell.slope);
      water.push_back(cell.water);
      fertility.push_back(cell.fertility);
      vegetation.push_back(cell.vegetation);
    }
  }

  if (!write_byte_array(stream, elevation) || !write_byte_array(stream, slope) ||
      !write_byte_array(stream, water) || !write_byte_array(stream, fertility) ||
      !write_byte_array(stream, vegetation) ||
      !write_integer<uint64_t>(stream, world_state.world_rng.seed) ||
      !write_integer<uint64_t>(stream, world_state.world_rng.stream_state)) {
    return false;
  }

  std::vector<const settlements::SettlementState*> sorted_settlements;
  sorted_settlements.reserve(world_state.settlements.size());
  for (const settlements::SettlementState& settlement : world_state.settlements) {
    sorted_settlements.push_back(&settlement);
  }
  std::sort(sorted_settlements.begin(), sorted_settlements.end(),
            [](const settlements::SettlementState* left, const settlements::SettlementState* right) {
              return left->settlement_id < right->settlement_id;
            });

  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(sorted_settlements.size()))) {
    return false;
  }

  for (const settlements::SettlementState* settlement : sorted_settlements) {
    std::vector<uint32_t> footprint = settlement->footprint_cell_indices;
    std::sort(footprint.begin(), footprint.end());

    if (!write_integer<uint32_t>(stream, settlement->settlement_id.value) ||
        !write_integer<int32_t>(stream, settlement->center.x) ||
        !write_integer<int32_t>(stream, settlement->center.y) ||
        !write_u32_vector(stream, footprint) ||
        !write_integer<int32_t>(stream, settlement->population_whole) ||
        !write_integer<int32_t>(stream, settlement->population_fraction_tenths) ||
        !write_integer<int32_t>(stream, settlement->population_change_basis_points) ||
        !write_integer<int32_t>(stream, settlement->development_pressure_tenths) ||
        !write_integer<int32_t>(stream, settlement->stockpile.food) ||
        !write_integer<int32_t>(stream, settlement->stockpile.wood) ||
        !write_integer<int32_t>(stream, settlement->stockpile.stone)) {
      return false;
    }

    for (const settlements::BuildingState& building : settlement->buildings) {
      if (!write_integer<uint8_t>(stream, static_cast<uint8_t>(building.building_type)) ||
          !write_bool(stream, building.exists) || !write_bool(stream, building.staffed)) {
        return false;
      }
    }

    if (!write_integer<int32_t>(stream, settlement->labor_state.last_serf_fill) ||
        !write_integer<int32_t>(stream, settlement->labor_state.last_artisan_fill) ||
        !write_integer<int32_t>(stream, settlement->labor_state.last_noble_fill) ||
        !write_integer<int32_t>(stream, settlement->labor_state.reassignment_budget_tenths) ||
        !write_integer<int32_t>(stream, settlement->labor_state.protected_food_floor_serfs) ||
        !write_integer<int32_t>(stream, settlement->labor_state.protected_base_demand) ||
        !write_integer<int32_t>(stream, settlement->labor_state.extra_role_demand) ||
        !write_integer<int32_t>(stream, settlement->labor_state.idle_fill) ||
        !write_integer<uint32_t>(stream, settlement->founding_source_settlement_id.value) ||
        !write_bool(stream, settlement->has_founding_source) ||
        !write_bool(stream, settlement->food_shortage_flag)) {
      return false;
    }
  }

  std::vector<const zones::ZoneState*> sorted_zones;
  sorted_zones.reserve(world_state.zones.size());
  for (const zones::ZoneState& zone : world_state.zones) {
    sorted_zones.push_back(&zone);
  }
  std::sort(sorted_zones.begin(), sorted_zones.end(),
            [](const zones::ZoneState* left, const zones::ZoneState* right) {
              return left->zone_id < right->zone_id;
            });

  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(sorted_zones.size()))) {
    return false;
  }

  for (const zones::ZoneState* zone : sorted_zones) {
    std::vector<uint32_t> member_cells = zone->member_cell_indices;
    std::sort(member_cells.begin(), member_cells.end());

    if (!write_integer<uint32_t>(stream, zone->zone_id.value) ||
        !write_integer<uint32_t>(stream, zone->owner_settlement_id.value) ||
        !write_integer<uint8_t>(stream, static_cast<uint8_t>(zone->zone_type)) ||
        !write_u32_vector(stream, member_cells)) {
      return false;
    }
  }

  std::vector<uint32_t> road_cell_indices;
  road_cell_indices.reserve(world_state.road_cell_count);
  for (uint32_t cell_index = 0; cell_index < world_state.road_cells.size(); ++cell_index) {
    if (world_state.road_cells[cell_index] != 0U) {
      road_cell_indices.push_back(cell_index);
    }
  }

  std::vector<const settlements::FarmPlotState*> sorted_plots_for_binary;
  sorted_plots_for_binary.reserve(world_state.farm_plots.size());
  for (const settlements::FarmPlotState& plot : world_state.farm_plots) {
    sorted_plots_for_binary.push_back(&plot);
  }
  std::sort(sorted_plots_for_binary.begin(), sorted_plots_for_binary.end(),
            [](const settlements::FarmPlotState* left, const settlements::FarmPlotState* right) {
              return left->plot_id < right->plot_id;
            });

  if (!write_integer<uint32_t>(stream, static_cast<uint32_t>(sorted_plots_for_binary.size()))) {
    return false;
  }

  for (const settlements::FarmPlotState* plot : sorted_plots_for_binary) {
    std::vector<uint32_t> sorted_cells = plot->cell_indices;
    std::sort(sorted_cells.begin(), sorted_cells.end());
    if (!write_integer<uint32_t>(stream, plot->plot_id.value) ||
        !write_integer<uint32_t>(stream, plot->parent_zone_id.value) ||
        !write_u32_vector(stream, sorted_cells) ||
        !write_integer<uint8_t>(stream, static_cast<uint8_t>(plot->state)) ||
        !write_integer<uint16_t>(stream, plot->avg_fertility_tenths) ||
        !write_integer<uint16_t>(stream, plot->avg_access_cost_tenths) ||
        !write_bool(stream, plot->forested_flag) ||
        !write_integer<uint16_t>(stream, plot->labor_coverage_tenths) ||
        !write_integer<int32_t>(stream, plot->current_year_required_labor) ||
        !write_integer<int32_t>(stream, plot->current_year_assigned_labor) ||
        !write_integer<int32_t>(stream, plot->opening_months_remaining) ||
        !write_integer<int32_t>(stream, plot->opening_work_remaining_tenths)) {
      return false;
    }
  }

  if (
      !write_integer<uint32_t>(stream, static_cast<uint32_t>(road_cell_indices.size())) ||
      !write_u32_values(stream, road_cell_indices) ||
      !write_integer<uint32_t>(stream, static_cast<uint32_t>(world_state.projects.size()))) {
    return false;
  }

  std::vector<const projects::ProjectState*> sorted_projects;
  sorted_projects.reserve(world_state.projects.size());
  for (const projects::ProjectState& project : world_state.projects) {
    sorted_projects.push_back(&project);
  }
  std::sort(sorted_projects.begin(), sorted_projects.end(),
            [](const projects::ProjectState* left, const projects::ProjectState* right) {
              return left->project_id < right->project_id;
            });

  for (const projects::ProjectState* project : sorted_projects) {
    std::vector<uint16_t> blocker_codes;
    blocker_codes.reserve(project->blocker_codes.size());
    for (const ProjectBlockerCode blocker_code : project->blocker_codes) {
      blocker_codes.push_back(static_cast<uint16_t>(blocker_code));
    }

    if (!write_integer<uint32_t>(stream, project->project_id.value) ||
        !write_integer<uint32_t>(stream, project->owner_settlement_id.value) ||
        !write_integer<uint8_t>(stream, static_cast<uint8_t>(project->family)) ||
        !write_integer<uint8_t>(stream, project->type) ||
        !write_integer<int32_t>(stream, project->target.x) ||
        !write_integer<int32_t>(stream, project->target.y) ||
        !write_u32_vector(stream, project->route_cell_indices) ||
        !write_integer<uint8_t>(stream, static_cast<uint8_t>(project->priority)) ||
        !write_integer<uint8_t>(stream, static_cast<uint8_t>(project->status)) ||
        !write_project_resource_state(stream, project->required_materials) ||
        !write_project_resource_state(stream, project->reserved_materials) ||
        !write_project_resource_state(stream, project->consumed_materials) ||
        !write_integer<int32_t>(stream, project->remaining_common_work) ||
        !write_integer<int32_t>(stream, project->remaining_skilled_work) ||
        !write_integer<int32_t>(stream, project->access_modifier_tenths) ||
        !write_u16_vector(stream, blocker_codes)) {
      return false;
    }
  }

  return true;
}

bool deserialize_world(std::istream& stream, LoadWorldStateResult& result) {
  uint32_t format_version = 0;
  uint32_t ruleset_version = 0;
  uint32_t generator_version = 0;
  uint16_t map_width = 0;
  uint16_t map_height = 0;
  if (!read_integer(stream, format_version) || !read_integer(stream, ruleset_version) ||
      !read_integer(stream, generator_version) || !read_integer(stream, map_width) ||
      !read_integer(stream, map_height)) {
    result.error_message = "Failed to read the save header.";
    return false;
  }

  result.format_version = format_version;
  if (format_version != kSaveFormatVersion) {
    result.error_message = "Unsupported save format version.";
    return false;
  }
  if (ruleset_version != kRulesetVersion) {
    result.error_message = "Unsupported save ruleset version.";
    return false;
  }
  if (generator_version != kGeneratorVersion) {
    result.error_message = "Unsupported save generator version.";
    return false;
  }
  if (!world::is_supported_world_size(map_width, map_height)) {
    result.error_message = "Saved world dimensions are not supported by the current runtime.";
    return false;
  }

  world::WorldState world_state;
  world_state.map_width = map_width;
  world_state.map_height = map_height;

  if (!read_integer(stream, world_state.calendar.year) ||
      !read_integer(stream, world_state.calendar.month)) {
    result.error_message = "Failed to read the saved calendar state.";
    return false;
  }
  if (world_state.calendar.month < 1U || world_state.calendar.month > 12U) {
    result.error_message = "Saved month index is out of range.";
    return false;
  }

  if (!read_integer(stream, world_state.terrain_seed) ||
      !read_string(stream, world_state.generation_config_path)) {
    result.error_message = "Failed to read the saved map snapshot.";
    return false;
  }

  const std::size_t cell_count =
      static_cast<std::size_t>(world_state.map_width) * static_cast<std::size_t>(world_state.map_height);
  std::vector<uint8_t> elevation;
  std::vector<uint8_t> slope;
  std::vector<uint8_t> water;
  std::vector<uint8_t> fertility;
  std::vector<uint8_t> vegetation;
  if (!read_byte_array(stream, cell_count, elevation) || !read_byte_array(stream, cell_count, slope) ||
      !read_byte_array(stream, cell_count, water) ||
      !read_byte_array(stream, cell_count, fertility) ||
      !read_byte_array(stream, cell_count, vegetation)) {
    result.error_message = "Failed to read the saved terrain arrays.";
    return false;
  }

  std::vector<map::MapCell> cells(cell_count);
  for (std::size_t index = 0; index < cell_count; ++index) {
    cells[index] = map::MapCell{
        .elevation = elevation[index],
        .slope = slope[index],
        .water = water[index],
        .fertility = fertility[index],
        .vegetation = vegetation[index],
    };
  }

  if (!world_state.map_grid.initialize_from_cells(world_state.map_width, world_state.map_height,
                                                  std::move(cells))) {
    result.error_message = "Failed to rebuild the authoritative map grid from the save.";
    return false;
  }

  if (!read_integer(stream, world_state.world_rng.seed) ||
      !read_integer(stream, world_state.world_rng.stream_state)) {
    result.error_message = "Failed to read the saved world RNG state.";
    return false;
  }

  uint32_t settlement_count = 0;
  if (!read_integer(stream, settlement_count)) {
    result.error_message = "Failed to read the settlement snapshot count.";
    return false;
  }

  world_state.settlements.clear();
  world_state.settlements.reserve(settlement_count);
  for (uint32_t settlement_index = 0; settlement_index < settlement_count; ++settlement_index) {
    settlements::SettlementState settlement;
    if (!read_integer(stream, settlement.settlement_id.value) ||
        !read_integer(stream, settlement.center.x) ||
        !read_integer(stream, settlement.center.y) ||
        !read_u32_vector(stream, settlement.footprint_cell_indices) ||
        !read_integer(stream, settlement.population_whole) ||
        !read_integer(stream, settlement.population_fraction_tenths) ||
        !read_integer(stream, settlement.population_change_basis_points) ||
        !read_integer(stream, settlement.development_pressure_tenths) ||
        !read_integer(stream, settlement.stockpile.food) ||
        !read_integer(stream, settlement.stockpile.wood) ||
        !read_integer(stream, settlement.stockpile.stone)) {
      result.error_message = "Failed to read a settlement snapshot.";
      return false;
    }

    for (settlements::BuildingState& building : settlement.buildings) {
      uint8_t encoded_building_type = 0;
      if (!read_integer(stream, encoded_building_type) || !is_supported_building_type(encoded_building_type) ||
          !read_bool(stream, building.exists) || !read_bool(stream, building.staffed)) {
        result.error_message = "Failed to read a settlement building snapshot.";
        return false;
      }

      building.building_type = static_cast<BuildingType>(encoded_building_type);
    }

    if (!read_integer(stream, settlement.labor_state.last_serf_fill) ||
        !read_integer(stream, settlement.labor_state.last_artisan_fill) ||
        !read_integer(stream, settlement.labor_state.last_noble_fill) ||
        !read_integer(stream, settlement.labor_state.reassignment_budget_tenths) ||
        !read_integer(stream, settlement.labor_state.protected_food_floor_serfs) ||
        !read_integer(stream, settlement.labor_state.protected_base_demand) ||
        !read_integer(stream, settlement.labor_state.extra_role_demand) ||
        !read_integer(stream, settlement.labor_state.idle_fill) ||
        !read_integer(stream, settlement.founding_source_settlement_id.value) ||
        !read_bool(stream, settlement.has_founding_source) ||
        !read_bool(stream, settlement.food_shortage_flag)) {
      result.error_message = "Failed to read a settlement labor or founding snapshot.";
      return false;
    }
    world_state.settlements.push_back(std::move(settlement));
  }

  uint32_t zone_count = 0;
  zones::initialize_zone_state(world_state);

  if (!read_integer(stream, zone_count)) {
    result.error_message = "Failed to read the zone snapshot count.";
    return false;
  }

  world_state.zones.clear();
  world_state.zones.reserve(zone_count);
  uint32_t max_zone_id = 0;
  for (uint32_t zone_index = 0; zone_index < zone_count; ++zone_index) {
    zones::ZoneState zone;
    uint8_t encoded_zone_type = 0;
    if (!read_integer(stream, zone.zone_id.value) ||
        !read_integer(stream, zone.owner_settlement_id.value) ||
        !read_integer(stream, encoded_zone_type) ||
        !is_supported_zone_type(encoded_zone_type) ||
        !read_u32_vector(stream, zone.member_cell_indices)) {
      result.error_message = "Failed to read a zone snapshot.";
      return false;
    }

    zone.zone_type = static_cast<ZoneType>(encoded_zone_type);
    std::sort(zone.member_cell_indices.begin(), zone.member_cell_indices.end());

    for (const uint32_t cell_index : zone.member_cell_indices) {
      if (cell_index >= world_state.zone_cells.size()) {
        result.error_message = "A saved zone references an out-of-bounds cell index.";
        return false;
      }

      zones::CellZoneState& cell_zone = world_state.zone_cells[cell_index];
      if (cell_zone.occupied) {
        result.error_message = "Saved zones overlap on at least one cell.";
        return false;
      }

      cell_zone.occupied = true;
      cell_zone.zone_id = zone.zone_id;
      cell_zone.owner_settlement_id = zone.owner_settlement_id;
      cell_zone.zone_type = zone.zone_type;
    }

    max_zone_id = std::max(max_zone_id, zone.zone_id.value);
    world_state.zones.push_back(std::move(zone));
  }

  world_state.next_zone_id = ZoneId{max_zone_id + 1U};

  uint32_t plot_count = 0;
  uint32_t road_cell_count = 0;
  uint32_t project_count = 0;
  std::vector<uint32_t> road_cell_indices;
  if (!read_integer(stream, plot_count)) {
    result.error_message = "Failed to read the farm plot snapshot count.";
    return false;
  }

  world_state.farm_plots.clear();
  world_state.farm_plots.reserve(plot_count);
  uint32_t max_plot_id = 0;
  for (uint32_t plot_index = 0; plot_index < plot_count; ++plot_index) {
    settlements::FarmPlotState plot;
    uint8_t encoded_state = 0;
    if (!read_integer(stream, plot.plot_id.value) ||
        !read_integer(stream, plot.parent_zone_id.value) ||
        !read_u32_vector(stream, plot.cell_indices) ||
        !read_integer(stream, encoded_state) ||
        !is_supported_farm_plot_state(encoded_state) ||
        !read_integer(stream, plot.avg_fertility_tenths) ||
        !read_integer(stream, plot.avg_access_cost_tenths) ||
        !read_bool(stream, plot.forested_flag) ||
        !read_integer(stream, plot.labor_coverage_tenths) ||
        !read_integer(stream, plot.current_year_required_labor) ||
        !read_integer(stream, plot.current_year_assigned_labor) ||
        !read_integer(stream, plot.opening_months_remaining) ||
        !read_integer(stream, plot.opening_work_remaining_tenths)) {
      result.error_message = "Failed to read a farm plot snapshot.";
      return false;
    }

    plot.state = static_cast<settlements::FarmPlotStateCode>(encoded_state);
    max_plot_id = std::max(max_plot_id, plot.plot_id.value);
    world_state.farm_plots.push_back(std::move(plot));
  }

  world_state.next_farm_plot_id = FarmPlotId{max_plot_id + 1U};

  if (!read_integer(stream, road_cell_count) ||
      !read_n_u32_values(stream, road_cell_count, road_cell_indices) ||
      !read_integer(stream, project_count)) {
    result.error_message = "Failed to read trailing snapshot section counts.";
    return false;
  }

  world_state.road_cells.assign(cell_count, 0U);
  for (const uint32_t road_cell_index : road_cell_indices) {
    if (road_cell_index >= cell_count) {
      result.error_message = "A saved road snapshot references an out-of-bounds cell index.";
      return false;
    }
    if (world_state.road_cells[road_cell_index] != 0U) {
      result.error_message = "A saved road snapshot contains duplicate road cells.";
      return false;
    }
    world_state.road_cells[road_cell_index] = 1U;
  }

  world_state.projects.clear();
  world_state.projects.reserve(project_count);
  uint32_t max_project_id = 0;
  for (uint32_t project_index = 0; project_index < project_count; ++project_index) {
    projects::ProjectState project;
    uint8_t encoded_family = 0;
    uint8_t encoded_priority = 0;
    uint8_t encoded_status = 0;
    std::vector<uint16_t> blocker_codes;
    if (!read_integer(stream, project.project_id.value) ||
        !read_integer(stream, project.owner_settlement_id.value) ||
        !read_integer(stream, encoded_family) || !read_integer(stream, project.type) ||
        !read_integer(stream, project.target.x) || !read_integer(stream, project.target.y) ||
        !read_u32_vector(stream, project.route_cell_indices) ||
        !read_integer(stream, encoded_priority) || !read_integer(stream, encoded_status) ||
        !read_project_resource_state(stream, project.required_materials) ||
        !read_project_resource_state(stream, project.reserved_materials) ||
        !read_project_resource_state(stream, project.consumed_materials) ||
        !read_integer(stream, project.remaining_common_work) ||
        !read_integer(stream, project.remaining_skilled_work) ||
        !read_integer(stream, project.access_modifier_tenths) ||
        !read_u16_vector(stream, blocker_codes)) {
      result.error_message = "Failed to read a project snapshot.";
      return false;
    }

    project.family = static_cast<ProjectFamily>(encoded_family);
    project.priority = static_cast<PriorityLabel>(encoded_priority);
    project.status = static_cast<ProjectStatus>(encoded_status);

    if (!projects::is_valid_project_family(project.family) ||
        !projects::is_valid_project_type_code(project.type) ||
        !projects::is_valid_priority_label(project.priority) ||
        !projects::is_valid_project_status(project.status)) {
      result.error_message = "A saved project snapshot contains unsupported enum values.";
      return false;
    }

    if (settlements::find_settlement(world_state.settlements, project.owner_settlement_id) == nullptr) {
      result.error_message = "A saved project references an unknown owner settlement.";
      return false;
    }

    if ((project.family == ProjectFamily::Road &&
         project.type != static_cast<uint8_t>(projects::ProjectTypeCode::Road)) ||
        (project.family == ProjectFamily::Building &&
         project.type != static_cast<uint8_t>(projects::ProjectTypeCode::WarehouseI))) {
      result.error_message = "A saved project snapshot contains an unsupported family/type combination.";
      return false;
    }

    if (project.family == ProjectFamily::Road) {
      if (project.route_cell_indices.empty()) {
        result.error_message = "A saved road project must contain at least one route cell.";
        return false;
      }

      for (const uint32_t route_cell_index : project.route_cell_indices) {
        if (route_cell_index >= cell_count) {
          result.error_message = "A saved project references an out-of-bounds route cell index.";
          return false;
        }
      }
    } else if (!project.route_cell_indices.empty()) {
      result.error_message = "A saved building project should not persist a route path.";
      return false;
    }

    project.blocker_codes.clear();
    project.blocker_codes.reserve(blocker_codes.size());
    for (const uint16_t encoded_blocker_code : blocker_codes) {
      const ProjectBlockerCode blocker_code = static_cast<ProjectBlockerCode>(encoded_blocker_code);
      if (!projects::is_valid_project_blocker_code(blocker_code)) {
        result.error_message = "A saved project contains an unsupported blocker code.";
        return false;
      }
      project.blocker_codes.push_back(blocker_code);
    }

    max_project_id = std::max(max_project_id, project.project_id.value);
    world_state.projects.push_back(std::move(project));
  }

  std::sort(world_state.projects.begin(), world_state.projects.end(),
            [](const projects::ProjectState& left, const projects::ProjectState& right) {
              return left.project_id < right.project_id;
            });
  world_state.next_project_id = ProjectId{max_project_id + 1U};

  zones::rebuild_zone_state(world_state);
  std::sort(world_state.farm_plots.begin(), world_state.farm_plots.end(),
            [](const settlements::FarmPlotState& left, const settlements::FarmPlotState& right) {
              return left.plot_id < right.plot_id;
            });
  world_state.plot_count = static_cast<uint32_t>(world_state.farm_plots.size());
  world_state.project_count = static_cast<uint32_t>(world_state.projects.size());
  world_state.road_cell_count = road_cell_count;
  world_state.dirty_chunks = world::make_all_chunk_coords(world_state.map_width, world_state.map_height);

  result.ok = true;
  result.world_state = std::move(world_state);
  return true;
}

}  // namespace

SaveWorldResult save_world(const world::WorldState& world_state, const SaveWorldParams& params) {
  if (params.path.empty()) {
    return {
        .ok = false,
        .error_message = "Save path must not be empty.",
    };
  }

  std::ofstream binary_stream(params.path, std::ios::binary | std::ios::trunc);
  if (!binary_stream.is_open()) {
    return {
        .ok = false,
        .error_message = "Failed to open the save path for writing.",
    };
  }

  if (!serialize_world(binary_stream, world_state)) {
    return {
        .ok = false,
        .error_message = "Failed to serialize the world snapshot.",
    };
  }

  binary_stream.flush();
  if (!binary_stream.good()) {
    return {
        .ok = false,
        .error_message = "Failed to flush the binary world snapshot to disk.",
    };
  }

  SaveWorldResult result{
      .ok = true,
  };

  if (!params.write_json_debug_export) {
    return result;
  }

  const std::filesystem::path json_path = build_json_debug_path(params.path);
  std::ofstream json_stream(json_path, std::ios::trunc);
  if (!json_stream.is_open()) {
    return {
        .ok = false,
        .error_message = "Binary save succeeded but the JSON debug export path could not be opened.",
    };
  }

  json_stream << build_json_debug_export(world_state);
  json_stream.flush();
  if (!json_stream.good()) {
    return {
        .ok = false,
        .error_message = "Binary save succeeded but the JSON debug export could not be written.",
    };
  }

  result.json_debug_path = json_path.string();
  return result;
}

LoadWorldStateResult load_world(const LoadWorldParams& params) {
  LoadWorldStateResult result;
  if (params.path.empty()) {
    result.error_message = "Load path must not be empty.";
    return result;
  }

  std::ifstream binary_stream(params.path, std::ios::binary);
  if (!binary_stream.is_open()) {
    result.error_message = "Failed to open the save path for reading.";
    return result;
  }

  deserialize_world(binary_stream, result);
  return result;
}

}  // namespace alpha::save
