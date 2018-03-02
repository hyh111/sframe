cmake_minimum_required(VERSION 2.8)

project(sframe)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")

set(LIB_INSTALL_DIR "/usr/local/sframe/lib")
set(HEADER_INSTALL_DIR "/usr/local/sframe/include")

set(ALL_DIRS
	net
	conf
	util
	serv
)

if(WIN32)
	set(ALL_DIRS ${ALL_DIRS} net/win)
elseif(UNIX)
	set(ALL_DIRS ${ALL_DIRS} net/linux)
else()
	message(FATAL_ERROR "Not surported os.")
endif()

foreach(DIR ${ALL_DIRS})
	file(GLOB CUR_SRCS ${DIR}/*.cpp)
	set(SRCS ${SRCS} ${CUR_SRCS})

	file(GLOB HEADERS ${DIR}/*.h)
	file(GLOB HEADERS_HPP ${DIR}/*.hpp)
	set(HEADERS ${HEADERS} ${HEADERS_HPP})
	install(FILES ${HEADERS} DESTINATION ${HEADER_INSTALL_DIR}/${DIR})
endforeach(DIR)

add_library(sframe ${SRCS})

install(TARGETS sframe ARCHIVE DESTINATION ${LIB_INSTALL_DIR})