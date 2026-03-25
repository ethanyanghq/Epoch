cmake_minimum_required(VERSION 3.24)

project(alpha_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(alpha_sim INTERFACE)

target_include_directories(alpha_sim
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Baseline target only.
# Source files are added in Milestone 1 as the simulation core comes online.
