class_name WorldBridge
extends RefCounted

const BRIDGE_EXTENSION := preload("res://addons/alpha/alpha.gdextension")

var _native_bridge: Object = null


func initialize() -> Dictionary:
	if BRIDGE_EXTENSION == null:
		return {
			"ok": false,
			"error_message": "Failed to load res://addons/alpha/alpha.gdextension."
		}

	if not ClassDB.class_exists("AlphaWorldBridge"):
		return {
			"ok": false,
			"error_message": "AlphaWorldBridge is not registered. Build the GDExtension and ensure res://addons/alpha/alpha.gdextension can load."
		}

	if not ClassDB.can_instantiate("AlphaWorldBridge"):
		return {
			"ok": false,
			"error_message": "AlphaWorldBridge exists but cannot be instantiated."
		}

	_native_bridge = ClassDB.instantiate("AlphaWorldBridge")
	if _native_bridge == null:
		return {
			"ok": false,
			"error_message": "Failed to instantiate AlphaWorldBridge."
		}

	return {"ok": true, "error_message": ""}


func is_initialized() -> bool:
	return _native_bridge != null


func create_world(params: Dictionary) -> Dictionary:
	return _native_bridge.call("create_world", params)


func load_world(params: Dictionary) -> Dictionary:
	return _native_bridge.call("load_world", params)


func save_world(params: Dictionary) -> Dictionary:
	return _native_bridge.call("save_world", params)


func apply_commands(batch: Dictionary) -> Dictionary:
	return _native_bridge.call("apply_commands", batch)


func advance_month() -> Dictionary:
	return _native_bridge.call("advance_month")


func get_chunk_visual(query: Dictionary) -> Dictionary:
	return _native_bridge.call("get_chunk_visual", query)


func get_overlay_chunk(query: Dictionary) -> Dictionary:
	return _native_bridge.call("get_overlay_chunk", query)


func get_settlement_summary(settlement_id: int) -> Dictionary:
	return _native_bridge.call("get_settlement_summary", settlement_id)


func get_settlement_detail(settlement_id: int) -> Dictionary:
	return _native_bridge.call("get_settlement_detail", settlement_id)


func get_projects(query: Dictionary) -> Dictionary:
	return _native_bridge.call("get_projects", query)


func get_world_metrics() -> Dictionary:
	return _native_bridge.call("get_world_metrics")
