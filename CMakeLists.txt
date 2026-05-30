
cmake_minimum_required(VERSION 3.28)
project(Vitrine VERSION 1.0 LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

include(ProjectUtils)

find_package(ProjectQt6 REQUIRED)
find_package(glm REQUIRED)
find_package(anari 0.16.0 REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(tinygltf REQUIRED)
find_package(KTX REQUIRED)
find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

option(BUILD_APP_TESTS "Build unit tests" OFF)

find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

add_subdirectory(src bin)

if(BUILD_APP_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
