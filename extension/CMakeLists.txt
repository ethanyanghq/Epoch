cmake_minimum_required(VERSION 3.24)

project(alpha_godot_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../sim ${CMAKE_CURRENT_BINARY_DIR}/alpha_sim)

add_library(alpha_godot_bridge STATIC
  src/godot_bridge/alpha_world_bridge.cpp
  src/godot_bridge/register_types.cpp
)

target_compile_features(alpha_godot_bridge PUBLIC cxx_std_20)

target_include_directories(alpha_godot_bridge
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(alpha_godot_bridge
  PUBLIC
    alpha_sim
)
