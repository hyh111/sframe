cmake_minimum_required(VERSION 2.8)

project(sframe)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(LIB_INSTALL_DIR "/usr/local/sframe/lib")
set(HEADER_INSTALL_DIR "/usr/local/sframe/include")

set(ALL_DIRS
	.
	net
	conf
	util
	serv
)

if(WIN32)
	add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	set(ALL_DIRS ${ALL_DIRS} net/win)
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")
	set(ALL_DIRS ${ALL_DIRS} net/linux)
else()
	message(FATAL_ERROR "Not surported os.")
endif()

foreach(DIR ${ALL_DIRS})
	file(GLOB CUR_SRCS ${DIR}/*.cpp)
	set(SRCS ${SRCS} ${CUR_SRCS})
	file(GLOB HEADERS ${DIR}/*.h ${DIR}/*.hpp)
	set(ALL_HEADERS ${ALL_HEADERS} ${HEADERS})
	install(FILES ${HEADERS} DESTINATION ${HEADER_INSTALL_DIR}/${DIR})

	string(COMPARE EQUAL ${DIR} "." IS_CUR_DIR)

	if (IS_CUR_DIR)
		set(GROUP "\\")
	else ()
		string(REPLACE "/" "\\" GROUP ${DIR})
	endif ()
	
	source_group(${GROUP} FILES ${HEADERS})
	source_group(${GROUP} FILES ${CUR_SRCS})

endforeach(DIR)

add_library(sframe ${SRCS} ${ALL_HEADERS})

install(TARGETS sframe ARCHIVE DESTINATION ${LIB_INSTALL_DIR})