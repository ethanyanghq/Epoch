cmake_minimum_required(VERSION 3.24)

project(alpha_godot_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(ALPHA_GODOT_PROJECT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../game"
    CACHE PATH
    "Path to the Godot project that should receive the built extension library.")

set(GODOT_CPP_DIR
    ""
    CACHE PATH
    "Path to a checked out godot-cpp repository that matches the installed Godot 4.x version.")

set(ALPHA_GODOT_BUILD_KIND
    "debug"
    CACHE STRING
    "Output naming for the built extension library. Expected values: debug or release.")
set_property(CACHE ALPHA_GODOT_BUILD_KIND PROPERTY STRINGS debug release)

if(NOT GODOT_CPP_DIR)
  foreach(candidate_dir
          "${CMAKE_CURRENT_SOURCE_DIR}/third_party/godot-cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/godot-cpp")
    if(EXISTS "${candidate_dir}/CMakeLists.txt")
      set(GODOT_CPP_DIR "${candidate_dir}")
      break()
    endif()
  endforeach()
endif()

if(NOT GODOT_CPP_DIR OR NOT EXISTS "${GODOT_CPP_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
          "Missing godot-cpp checkout. Set GODOT_CPP_DIR to a local godot-cpp repository, "
          "or place it at extension/third_party/godot-cpp or third_party/godot-cpp.")
endif()

if(ALPHA_GODOT_BUILD_KIND STREQUAL "debug")
  set(GODOTCPP_TARGET template_debug CACHE STRING "" FORCE)
  set(alpha_godotcpp_target godot-cpp::template_debug)
  set(alpha_output_name "libalpha_godot_bridge.macos.debug")
elseif(ALPHA_GODOT_BUILD_KIND STREQUAL "release")
  set(GODOTCPP_TARGET template_release CACHE STRING "" FORCE)
  set(alpha_godotcpp_target godot-cpp::template_release)
  set(alpha_output_name "libalpha_godot_bridge.macos.release")
else()
  message(FATAL_ERROR "Unsupported ALPHA_GODOT_BUILD_KIND='${ALPHA_GODOT_BUILD_KIND}'. Use debug or release.")
endif()

add_subdirectory("${GODOT_CPP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp" SYSTEM)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../sim" "${CMAKE_CURRENT_BINARY_DIR}/alpha_sim")

add_library(alpha_godot_bridge SHARED
  src/godot_bridge/alpha_world_bridge.cpp
  src/godot_bridge/register_types.cpp
)

target_compile_features(alpha_godot_bridge PUBLIC cxx_std_20)

target_include_directories(alpha_godot_bridge
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(alpha_godot_bridge
  PRIVATE
    alpha_sim
    ${alpha_godotcpp_target}
)

set_target_properties(alpha_godot_bridge PROPERTIES
  PREFIX ""
  OUTPUT_NAME "${alpha_output_name}"
  LIBRARY_OUTPUT_DIRECTORY "${ALPHA_GODOT_PROJECT_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${ALPHA_GODOT_PROJECT_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
