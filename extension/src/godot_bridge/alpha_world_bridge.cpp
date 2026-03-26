#include "godot_bridge/alpha_world_bridge.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot_bridge {
namespace {

using godot::Array;
using godot::Dictionary;
using godot::PackedByteArray;
using godot::String;
using godot::Variant;

std::string to_std_string(const String& value) {
  return std::string(value.utf8().get_data());
}

String to_godot_string(const std::string& value) {
  return String(value.c_str());
}

template <typename IntegerType>
IntegerType dict_integer(const Dictionary& dictionary, const char* key, const IntegerType fallback = 0) {
  if (!dictionary.has(key)) {
    return fallback;
  }

  return static_cast<IntegerType>(static_cast<int64_t>(dictionary[key]));
}

String dict_string(const Dictionary& dictionary, const char* key, const String& fallback = String()) {
  if (!dictionary.has(key)) {
    return fallback;
  }

  return static_cast<String>(dictionary[key]);
}

bool dict_bool(const Dictionary& dictionary, const char* key, const bool fallback = false) {
  if (!dictionary.has(key)) {
    return fallback;
  }

  return static_cast<bool>(dictionary[key]);
}

alpha::CellCoord cell_coord_from_dict(const Dictionary& dictionary) {
  return {
      .x = dict_integer<int32_t>(dictionary, "x"),
      .y = dict_integer<int32_t>(dictionary, "y"),
  };
}

Dictionary cell_coord_to_dict(const alpha::CellCoord& coord) {
  Dictionary dictionary;
  dictionary["x"] = coord.x;
  dictionary["y"] = coord.y;
  return dictionary;
}

alpha::ChunkCoord chunk_coord_from_dict(const Dictionary& dictionary) {
  return {
      .x = dict_integer<int16_t>(dictionary, "x"),
      .y = dict_integer<int16_t>(dictionary, "y"),
  };
}

Dictionary chunk_coord_to_dict(const alpha::ChunkCoord& coord) {
  Dictionary dictionary;
  dictionary["x"] = coord.x;
  dictionary["y"] = coord.y;
  return dictionary;
}

Dictionary building_state_to_dict(const alpha::BuildingStateView& building_state) {
  Dictionary dictionary;
  dictionary["building_type"] = static_cast<int64_t>(building_state.building_type);
  dictionary["exists"] = building_state.exists;
  dictionary["staffed"] = building_state.staffed;
  return dictionary;
}

Dictionary overlay_legend_entry_to_dict(const alpha::OverlayLegendEntry& entry) {
  Dictionary dictionary;
  dictionary["value_index"] = entry.value_index;
  dictionary["label"] = to_godot_string(entry.label);
  return dictionary;
}

Dictionary project_view_to_dict(const alpha::ProjectView& project) {
  Dictionary dictionary;
  dictionary["project_id"] = static_cast<int64_t>(project.project_id.value);
  dictionary["owner_settlement_id"] = static_cast<int64_t>(project.owner_settlement_id.value);
  dictionary["family"] = static_cast<int64_t>(project.family);
  dictionary["type_name"] = to_godot_string(project.type_name);
  dictionary["priority"] = static_cast<int64_t>(project.priority);
  dictionary["status"] = static_cast<int64_t>(project.status);
  dictionary["status_name"] = to_godot_string(project.status_name);
  dictionary["required_food"] = project.required_food;
  dictionary["required_wood"] = project.required_wood;
  dictionary["required_stone"] = project.required_stone;
  dictionary["reserved_food"] = project.reserved_food;
  dictionary["reserved_wood"] = project.reserved_wood;
  dictionary["reserved_stone"] = project.reserved_stone;
  dictionary["remaining_common_work"] = project.remaining_common_work;
  dictionary["remaining_skilled_work"] = project.remaining_skilled_work;
  dictionary["access_modifier_tenths"] = project.access_modifier_tenths;

  Array blocker_codes;
  blocker_codes.resize(static_cast<int64_t>(project.blocker_codes.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(project.blocker_codes.size()); ++index) {
    blocker_codes[index] = static_cast<int64_t>(project.blocker_codes[static_cast<std::size_t>(index)]);
  }
  dictionary["blocker_codes"] = blocker_codes;

  Array blockers;
  blockers.resize(static_cast<int64_t>(project.blockers.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(project.blockers.size()); ++index) {
    blockers[index] = to_godot_string(project.blockers[static_cast<std::size_t>(index)]);
  }
  dictionary["blockers"] = blockers;

  return dictionary;
}

Dictionary settlement_summary_to_dict(const alpha::SettlementSummary& summary) {
  Dictionary dictionary;
  dictionary["settlement_id"] = static_cast<int64_t>(summary.settlement_id.value);
  dictionary["center"] = cell_coord_to_dict(summary.center);
  dictionary["population_whole"] = summary.population_whole;
  dictionary["food"] = summary.food;
  dictionary["wood"] = summary.wood;
  dictionary["stone"] = summary.stone;
  dictionary["active_zone_count"] = static_cast<int64_t>(summary.active_zone_count);
  dictionary["active_project_count"] = static_cast<int64_t>(summary.active_project_count);
  dictionary["food_shortage_flag"] = summary.food_shortage_flag;

  Array buildings;
  buildings.resize(static_cast<int64_t>(summary.buildings.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(summary.buildings.size()); ++index) {
    buildings[index] = building_state_to_dict(summary.buildings[static_cast<std::size_t>(index)]);
  }
  dictionary["buildings"] = buildings;

  return dictionary;
}

Dictionary settlement_detail_to_dict(const alpha::SettlementDetail& detail) {
  Dictionary dictionary;
  dictionary["summary"] = settlement_summary_to_dict(detail.summary);

  Dictionary role_fill;
  role_fill["serfs"] = detail.role_fill.serfs;
  role_fill["artisans"] = detail.role_fill.artisans;
  role_fill["nobles"] = detail.role_fill.nobles;
  dictionary["role_fill"] = role_fill;

  Dictionary labor_demand;
  labor_demand["protected_base"] = detail.labor_demand.protected_base;
  labor_demand["extra_roles"] = detail.labor_demand.extra_roles;
  labor_demand["idle"] = detail.labor_demand.idle;
  dictionary["labor_demand"] = labor_demand;

  Array farm_plots;
  farm_plots.resize(static_cast<int64_t>(detail.farm_plots.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(detail.farm_plots.size()); ++index) {
    const alpha::FarmPlotView& plot = detail.farm_plots[static_cast<std::size_t>(index)];
    Dictionary plot_dictionary;
    plot_dictionary["plot_id"] = static_cast<int64_t>(plot.plot_id.value);
    plot_dictionary["parent_zone_id"] = static_cast<int64_t>(plot.parent_zone_id.value);
    plot_dictionary["size"] = plot.size;
    plot_dictionary["avg_fertility_tenths"] = plot.avg_fertility_tenths;
    plot_dictionary["avg_access_cost_tenths"] = plot.avg_access_cost_tenths;
    plot_dictionary["labor_coverage_tenths"] = plot.labor_coverage_tenths;
    plot_dictionary["state_name"] = to_godot_string(plot.state_name);
    farm_plots[index] = plot_dictionary;
  }
  dictionary["farm_plots"] = farm_plots;
  dictionary["transport_capacity"] = static_cast<int64_t>(detail.transport_capacity);
  dictionary["development_pressure_tenths"] =
      static_cast<int64_t>(detail.development_pressure_tenths);
  return dictionary;
}

Dictionary world_metrics_to_dict(const alpha::WorldMetrics& metrics) {
  Dictionary dictionary;
  dictionary["settlement_count"] = static_cast<int64_t>(metrics.settlement_count);
  dictionary["zone_count"] = static_cast<int64_t>(metrics.zone_count);
  dictionary["plot_count"] = static_cast<int64_t>(metrics.plot_count);
  dictionary["project_count"] = static_cast<int64_t>(metrics.project_count);
  dictionary["road_cell_count"] = static_cast<int64_t>(metrics.road_cell_count);
  dictionary["dirty_chunk_count"] = static_cast<int64_t>(metrics.dirty_chunk_count);
  return dictionary;
}

Array dirty_chunks_to_array(const std::vector<alpha::ChunkCoord>& dirty_chunks) {
  Array chunk_array;
  chunk_array.resize(static_cast<int64_t>(dirty_chunks.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(dirty_chunks.size()); ++index) {
    chunk_array[index] = chunk_coord_to_dict(dirty_chunks[static_cast<std::size_t>(index)]);
  }
  return chunk_array;
}

Array settlement_ids_to_array(const std::vector<alpha::SettlementId>& settlement_ids) {
  Array array;
  array.resize(static_cast<int64_t>(settlement_ids.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(settlement_ids.size()); ++index) {
    array[index] = static_cast<int64_t>(settlement_ids[static_cast<std::size_t>(index)].value);
  }
  return array;
}

Array project_ids_to_array(const std::vector<alpha::ProjectId>& project_ids) {
  Array array;
  array.resize(static_cast<int64_t>(project_ids.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(project_ids.size()); ++index) {
    array[index] = static_cast<int64_t>(project_ids[static_cast<std::size_t>(index)].value);
  }
  return array;
}

Array overlay_types_to_array(const std::vector<alpha::OverlayType>& overlay_types) {
  Array array;
  array.resize(static_cast<int64_t>(overlay_types.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(overlay_types.size()); ++index) {
    array[index] = static_cast<int64_t>(overlay_types[static_cast<std::size_t>(index)]);
  }
  return array;
}

Dictionary create_world_result_to_dict(const alpha::CreateWorldResult& result) {
  Dictionary dictionary;
  dictionary["ok"] = result.ok;
  dictionary["error_message"] = to_godot_string(result.error_message);
  dictionary["dirty_chunks"] = dirty_chunks_to_array(result.dirty_chunks);
  return dictionary;
}

Dictionary load_world_result_to_dict(const alpha::LoadWorldResult& result) {
  Dictionary dictionary;
  dictionary["ok"] = result.ok;
  dictionary["error_message"] = to_godot_string(result.error_message);
  dictionary["format_version"] = static_cast<int64_t>(result.format_version);
  dictionary["dirty_chunks"] = dirty_chunks_to_array(result.dirty_chunks);
  return dictionary;
}

Dictionary save_world_result_to_dict(const alpha::SaveWorldResult& result) {
  Dictionary dictionary;
  dictionary["ok"] = result.ok;
  dictionary["error_message"] = to_godot_string(result.error_message);
  dictionary["json_debug_path"] = to_godot_string(result.json_debug_path);
  return dictionary;
}

Dictionary batch_result_to_dict(const alpha::BatchResult& result) {
  Dictionary dictionary;

  Array outcomes;
  outcomes.resize(static_cast<int64_t>(result.outcomes.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(result.outcomes.size()); ++index) {
    const alpha::CommandOutcome& outcome = result.outcomes[static_cast<std::size_t>(index)];
    Dictionary outcome_dictionary;
    outcome_dictionary["accepted"] = outcome.accepted;
    outcome_dictionary["command_index"] = static_cast<int64_t>(outcome.command_index);
    outcome_dictionary["reject_reason"] = static_cast<int64_t>(outcome.reject_reason);
    outcome_dictionary["reject_message"] = to_godot_string(outcome.reject_message);
    outcomes[index] = outcome_dictionary;
  }

  dictionary["outcomes"] = outcomes;
  dictionary["dirty_chunks"] = dirty_chunks_to_array(result.dirty_chunks);
  dictionary["dirty_overlays"] = overlay_types_to_array(result.dirty_overlays);
  dictionary["dirty_settlements"] = settlement_ids_to_array(result.dirty_settlements);
  dictionary["new_projects"] = project_ids_to_array(result.new_projects);
  return dictionary;
}

Dictionary turn_report_to_dict(const alpha::TurnReport& report) {
  Dictionary dictionary;
  dictionary["year"] = static_cast<int64_t>(report.year);
  dictionary["month"] = static_cast<int64_t>(report.month);

  Array phase_timings;
  phase_timings.resize(static_cast<int64_t>(report.phase_timings.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(report.phase_timings.size()); ++index) {
    const alpha::PhaseTiming& phase_timing = report.phase_timings[static_cast<std::size_t>(index)];
    Dictionary phase_dictionary;
    phase_dictionary["phase_name"] = to_godot_string(phase_timing.phase_name);
    phase_dictionary["duration_ms"] = static_cast<int64_t>(phase_timing.duration_ms);
    phase_timings[index] = phase_dictionary;
  }
  dictionary["phase_timings"] = phase_timings;

  Array stockpile_deltas;
  stockpile_deltas.resize(static_cast<int64_t>(report.stockpile_deltas.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(report.stockpile_deltas.size()); ++index) {
    const alpha::ResourceDelta& delta = report.stockpile_deltas[static_cast<std::size_t>(index)];
    Dictionary delta_dictionary;
    delta_dictionary["settlement_id"] = static_cast<int64_t>(delta.settlement_id.value);
    delta_dictionary["food_delta"] = delta.food_delta;
    delta_dictionary["wood_delta"] = delta.wood_delta;
    delta_dictionary["stone_delta"] = delta.stone_delta;
    stockpile_deltas[index] = delta_dictionary;
  }
  dictionary["stockpile_deltas"] = stockpile_deltas;

  Array population_deltas;
  population_deltas.resize(static_cast<int64_t>(report.population_deltas.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(report.population_deltas.size()); ++index) {
    const alpha::PopulationDelta& delta =
        report.population_deltas[static_cast<std::size_t>(index)];
    Dictionary delta_dictionary;
    delta_dictionary["settlement_id"] = static_cast<int64_t>(delta.settlement_id.value);
    delta_dictionary["whole_delta"] = delta.whole_delta;
    delta_dictionary["fractional_delta_tenths"] = delta.fractional_delta_tenths;
    delta_dictionary["starvation_occurred"] = delta.starvation_occurred;
    population_deltas[index] = delta_dictionary;
  }
  dictionary["population_deltas"] = population_deltas;

  dictionary["completed_projects"] = project_ids_to_array(report.completed_projects);
  dictionary["newly_blocked_projects"] = project_ids_to_array(report.newly_blocked_projects);
  dictionary["newly_unblocked_projects"] = project_ids_to_array(report.newly_unblocked_projects);
  dictionary["shortage_settlements"] = settlement_ids_to_array(report.shortage_settlements);
  dictionary["dirty_chunks"] = dirty_chunks_to_array(report.dirty_chunks);
  dictionary["dirty_overlays"] = overlay_types_to_array(report.dirty_overlays);
  return dictionary;
}

Dictionary chunk_visual_to_dict(const alpha::ChunkVisualResult& result) {
  Dictionary dictionary;
  dictionary["chunk"] = chunk_coord_to_dict(result.chunk);
  dictionary["width"] = result.width;
  dictionary["height"] = result.height;

  PackedByteArray terrain_color_indices;
  PackedByteArray road_flags;
  PackedByteArray settlement_flags;
  PackedByteArray reserved_flags;
  terrain_color_indices.resize(static_cast<int64_t>(result.cells.size()));
  road_flags.resize(static_cast<int64_t>(result.cells.size()));
  settlement_flags.resize(static_cast<int64_t>(result.cells.size()));
  reserved_flags.resize(static_cast<int64_t>(result.cells.size()));

  for (int64_t index = 0; index < static_cast<int64_t>(result.cells.size()); ++index) {
    const alpha::ChunkVisualCell& cell = result.cells[static_cast<std::size_t>(index)];
    terrain_color_indices.set(index, cell.terrain_color_index);
    road_flags.set(index, cell.road_flag);
    settlement_flags.set(index, cell.settlement_flag);
    reserved_flags.set(index, cell.reserved);
  }

  dictionary["terrain_color_indices"] = terrain_color_indices;
  dictionary["road_flags"] = road_flags;
  dictionary["settlement_flags"] = settlement_flags;
  dictionary["reserved_flags"] = reserved_flags;
  return dictionary;
}

Dictionary overlay_chunk_to_dict(const alpha::OverlayChunkResult& result) {
  Dictionary dictionary;
  dictionary["chunk"] = chunk_coord_to_dict(result.chunk);
  dictionary["overlay_type"] = static_cast<int64_t>(result.overlay_type);
  dictionary["width"] = result.width;
  dictionary["height"] = result.height;

  PackedByteArray values;
  values.resize(static_cast<int64_t>(result.values.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(result.values.size()); ++index) {
    values.set(index, result.values[static_cast<std::size_t>(index)]);
  }
  dictionary["values"] = values;

  Array legend;
  legend.resize(static_cast<int64_t>(result.legend.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(result.legend.size()); ++index) {
    legend[index] = overlay_legend_entry_to_dict(result.legend[static_cast<std::size_t>(index)]);
  }
  dictionary["legend"] = legend;
  return dictionary;
}

alpha::WorldCreateParams create_world_params_from_dict(const Dictionary& dictionary) {
  return {
      .terrain_seed = dict_integer<uint64_t>(dictionary, "terrain_seed"),
      .gameplay_seed = dict_integer<uint64_t>(dictionary, "gameplay_seed"),
      .map_width = dict_integer<uint16_t>(dictionary, "map_width"),
      .map_height = dict_integer<uint16_t>(dictionary, "map_height"),
      .generation_config_path = to_std_string(dict_string(dictionary, "generation_config_path")),
  };
}

alpha::LoadWorldParams load_world_params_from_dict(const Dictionary& dictionary) {
  return {
      .path = to_std_string(dict_string(dictionary, "path")),
  };
}

alpha::SaveWorldParams save_world_params_from_dict(const Dictionary& dictionary) {
  return {
      .path = to_std_string(dict_string(dictionary, "path")),
      .write_json_debug_export = dict_bool(dictionary, "write_json_debug_export"),
  };
}

alpha::ChunkVisualQuery chunk_visual_query_from_dict(const Dictionary& dictionary) {
  return {
      .chunk = chunk_coord_from_dict(static_cast<Dictionary>(dictionary.get("chunk", Dictionary()))),
      .layer_index = dict_integer<uint8_t>(dictionary, "layer_index"),
  };
}

alpha::OverlayChunkQuery overlay_chunk_query_from_dict(const Dictionary& dictionary) {
  return {
      .chunk = chunk_coord_from_dict(static_cast<Dictionary>(dictionary.get("chunk", Dictionary()))),
      .overlay_type = static_cast<alpha::OverlayType>(dict_integer<int64_t>(dictionary, "overlay_type")),
  };
}

alpha::ProjectListQuery project_list_query_from_dict(const Dictionary& dictionary) {
  return {
      .settlement_id = alpha::SettlementId{
          static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "settlement_id"))},
  };
}

std::vector<alpha::CellCoord> cell_vector_from_array(const Array& cell_array) {
  std::vector<alpha::CellCoord> cells;
  cells.reserve(static_cast<std::size_t>(cell_array.size()));
  for (int64_t index = 0; index < cell_array.size(); ++index) {
    cells.push_back(cell_coord_from_dict(static_cast<Dictionary>(cell_array[index])));
  }
  return cells;
}

alpha::CommandVariant parse_command_dictionary(const Dictionary& dictionary, String& error_message) {
  const String command_type = dict_string(dictionary, "type");

  if (command_type == "zone_cells") {
    return alpha::ZoneCellsCommand{
        .settlement_id =
            alpha::SettlementId{static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "settlement_id"))},
        .zone_type = static_cast<alpha::ZoneType>(dict_integer<int64_t>(dictionary, "zone_type")),
        .cells = cell_vector_from_array(static_cast<Array>(dictionary.get("cells", Array()))),
    };
  }

  if (command_type == "remove_zone_cells") {
    return alpha::RemoveZoneCellsCommand{
        .settlement_id =
            alpha::SettlementId{static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "settlement_id"))},
        .cells = cell_vector_from_array(static_cast<Array>(dictionary.get("cells", Array()))),
    };
  }

  if (command_type == "queue_road") {
    return alpha::QueueRoadCommand{
        .settlement_id =
            alpha::SettlementId{static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "settlement_id"))},
        .route_cells = cell_vector_from_array(static_cast<Array>(dictionary.get("route_cells", Array()))),
        .priority =
            static_cast<alpha::PriorityLabel>(dict_integer<int64_t>(dictionary, "priority")),
    };
  }

  if (command_type == "queue_building") {
    return alpha::QueueBuildingCommand{
        .settlement_id =
            alpha::SettlementId{static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "settlement_id"))},
        .building_type =
            static_cast<alpha::BuildingType>(dict_integer<int64_t>(dictionary, "building_type")),
        .priority =
            static_cast<alpha::PriorityLabel>(dict_integer<int64_t>(dictionary, "priority")),
    };
  }

  if (command_type == "queue_founding") {
    return alpha::QueueFoundingCommand{
        .source_settlement_id = alpha::SettlementId{
            static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "source_settlement_id"))},
        .target_center =
            cell_coord_from_dict(static_cast<Dictionary>(dictionary.get("target_center", Dictionary()))),
        .priority =
            static_cast<alpha::PriorityLabel>(dict_integer<int64_t>(dictionary, "priority")),
    };
  }

  if (command_type == "set_project_priority") {
    return alpha::SetProjectPriorityCommand{
        .project_id =
            alpha::ProjectId{static_cast<uint32_t>(dict_integer<int64_t>(dictionary, "project_id"))},
        .priority =
            static_cast<alpha::PriorityLabel>(dict_integer<int64_t>(dictionary, "priority")),
    };
  }

  error_message = "Unknown command type: " + command_type;
  return alpha::RemoveZoneCellsCommand{};
}

}  // namespace

void AlphaWorldBridge::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("create_world", "params"),
                              &AlphaWorldBridge::create_world);
  godot::ClassDB::bind_method(godot::D_METHOD("load_world", "params"),
                              &AlphaWorldBridge::load_world);
  godot::ClassDB::bind_method(godot::D_METHOD("save_world", "params"),
                              &AlphaWorldBridge::save_world);
  godot::ClassDB::bind_method(godot::D_METHOD("apply_commands", "batch"),
                              &AlphaWorldBridge::apply_commands);
  godot::ClassDB::bind_method(godot::D_METHOD("advance_month"),
                              &AlphaWorldBridge::advance_month);
  godot::ClassDB::bind_method(godot::D_METHOD("get_chunk_visual", "query"),
                              &AlphaWorldBridge::get_chunk_visual);
  godot::ClassDB::bind_method(godot::D_METHOD("get_overlay_chunk", "query"),
                              &AlphaWorldBridge::get_overlay_chunk);
  godot::ClassDB::bind_method(godot::D_METHOD("get_settlement_summary", "settlement_id"),
                              &AlphaWorldBridge::get_settlement_summary);
  godot::ClassDB::bind_method(godot::D_METHOD("get_settlement_detail", "settlement_id"),
                              &AlphaWorldBridge::get_settlement_detail);
  godot::ClassDB::bind_method(godot::D_METHOD("get_projects", "query"),
                              &AlphaWorldBridge::get_projects);
  godot::ClassDB::bind_method(godot::D_METHOD("get_world_metrics"),
                              &AlphaWorldBridge::get_world_metrics);
}

