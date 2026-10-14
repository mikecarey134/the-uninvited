cmake_minimum_required(VERSION 3.16)
project(TheUninvited CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED GAMECORE_DIR)
    set(GAMECORE_DIR "${CMAKE_SOURCE_DIR}/../GameCore")
endif()

find_package(PkgConfig REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GAMECORE_DIR}/include
)

link_directories(${GAMECORE_DIR}/build)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(game ${SOURCES})
target_link_libraries(game gamecore)
