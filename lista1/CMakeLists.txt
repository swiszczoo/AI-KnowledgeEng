cmake_minimum_required(VERSION 3.20)
project(AI_lista1)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE ZAD1_SOURCES "zad1/*.*")
add_executable(zad1 ${ZAD1_SOURCES})

add_custom_command(TARGET zad1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/data/connection_graph.csv ${CMAKE_BINARY_DIR}
    COMMAND_EXPAND_LISTS
)
