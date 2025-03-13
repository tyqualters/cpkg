cmake_minimum_required(VERSION 3.20)
project(cpkg)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

add_executable(cpkg src/main.cpp src/ninja_syntax.cpp src/ninja_syntax.hpp src/ninja_generator.cpp src/ninja_generator.hpp src/utils.cpp src/utils.hpp)

if(MSVC)
    target_link_options(cpkg PRIVATE "/SUBSYSTEM:CONSOLE")
endif()

if(WIN32)
    # Need to tell the binaries to build in same DIR so Debug will work...
    # I really hate Windows
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

# sol2
target_include_directories(cpkg PRIVATE vendor/sol2-3.3.0/single/single/include)

# lua
add_subdirectory(vendor/lua-5.4.7)
target_include_directories(cpkg PRIVATE vendor/lua-5.4.7/src)
target_link_libraries(cpkg PRIVATE lua54)

# fmt
add_subdirectory(vendor/fmt-11.1.4)
target_link_libraries(cpkg PRIVATE fmt::fmt)

# cxxopts
add_subdirectory(vendor/cxxopts-3.2.0)
target_link_libraries(cpkg PRIVATE cxxopts::cxxopts)
target_include_directories(cpkg PRIVATE cxxopts-3.2.0/include)

# libpsl (I have spent 5 hours trying to get this to work...)
# Also had to comment out Lines 1295-1297 and add link_libraries(Libpsl) in CURL's CMakeLists.txt to get this to work.
# THIS IS THE KIND OF ISSUE I WANT TO PREVENT WITH THIS PROJECT!!!
if(WIN32)
    # Add a library target for Libpsl
    set(LIBPSL_INCLUDE_DIR vendor/libpsl-0.21.5/build/include)
    set(LIBPSL_STATIC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libpsl-0.21.5/build/src/libpsl.a)
    add_library(Libpsl STATIC IMPORTED)
    set_target_properties(Libpsl PROPERTIES IMPORTED_LOCATION ${LIBPSL_STATIC_LIB})

    message(STATUS "LIBPSL_INCLUDE_DIR = ${LIBPSL_INCLUDE_DIR}")
    message(STATUS "LIBPSL_STATIC_LIB  = ${LIBPSL_STATIC_LIB}")

    # Add include and Link directories (required for CURL to be happy)
    include_directories(${LIBPSL_INCLUDE_DIR})
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/libpsl-0.21.5/build/src)

    # Link the import library (psl-5.lib) with cpkg
    target_link_libraries(cpkg PUBLIC Libpsl)

    # This is required for CURL
    add_compile_definitions(PSL_STATIC=1)

    # Check the CWD for DLLs
    set_target_properties(cpkg PROPERTIES INSTALL_RPATH "${CMAKE_CURRENT_BINARY_DIR}")

    # Set env variable for PSL
    set(PSL_DATABASE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libpsl-0.21.5/list/public_suffix_list.dat)

    # How to build Libpsl (recommended):
    # Install Python3 and Pip3
    # pip3 install meson
    # Add install location (for User Install: "%AppData%\Roaming\Python\PythonXY\Scripts") to PATH Environment Variable
    # Download Ninja or if using CLion, locate the ninja.exe file
    # Add that directory to the PATH Environment Variable as well
    # Make sure you have VisualStudio 2022 Build Tools installed
    # Download source code from repository
    # Open the x64 Native Tools Command Prompt for VS 2022
    # Make a builddir directory
    # Run: meson builddir --buildtype=release -Ddefault_both_libraries=static -Ddefault_library=static --backend=ninja
    # Run: cd builddir
    # Run: ninja
endif()

# openssl
if(WIN32)
    set(OPENSSL_ROOT_DIR vendor/openssl-3.4.1)
    include(vendor/openssl-3.4.1/OpenSSLConfig.cmake)
    find_package(OpenSSL REQUIRED)
    target_include_directories(cpkg PUBLIC vendor/openssl-3.4.1/include)
    link_libraries(OpenSSL::SSL OpenSSL::Crypto)

    # Enable SSL for libcurl
    set(CURL_SSL_BACKEND OpenSSL)
    set(CURL_USE_SSL ON)

    # Add definitions for compiling with SSL
    add_compile_definitions(CURL_SSL_BACKEND=OpenSSL)
    add_compile_definitions(CURL_USE_SSL=ON)

    # Ensure libcurl is built with OpenSSL
    set(USE_OPENSSL ON)
endif()

# libcurl
add_subdirectory(vendor/curl-8.12.1)
target_link_libraries(cpkg PRIVATE CURL::libcurl)
target_include_directories(cpkg PRIVATE curl-8.12.1/include)