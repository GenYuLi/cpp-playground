# pg_lib — playground library for language practice + asio experiments.
# Built as SHARED so consumers in bin/ link against libpg_lib.so.

file(GLOB_RECURSE PG_SOURCES CONFIGURE_DEPENDS
  src/asio_test/*.cpp
  src/language_practice/*.cpp
)

file(GLOB_RECURSE PG_PUBLIC_HEADERS CONFIGURE_DEPENDS
  include/pg/*.hpp
  include/pg/*.h
)

add_library(pg_lib SHARED ${PG_SOURCES})

target_sources(pg_lib PUBLIC ${PG_PUBLIC_HEADERS})

target_include_directories(pg_lib
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pg_lib
  PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    Boost::system
)
