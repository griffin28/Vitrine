
cmake_minimum_required(VERSION 3.28)
project(MyProject VERSION 1.0 LANGUAGES CXX)

# set(CMAKE_DIABLE_IN_SOURCE_BUILD ON) - creates an error, however,
# files can still be created in the source directory.
# This is an older alternative:
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
# Resource Compiler
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

include(ProjectUtils)

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support" OFF)  

if(ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

find_package(ProjectQt6 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(tinygltf REQUIRED)
find_package(KTX REQUIRED)

# Options
option(BUILD_TESTS "Build unit tests" OFF)

# Setup Vulkan C++ module if enabled
if(ENABLE_CPP20_MODULE)
    message(STATUS "C++ 20 module support enabled")
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
        PUBLIC
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    )
    target_include_directories(VulkanCppModule
        PUBLIC
            ${Vulkan_INCLUDE_DIR}
    )
    target_link_libraries(VulkanCppModule
        PUBLIC
            Vulkan::Vulkan
    )

    set_target_properties(VulkanCppModule 
        PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
    )

    target_sources(VulkanCppModule 
        PUBLIC 
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${Vulkan_INCLUDE_DIR}"
            FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    target_sources(VulkanCppModule
        PRIVATE
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else()
    message(STATUS "C++ 20 module support disabled")
    # Create a dummy interface library
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
        INTERFACE
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    )
endif()

find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

# Source directory
add_subdirectory(src bin)

# Tests 
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()