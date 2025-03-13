cmake_minimum_required(VERSION 3.31)
project(lua54)

set(CMAKE_C_STANDARD 23)

file(GLOB SOURCES src/*c src/*.h)

add_library(lua54 STATIC ${SOURCES})