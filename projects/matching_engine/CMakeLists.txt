# =============================================================================
# matching_engine — header-only async matching engine library + tests.
#
# This file should:
#   1. Define `matching_engine_lib` as an INTERFACE library (no .cpp to compile).
#   2. Expose its public headers under include/ so consumers can write:
#          #include <matching_engine/matching/async_engine.hpp>
#   3. Propagate dependencies that headers need (fmt) and required C++ standard
#      (C++20 for coroutines) — anyone who links us must inherit these.
#   4. Build the test executable in tests/coro_matching_test.cpp, which links
#      against matching_engine_lib.
#
# Reminders:
#   - For an INTERFACE library, every target_*() call MUST use INTERFACE
#     keyword (PUBLIC/PRIVATE don't make sense — there is no "self" to compile).
#   - fmt::fmt is already provided by find_package() in the top-level
#     CMakeLists.txt — just reference it here.
#   - coro_matching_test.cpp uses C++20 coroutines, so it inherits cxx_std_20
#     automatically once it links matching_engine_lib (that's the magic of
#     target_compile_features INTERFACE).
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(matching_engine)

# define the INTERFACE library
add_library(matching_engine_lib INTERFACE)

find_package(fmt CONFIG REQUIRED)

# TODO: tell consumers where the public headers live
# Hint: ${CMAKE_CURRENT_SOURCE_DIR}/include is the right path. Consumers will
# write `#include <matching_engine/...>`, so the include path stops at include/.
# target_include_directories(matching_engine_lib ...)
target_include_directories(matching_engine_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# TODO: propagate fmt as a usage requirement
# target_link_libraries(matching_engine_lib ...)
target_link_libraries(matching_engine_lib INTERFACE fmt::fmt)

# TODO: require C++20 from any consumer (needed for coroutines)
# target_compile_features(matching_engine_lib ...)
target_compile_features(matching_engine_lib INTERFACE cxx_std_20)

# TODO: build the test executable from tests/coro_matching_test.cpp
# add_executable(coro_matching_test ...)
# target_link_libraries(coro_matching_test ...)
add_executable(coro_matching_test tests/coro_matching_test.cpp)
target_link_libraries(coro_matching_test PRIVATE matching_engine_lib)
