cmake_minimum_required(VERSION 2.8)

project(server)

if(WIN32)
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -g -Wall")
else()
	message(FATAL_ERROR "Not surported os.")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../bin/server)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(../../sframe sframe-obj)

include_directories(../../sframe)

add_subdirectory(config)
add_subdirectory(gateservice)
add_subdirectory(httpservice)
add_subdirectory(ssproto)
add_subdirectory(workservice)

aux_source_directory(. SRCS)
add_executable(server ${SRCS})
target_link_libraries(server config gateservice httpservice ssproto workservice sframe)
add_dependencies(server sframe)

if (WIN32)
	target_link_libraries(server ws2_32.lib)
endif()