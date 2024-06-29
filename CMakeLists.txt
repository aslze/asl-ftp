cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

project(asl-ftp)

if(NOT TARGET asls)
	find_package(ASL REQUIRED)
endif()

add_subdirectory(src)
