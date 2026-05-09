cmake_minimum_required(VERSION 3.24)

project(
  CppPlayground
  VERSION 1.0
  LANGUAGES CXX
)

# --- C++ standard (overridable via -DCXX_VERSION=23) ---
set(CXX_VERSION "20" CACHE STRING "Select C++ standard: 14, 17, 20, or 23")
set_property(CACHE CXX_VERSION PROPERTY STRINGS "14" "17" "20" "23")
set(CMAKE_CXX_STANDARD ${CXX_VERSION})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Using C++ Standard: ${CMAKE_CXX_STANDARD}")

# --- compile_commands.json for clangd / IDEs ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- External dependencies (provided by nix flake or system) ---
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost 1.74.0 REQUIRED CONFIG COMPONENTS system)

# --- Subprojects ---
# Each subdirectory defines its own targets and exposes them by name.
# Order matters only when one subdir consumes targets from another:
#   bin/ links pg_lib and matching_engine_lib, so libraries come first.
add_subdirectory(projects/matching_engine)
add_subdirectory(pg)
add_subdirectory(bin)
