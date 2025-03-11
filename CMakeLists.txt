cmake_minimum_required(VERSION 3.31)
project(cpkg)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

add_executable(cpkg main.cpp)

# sol2
target_include_directories(cpkg PRIVATE sol2-3.3.0/single/single/include)

# lua
add_subdirectory(lua-5.4.7)
target_include_directories(cpkg PRIVATE vendors/lua-5.4.7/src)
target_link_libraries(cpkg PRIVATE lua54)

# fmt
add_subdirectory(fmt-11.1.4)
target_link_libraries(cpkg PRIVATE fmt::fmt)

# cxxopts
add_subdirectory(cxxopts-3.2.0)
target_link_libraries(cpkg PRIVATE cxxopts::cxxopts)
target_include_directories(cpkg PRIVATE cxxopts-3.2.0/include)

# libcurl
add_subdirectory(curl-8.12.1)
target_link_libraries(cpkg PRIVATE CURL::libcurl)
target_include_directories(cpkg PRIVATE curl-8.12.1/include)