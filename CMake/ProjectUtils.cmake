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
# function(add_shader_target TARGET)

# endfunction()

# function(add_shaders_target target shader_files)
#     add_custom_target(${target} ALL)
#     foreach(shader_file IN LISTS shader_files)
#         get_filename_component(shader_name ${shader_file} NAME)
#         set(compiled_shader "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.spv")
#         add_custom_command(
#             OUTPUT ${compiled_shader}
#             COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
#             COMMAND glslc ${shader_file} -o ${compiled_shader}
#             DEPENDS ${shader_file}
#             COMMENT "Compiling shader: ${shader_name}"
#             VERBATIM
#         )
#         add_custom_target(${target}_${shader_name} DEPENDS ${compiled_shader})
#         add_dependencies(${target} ${target}_${shader_name})
#     endforeach()
# endfunction()

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
