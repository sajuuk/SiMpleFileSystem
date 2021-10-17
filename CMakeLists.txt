cmake_minimum_required(VERSION 3.0.0)
project(simplefilesystem VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(LINUX_HEADERS F:/VitrualMachineSharing/linux)
add_definitions(-D__KERNEL__)
add_library(simplefilesystem mkfs.c common.h)
add_executable(mkfs mkfs.c common.h)

target_include_directories(mkfs PUBLIC F:/VitrualMachineSharing/include)
