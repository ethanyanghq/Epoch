cmake_minimum_required(VERSION 3.24)

project(alpha_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(alpha_sim STATIC
  src/api/world_api.cpp
  src/core/simulation.cpp
  src/map/chunk_visuals.cpp
  src/map/map_grid.cpp
  src/map/overlay_chunks.cpp
  src/projects/project_types.cpp
  src/save/save_io.cpp
  src/settlements/settlement_types.cpp
  src/world/world_state.cpp
  src/zones/zone_types.cpp
)

target_compile_features(alpha_sim PUBLIC cxx_std_20)

target_include_directories(alpha_sim
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_TESTING)
  add_executable(alpha_sim_smoke_test
    tests/unit/world_api_smoke_test.cpp
  )

  add_executable(alpha_sim_save_load_test
    tests/unit/save_load_roundtrip_test.cpp
  )

  add_executable(alpha_sim_zoning_test
    tests/unit/zoning_commands_test.cpp
  )

  add_executable(alpha_sim_project_commands_test
    tests/unit/project_commands_test.cpp
  )

  target_link_libraries(alpha_sim_smoke_test
    PRIVATE
      alpha_sim
  )

  target_link_libraries(alpha_sim_save_load_test
    PRIVATE
      alpha_sim
  )

  target_link_libraries(alpha_sim_zoning_test
    PRIVATE
      alpha_sim
  )

  target_link_libraries(alpha_sim_project_commands_test
    PRIVATE
      alpha_sim
  )

  add_test(NAME alpha_sim_smoke_test COMMAND alpha_sim_smoke_test)
  add_test(NAME alpha_sim_save_load_test COMMAND alpha_sim_save_load_test)
  add_test(NAME alpha_sim_zoning_test COMMAND alpha_sim_zoning_test)
  add_test(NAME alpha_sim_project_commands_test COMMAND alpha_sim_project_commands_test)
endif()