Dictionary AlphaWorldBridge::create_world(const Dictionary& params) {
  return create_world_result_to_dict(world_api_.create_world(create_world_params_from_dict(params)));
}

Dictionary AlphaWorldBridge::load_world(const Dictionary& params) {
  return load_world_result_to_dict(world_api_.load_world(load_world_params_from_dict(params)));
}

Dictionary AlphaWorldBridge::save_world(const Dictionary& params) const {
  return save_world_result_to_dict(world_api_.save_world(save_world_params_from_dict(params)));
}

Dictionary AlphaWorldBridge::apply_commands(const Dictionary& batch) {
  alpha::CommandBatch command_batch;
  const Array commands = static_cast<Array>(batch.get("commands", Array()));
  command_batch.commands.reserve(static_cast<std::size_t>(commands.size()));

  for (int64_t index = 0; index < commands.size(); ++index) {
    String error_message;
    alpha::CommandVariant command =
        parse_command_dictionary(static_cast<Dictionary>(commands[index]), error_message);
    if (!error_message.is_empty()) {
      Dictionary error_result;
      Array outcomes;
      Dictionary outcome;
      outcome["accepted"] = false;
      outcome["command_index"] = index;
      outcome["reject_reason"] = static_cast<int64_t>(alpha::CommandRejectReason::Unknown);
      outcome["reject_message"] = error_message;
      outcomes.push_back(outcome);
      error_result["outcomes"] = outcomes;
      error_result["dirty_chunks"] = Array();
      error_result["dirty_overlays"] = Array();
      error_result["dirty_settlements"] = Array();
      error_result["new_projects"] = Array();
      return error_result;
    }
    command_batch.commands.push_back(std::move(command));
  }

  return batch_result_to_dict(world_api_.apply_commands(command_batch));
}

