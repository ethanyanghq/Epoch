extends Control

const WorldBridgeScript := preload("res://scripts/data_view/world_bridge.gd")

const DEFAULT_WORLD_CREATE_PARAMS := {
	"terrain_seed": 11,
	"gameplay_seed": 29,
	"map_width": 1024,
	"map_height": 1024,
	"generation_config_path": "data/generation/default_alpha.json"
}

const DEFAULT_SETTLEMENT_ID := 1

var _bridge = WorldBridgeScript.new()
var _world_loaded := false
var _current_settlement_id := DEFAULT_SETTLEMENT_ID
var _last_turn_report: Dictionary = {}

var _path_edit: LineEdit
var _status_label: Label
var _summary_label: RichTextLabel
var _detail_label: RichTextLabel
var _advance_button: Button
var _save_button: Button


func _ready() -> void:
	_build_ui()
	_path_edit.text = OS.get_user_data_dir().path_join("alpha_m1_save.bin")

	var init_result: Dictionary = _bridge.initialize()
	if not init_result.get("ok", false):
		_set_status("Bridge initialization failed: %s" % init_result.get("error_message", "unknown error"))
		_set_interaction_enabled(false)
		return

	_set_status("Native bridge initialized.")
	var user_args := OS.get_cmdline_user_args()
	var load_path := _extract_load_path(user_args)
	if load_path.is_empty():
		_create_world()
	else:
		_path_edit.text = load_path
		_load_world(load_path)

	if _world_loaded and _has_flag(user_args, "--advance-month-on-startup"):
		_advance_month()

	var save_path := _extract_save_path(user_args)
	if _world_loaded and not save_path.is_empty():
		_path_edit.text = save_path
		_save_world(save_path)


func _build_ui() -> void:
	var margin := MarginContainer.new()
	margin.set_anchors_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 24)
	margin.add_theme_constant_override("margin_top", 24)
	margin.add_theme_constant_override("margin_right", 24)
	margin.add_theme_constant_override("margin_bottom", 24)
	add_child(margin)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 12)
	margin.add_child(root)

	var title := Label.new()
	title.text = "Epoch Alpha Milestone 1 Shell"
	title.add_theme_font_size_override("font_size", 26)
	root.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Thin Godot shell over the native AlphaWorldBridge."
	root.add_child(subtitle)

	var path_row := HBoxContainer.new()
	path_row.add_theme_constant_override("separation", 8)
	root.add_child(path_row)

	_path_edit = LineEdit.new()
	_path_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_path_edit.placeholder_text = "Saved world path"
	path_row.add_child(_path_edit)

	var load_button := Button.new()
	load_button.text = "Load World"
	load_button.pressed.connect(_on_load_pressed)
	path_row.add_child(load_button)

	_save_button = Button.new()
	_save_button.text = "Save World"
	_save_button.pressed.connect(_on_save_pressed)
	path_row.add_child(_save_button)

	var action_row := HBoxContainer.new()
	action_row.add_theme_constant_override("separation", 8)
	root.add_child(action_row)

	var create_button := Button.new()
	create_button.text = "Create New World"
	create_button.pressed.connect(_on_create_pressed)
	action_row.add_child(create_button)

	_advance_button = Button.new()
	_advance_button.text = "Advance One Month"
	_advance_button.pressed.connect(_on_advance_pressed)
	action_row.add_child(_advance_button)

	_status_label = Label.new()
	_status_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	root.add_child(_status_label)

	_summary_label = RichTextLabel.new()
	_summary_label.fit_content = true
	_summary_label.bbcode_enabled = false
	_summary_label.custom_minimum_size = Vector2(0, 180)
	root.add_child(_summary_label)

	_detail_label = RichTextLabel.new()
	_detail_label.fit_content = true
	_detail_label.bbcode_enabled = false
	_detail_label.custom_minimum_size = Vector2(0, 220)
	root.add_child(_detail_label)


func _set_interaction_enabled(enabled: bool) -> void:
	if _advance_button != null:
		_advance_button.disabled = not enabled or not _world_loaded
	if _save_button != null:
		_save_button.disabled = not enabled or not _world_loaded


func _extract_load_path(user_args: PackedStringArray) -> String:
	for index in range(user_args.size()):
		var arg := user_args[index]
		if arg.begins_with("--load-world="):
			return arg.trim_prefix("--load-world=")
		if arg == "--load-world" and index + 1 < user_args.size():
			return user_args[index + 1]
	return ""


func _extract_save_path(user_args: PackedStringArray) -> String:
	for index in range(user_args.size()):
		var arg := user_args[index]
		if arg.begins_with("--save-world="):
			return arg.trim_prefix("--save-world=")
		if arg == "--save-world" and index + 1 < user_args.size():
			return user_args[index + 1]
	return ""


func _has_flag(user_args: PackedStringArray, flag: String) -> bool:
	for arg in user_args:
		if arg == flag:
			return true
	return false


func _create_world() -> void:
	var result: Dictionary = _bridge.create_world(DEFAULT_WORLD_CREATE_PARAMS)
	if not result.get("ok", false):
		_world_loaded = false
		_set_interaction_enabled(true)
		_set_status("Create world failed: %s" % result.get("error_message", "unknown error"))
		return

	_world_loaded = true
	_current_settlement_id = DEFAULT_SETTLEMENT_ID
	_last_turn_report = {}
	_refresh_read_models()
	_set_interaction_enabled(true)
	_set_status("Created a new 1024x1024 world through the native bridge.")


