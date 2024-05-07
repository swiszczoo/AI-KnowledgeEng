cmake_minimum_required(VERSION 3.20)
project(AI_lista2)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE LISTA2_SOURCES "src/*.*")
add_executable(lista2 ${LISTA2_SOURCES})

