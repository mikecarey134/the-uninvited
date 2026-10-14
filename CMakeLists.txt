cmake_minimum_required(VERSION 3.16)
project(TheUninvited CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GAMECORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/gamecorev0.1.1/gamecorev0.1.1")
add_subdirectory("${GAMECORE_DIR}" engine EXCLUDE_FROM_ALL)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(game ${SOURCES})
target_include_directories(game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(game PRIVATE gamecore_engine game_net_session)
