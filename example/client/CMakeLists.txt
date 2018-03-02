cmake_minimum_required(VERSION 2.8)

project(client)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -pthread")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../bin/client)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(../../sframe sframe-obj)

include_directories(../../sframe)
#link_directories(/home/programmer/sf/lib)

aux_source_directory(. SRCS)
add_executable(client ${SRCS})
target_link_libraries(client sframe)
