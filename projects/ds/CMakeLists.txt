# =============================================================================
# ds — header-only 資料結構練習場 (對應 rust 的 src/ds)。
#
#   - ds_lib 是 INTERFACE library，public headers 放在 include/ds/...，
#     消費端寫 #include <ds/tree/tree.hpp>，include 路徑停在 include/。
#   - 每個結構旁邊在 tests/ 放一支 *_test.cpp，全部 GLOB 進單一 ds_tests
#     執行檔（doctest），掛上 ctest → `ctest` 就能一鍵全跑 (≈ cargo test)。
#   - doctest 由 nix flake 提供 (find_package)，與其他相依一致。
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(ds)

add_library(ds_lib INTERFACE)
target_include_directories(ds_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ds_lib INTERFACE cxx_std_20)

find_package(doctest CONFIG REQUIRED)

# tests/ 下的 *_test.cpp + 唯一的 doctest main TU → 單一 ds_tests。
file(GLOB_RECURSE DS_TEST_FILES CONFIGURE_DEPENDS tests/*_test.cpp)

if(DS_TEST_FILES)
  add_executable(ds_tests tests/ds_test_main.cpp ${DS_TEST_FILES})
  target_link_libraries(ds_tests PRIVATE ds_lib doctest::doctest)
  add_test(NAME ds_tests COMMAND ds_tests)
endif()
