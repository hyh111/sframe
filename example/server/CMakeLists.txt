cmake_minimum_required(VERSION 2.8)

project(server)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -pthread")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../bin/server)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(../../sframe obj)

include_directories(../../sframe)
#link_directories(/home/programmer/sf/lib)

add_subdirectory(config)
add_subdirectory(gateservice)
add_subdirectory(httpservice)
add_subdirectory(ssproto)
add_subdirectory(workservice)

aux_source_directory(. SRCS)
add_executable(server ${SRCS})
target_link_libraries(server config gateservice httpservice ssproto workservice sframe)
add_dependencies(server sframe)