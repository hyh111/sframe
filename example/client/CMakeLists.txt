cmake_minimum_required(VERSION 2.8)

project(client)

if(WIN32)
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g -Wall")
else()
	message(FATAL_ERROR "Not surported os.")
endif()

include_directories(../../sframe)

aux_source_directory(. SRCS)
add_executable(client ${SRCS})
target_link_libraries(client sframe)

if (WIN32)
	target_link_libraries(client ws2_32.lib)
endif()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "example")