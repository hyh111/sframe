cmake_minimum_required(VERSION 2.8)

project(server)

if(WIN32)
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -g -Wall")
else()
	message(FATAL_ERROR "Not surported os.")
endif()

include_directories(../../sframe)

set(ALL_DIRS
	.
	config
	gateservice
	httpservice
	ssproto
	workservice
)

foreach(DIR ${ALL_DIRS})
	file(GLOB CUR_SRCS ${DIR}/*.cpp)
	set(SRCS ${SRCS} ${CUR_SRCS})
	file(GLOB HEADERS ${DIR}/*.h ${DIR}/*.hpp)
	set(ALL_HEADERS ${ALL_HEADERS} ${HEADERS})

	string(COMPARE EQUAL ${DIR} "." IS_CUR_DIR)

	if (IS_CUR_DIR)
		set(GROUP "\\")
	else ()
		string(REPLACE "/" "\\" GROUP ${DIR})
	endif ()
	
	source_group(${GROUP} FILES ${HEADERS})
	source_group(${GROUP} FILES ${CUR_SRCS})
endforeach(DIR)

add_executable(server ${SRCS} ${ALL_HEADERS})
target_link_libraries(server sframe)

if (WIN32)
	target_link_libraries(server ws2_32.lib)
endif()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "example")