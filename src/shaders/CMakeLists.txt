set(SHADER_SOURCES
	shader.vert
    shader.frag
    shader.comp
)

add_shaders_target(slang_shaders SOURCES ${SHADER_SOURCES})

add_custom_command(TARGET slang_shaders POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vert.spv
        ${CMAKE_CURRENT_SOURCE_DIR}/frag.spv
        ${CMAKE_CURRENT_SOURCE_DIR}/comp.spv
        $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/shaders
	COMMENT "Copying compiled shaders to output directory")
