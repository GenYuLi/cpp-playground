# Each .cpp under language/ and asio/ becomes its own executable.
# Adding a new playground bin = drop a .cpp here, re-run cmake -B build.

file(GLOB BIN_SOURCES CONFIGURE_DEPENDS
  language/*.cpp
  asio/*.cpp
)

foreach(src ${BIN_SOURCES})
  get_filename_component(bin_name ${src} NAME_WE)
  add_executable(${bin_name} ${src})
  target_link_libraries(${bin_name} PRIVATE pg_lib)
endforeach()
