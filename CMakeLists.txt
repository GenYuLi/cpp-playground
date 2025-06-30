# CMake 最低版本要求
cmake_minimum_required(VERSION 3.20)

# 專案名稱與版本
project(
  CppPlayground
  VERSION 1.0
  LANGUAGES CXX)

# C++ 標準切換開關 (這部分完全不變) option to enable C++ standard selection

# set cache 變數 CXX_VERSION，讓使用者可以選擇 C++ 標準 set(cxx_version "20" cache string
# "select c++ standard: 14, 17, 20, or 23")
set(CXX_VERSION
    "20"
    CACHE STRING "Select C++ standard: 14, 17, 20, or 23")
set_property(CACHE CXX_VERSION PROPERTY STRINGS "14" "17" "20" "23")
set(CMAKE_CXX_STANDARD ${CXX_VERSION})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Using C++ Standard: ${CMAKE_CXX_STANDARD}")

# 自動產生 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- 函式庫相依管理 ---
# 引入 FetchContent 模組 (用它來下載 fmt 和 json)
include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3)
set(JSON_BuildTests
    OFF
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json)

find_package(Boost 1.74.0 REQUIRED COMPONENTS system)

# --- 建立函式庫 ---

add_library(
  pg_lib # target name
  src/asio_test/asio_test.cpp # implementation file
  src/asio_test/asio_test.hpp # header file
  src/language_practice/lp.cpp # implementation file
  src/language_practice/lp.hpp # header file
  src/language_practice/some_ds.hpp # header file and only
)

target_include_directories(
  pg_lib PUBLIC # PUBLIC=自己也要、link 進來的人也要
                ${CMAKE_CURRENT_SOURCE_DIR}/src/language_practice)

target_link_libraries(
  pg_lib
  PUBLIC # PUBLIC=自己也要、link 進來的人也要
         fmt::fmt # (★★ 這裡是主要修改處 ★★) - 使用 fmt 函式庫
         nlohmann_json::nlohmann_json # (★★ 這裡是主要修改處 ★★) - 使用 nlohmann_json 函式庫
         Boost::system # (★★ 這裡是主要修改處 ★★) - 連結 Boost.System，Asio 需要它
)

# --- 建立執行檔與連結函式庫 ---
file(GLOB BIN_FILES src/bin/*.cpp)

foreach(src ${BIN_FILES})
  get_filename_component(bin_name ${src} NAME_WE)
  add_executable(${bin_name} ${src})
  list(APPEND BINARYS ${bin_name})
endforeach()

foreach(bin IN LISTS BINARYS)
  target_include_directories(
    ${bin} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/language_practice
                   ${CMAKE_CURRENT_SOURCE_DIR}/src/asio_test)
  # 設定每個執行檔的 C++ 標準
  target_compile_features(${bin} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
  # 將函式庫連結到我們的執行檔
  target_link_libraries(${bin} PRIVATE pg_lib)
endforeach()

# Boost.Asio 大多是 header-only，但它依賴於 Boost.System，所以我們需要連結它。 find_package 會自動處理
# Boost 的 include 路徑，所以你不需要手動設定。
