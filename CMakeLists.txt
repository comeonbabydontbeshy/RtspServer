CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(RtspServer)

INCLUDE(CMakeLists.CommonFlag.cmake)

SET(INCLUDE_DIRS
	${Boost_INCLUDE_DIRS}
	${CMAKE_HOME_DIRECTORY}/src
	)

SET(SRC_FILES
	src/common/BaseService.cc
	src/common/runner.cc
	src/main.cc
	)

INCLUDE_DIRECTORIES(${INCLUDE_DIRS})

LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

ADD_EXECUTABLE(rtspSrv ${SRC_FILES})

TARGET_LINK_LIBRARIES(rtspSrv 
	-lboost_system
	-lboost_thread)
	