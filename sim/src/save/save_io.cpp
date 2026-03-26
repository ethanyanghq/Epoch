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

const char* building_type_name(const BuildingType building_type) noexcept {
  switch (building_type) {
    case BuildingType::EstateI:
      return "EstateI";
    case BuildingType::WarehouseI:
      return "WarehouseI";
  }

  return "Unknown";
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
         << settlement.labor_state.protected_food_floor_serfs << "\n";
    json << "      },\n";
    json << "      \"founding_source_settlement_id\": "
         << settlement.founding_source_settlement_id.value << ",\n";
    json << "      \"has_founding_source\": "
         << (settlement.has_founding_source ? "true" : "false") << "\n";
    json << "    }";
    if (settlement_index + 1U != sorted_settlements.size()) {
      json << ",";
    }
    json << "\n";
  }

  json << "  ],\n";
  json << "  \"zone_snapshots\": [],\n";
  json << "  \"farm_plot_snapshots\": [],\n";
  json << "  \"road_snapshot\": {\n";
  json << "    \"road_cell_indices\": []\n";
  json << "  },\n";
  json << "  \"project_snapshots\": []\n";
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
        !write_integer<uint32_t>(stream, settlement->founding_source_settlement_id.value) ||
        !write_bool(stream, settlement->has_founding_source)) {
      return false;
    }
  }

  return write_integer<uint32_t>(stream, 0U) && write_integer<uint32_t>(stream, 0U) &&
         write_integer<uint32_t>(stream, 0U) && write_integer<uint32_t>(stream, 0U);
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
        !read_integer(stream, settlement.founding_source_settlement_id.value) ||
        !read_bool(stream, settlement.has_founding_source)) {
      result.error_message = "Failed to read a settlement labor or founding snapshot.";
      return false;
    }

    settlement.food_shortage_flag = false;
    world_state.settlements.push_back(std::move(settlement));
  }

  uint32_t zone_count = 0;
  uint32_t plot_count = 0;
  uint32_t road_cell_count = 0;
  uint32_t project_count = 0;
  if (!read_integer(stream, zone_count) || !read_integer(stream, plot_count) ||
      !read_integer(stream, road_cell_count) || !read_integer(stream, project_count)) {
    result.error_message = "Failed to read trailing snapshot section counts.";
    return false;
  }
  if (zone_count != 0U || plot_count != 0U || road_cell_count != 0U || project_count != 0U) {
    result.error_message =
        "This Milestone 1 save/load path supports only saves with zero zones, plots, roads, and projects.";
    return false;
  }

  world_state.zone_count = 0;
  world_state.plot_count = 0;
  world_state.project_count = 0;
  world_state.road_cell_count = 0;
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