func _load_world(path: String) -> void:
	if path.is_empty():
		_set_status("Load world failed: no path was provided.")
		return

	var result: Dictionary = _bridge.load_world({"path": path})
	if not result.get("ok", false):
		_world_loaded = false
		_set_interaction_enabled(true)
		_set_status("Load world failed: %s" % result.get("error_message", "unknown error"))
		return

	_world_loaded = true
	_current_settlement_id = DEFAULT_SETTLEMENT_ID
	_last_turn_report = {}
	_refresh_read_models()
	_set_interaction_enabled(true)
	_set_status("Loaded world: %s" % path)


func _save_world(path: String) -> void:
	if not _world_loaded:
		_set_status("Save world failed: no world is loaded.")
		return
	if path.is_empty():
		_set_status("Save world failed: no path was provided.")
		return

	var result: Dictionary = _bridge.save_world({
		"path": path,
		"write_json_debug_export": true
	})
	if not result.get("ok", false):
		_set_status("Save world failed: %s" % result.get("error_message", "unknown error"))
		return

	_set_status("Saved world to %s" % path)


func _advance_month() -> void:
	if not _world_loaded:
		_set_status("Advance month failed: no world is loaded.")
		return

	_last_turn_report = _bridge.advance_month()
	_refresh_read_models()
	_set_status(
		"Advanced to Year %s Month %s." % [
			_last_turn_report.get("year", 0),
			_last_turn_report.get("month", 0)
		]
	)


func _refresh_read_models() -> void:
	if not _world_loaded:
		_summary_label.text = ""
		_detail_label.text = ""
		return

	var summary: Dictionary = _bridge.get_settlement_summary(_current_settlement_id)
	var detail: Dictionary = _bridge.get_settlement_detail(_current_settlement_id)
	var metrics: Dictionary = _bridge.get_world_metrics()

	_summary_label.text = _format_summary(summary, metrics)
	_detail_label.text = _format_detail(detail, _last_turn_report)


func _format_summary(summary: Dictionary, metrics: Dictionary) -> String:
	var center: Dictionary = summary.get("center", {})
	return "\n".join([
		"Settlement %s at (%s, %s)" % [
			summary.get("settlement_id", 0),
			center.get("x", 0),
			center.get("y", 0)
		],
		"Population: %s" % summary.get("population_whole", 0),
		"Stockpile: food=%s wood=%s stone=%s" % [
			summary.get("food", 0),
			summary.get("wood", 0),
			summary.get("stone", 0)
		],
		"Active zones=%s projects=%s shortage=%s" % [
			summary.get("active_zone_count", 0),
			summary.get("active_project_count", 0),
			summary.get("food_shortage_flag", false)
		],
		"World metrics: settlements=%s zones=%s plots=%s projects=%s roads=%s dirty_chunks=%s" % [
			metrics.get("settlement_count", 0),
			metrics.get("zone_count", 0),
			metrics.get("plot_count", 0),
			metrics.get("project_count", 0),
			metrics.get("road_cell_count", 0),
			metrics.get("dirty_chunk_count", 0)
		]
	])


func _format_detail(detail: Dictionary, turn_report: Dictionary) -> String:
	var role_fill: Dictionary = detail.get("role_fill", {})
	var labor_demand: Dictionary = detail.get("labor_demand", {})
	var lines := [
		"Labor fill: serfs=%s artisans=%s nobles=%s" % [
			role_fill.get("serfs", 0),
			role_fill.get("artisans", 0),
			role_fill.get("nobles", 0)
		],
		"Labor demand: protected=%s extra=%s idle=%s" % [
			labor_demand.get("protected_base", 0),
			labor_demand.get("extra_roles", 0),
			labor_demand.get("idle", 0)
		],
		"Farm plots: %s transport_capacity=%s development_pressure=%s" % [
			detail.get("farm_plots", []).size(),
			detail.get("transport_capacity", 0),
			detail.get("development_pressure_tenths", 0)
		]
	]

	if not turn_report.is_empty():
		var phase_names := PackedStringArray()
		for phase in turn_report.get("phase_timings", []):
			phase_names.append("%s=%sms" % [phase.get("phase_name", "phase"), phase.get("duration_ms", 0)])
		lines.append("Turn report: year=%s month=%s" % [
			turn_report.get("year", 0),
			turn_report.get("month", 0)
		])
		lines.append("Phase timings: %s" % ", ".join(phase_names))
		lines.append("Completed projects: %s" % Array(turn_report.get("completed_projects", [])).size())
		lines.append("Shortage settlements: %s" % Array(turn_report.get("shortage_settlements", [])).size())

	return "\n".join(lines)


func _set_status(message: String) -> void:
	_status_label.text = message
	print(message)


func _on_create_pressed() -> void:
	_create_world()


func _on_load_pressed() -> void:
	_load_world(_path_edit.text.strip_edges())


func _on_save_pressed() -> void:
	_save_world(_path_edit.text.strip_edges())


func _on_advance_pressed() -> void:
	_advance_month()