Dictionary AlphaWorldBridge::advance_month() {
  return turn_report_to_dict(world_api_.advance_month());
}

Dictionary AlphaWorldBridge::get_chunk_visual(const Dictionary& query) const {
  return chunk_visual_to_dict(world_api_.get_chunk_visual(chunk_visual_query_from_dict(query)));
}

Dictionary AlphaWorldBridge::get_overlay_chunk(const Dictionary& query) const {
  return overlay_chunk_to_dict(world_api_.get_overlay_chunk(overlay_chunk_query_from_dict(query)));
}

Dictionary AlphaWorldBridge::get_settlement_summary(const int64_t settlement_id) const {
  return settlement_summary_to_dict(
      world_api_.get_settlement_summary(alpha::SettlementId{static_cast<uint32_t>(settlement_id)}));
}

Dictionary AlphaWorldBridge::get_settlement_detail(const int64_t settlement_id) const {
  return settlement_detail_to_dict(
      world_api_.get_settlement_detail(alpha::SettlementId{static_cast<uint32_t>(settlement_id)}));
}

Dictionary AlphaWorldBridge::get_projects(const Dictionary& query) const {
  const alpha::ProjectListResult result = world_api_.get_projects(project_list_query_from_dict(query));
  Dictionary dictionary;
  Array projects;
  projects.resize(static_cast<int64_t>(result.projects.size()));
  for (int64_t index = 0; index < static_cast<int64_t>(result.projects.size()); ++index) {
    projects[index] = project_view_to_dict(result.projects[static_cast<std::size_t>(index)]);
  }
  dictionary["projects"] = projects;
  return dictionary;
}

Dictionary AlphaWorldBridge::get_world_metrics() const {
  return world_metrics_to_dict(world_api_.get_world_metrics());
}

}  // namespace godot_bridge
