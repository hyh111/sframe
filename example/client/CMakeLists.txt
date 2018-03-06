cmake_minimum_required(VERSION 2.8)

project(client)

if(WIN32)
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g -Wall")
else()
	message(FATAL_ERROR "Not surported os.")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../bin/client)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(../../sframe sframe-obj)

include_directories(../../sframe)

aux_source_directory(. SRCS)
add_executable(client ${SRCS})
target_link_libraries(client sframe)

if (WIN32)
	target_link_libraries(client ws2_32.lib)
endif()