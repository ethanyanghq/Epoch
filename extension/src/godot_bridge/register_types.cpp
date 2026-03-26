#include "godot_bridge/alpha_world_bridge.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

namespace godot_bridge {

void initialize_alpha_module(const godot::ModuleInitializationLevel level) {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }

  godot::ClassDB::register_class<AlphaWorldBridge>();
}

void uninitialize_alpha_module(const godot::ModuleInitializationLevel level) {
  if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
}

}  // namespace godot_bridge

extern "C" {

GDExtensionBool GDE_EXPORT alpha_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization* initialization) {
  godot::GDExtensionBinding::InitObject init_object(get_proc_address, library, initialization);
  init_object.register_initializer(godot_bridge::initialize_alpha_module);
  init_object.register_terminator(godot_bridge::uninitialize_alpha_module);
  init_object.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init_object.init();
}

}  // extern "C"
