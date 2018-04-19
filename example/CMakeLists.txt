cmake_minimum_required(VERSION 2.8)

project(example)

message(STATUS "CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

if(UNIX)
	string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Release" IS_RELEASE)
	if (IS_RELEASE)
		set(OUT_DIR "/Release")
	else()
		set(OUT_DIR "/Debug")
	endif()
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin${OUT_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib${OUT_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(../sframe sframe-obj)
add_subdirectory(client)
add_subdirectory(server)
