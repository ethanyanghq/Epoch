cmake_minimum_required(VERSION 3.24)

project(alpha_godot_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(alpha_godot_bridge INTERFACE)

target_include_directories(alpha_godot_bridge
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Baseline target only.
# Godot headers, bindings, and bridge sources are added in Milestone 1.
