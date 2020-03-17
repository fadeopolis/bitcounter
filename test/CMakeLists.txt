
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin")

function(add_basic_test NAME)
  set(FILE "${NAME}.cpp")

  add_executable("${NAME}" "${FILE}")
  target_link_libraries("${NAME}" PRIVATE bc)

  add_test(basic "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}")
endfunction(add_basic_test)

add_basic_test(all_zeroes)

