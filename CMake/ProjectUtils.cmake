#======== Interface libraries
# Compiler Settings
#
# Add -save-temps=obj to force output of each stage to be stored in a file instead
# of memory (GCC and Clang)
#
# Add -H to see which paths are being used to include a requested header
add_library(compiler_props INTERFACE)
target_compile_options(compiler_props
    INTERFACE
        $<$<CONFIG:DEBUG>:-Wall -Wextra -Wpedantic>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-gstatement-frontiers -ginline-points> # extended debug info for inline funcs
)

#======== Macros
macro(create_find_package pkg required quiet components)
    if(("${required}" STREQUAL "1") AND ("${quiet}" STREQUAL "1"))
        find_package(${pkg} QUIET REQUIRED COMPONENTS ${components})
    elseif(${required})
        find_package(${pkg} REQUIRED COMPONENTS ${components})
    elseif(${quiet})
        find_package(${pkg} QUIET COMPONENTS ${components})
    endif()
endmacro(create_find_package)

#======== Functions
function (add_shaders_target TARGET)
    find_program (GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
    cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
    set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR})
    add_custom_command (
            OUTPUT ${SHADERS_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
    add_custom_command (
        OUTPUT ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv
        COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.0 ${SHADER_SOURCES}
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
        COMMENT "Compiling Shaders"
        VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv)
endfunction ()

function (add_slang_shader_target TARGET)
    find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
    cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
    set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR})
    set (ENTRY_POINTS -entry vertMain -entry fragMain)
    add_custom_command (
            OUTPUT ${SHADERS_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
    add_custom_command (
            OUTPUT  ${SHADERS_DIR}/slang.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
            WORKING_DIRECTORY ${SHADERS_DIR}
            DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

function(AddValgrind target)
    find_program(VALGRIND_PATH valgrind)
    if(NOT VALGRIND_PATH)
        add_custom_target(valgrind COMMAND false
            COMMENT "valgrind not found")
        return()
    endif()
    add_custom_target(valgrind 
        COMMAND ${VALGRIND_PATH} --leak-check=full $<TARGET_FILE:${target}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()
