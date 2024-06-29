cmake_minimum_required(VERSION 3.5)

set(LIB_NAME asl-ftp)

set(CMAKE_CXX_STANDARD 11)

project(${LIB_NAME})

if(NOT TARGET asls)
	find_package(ASL REQUIRED)
endif()

add_subdirectory(src)
