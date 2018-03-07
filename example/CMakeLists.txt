cmake_minimum_required(VERSION 2.8)

project(example)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(../sframe sframe-obj)
add_subdirectory(client)
add_subdirectory(server)
