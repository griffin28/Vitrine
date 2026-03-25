set(TEXTURE_SOURCES
    viking_room.png
)

add_custom_target(texture_cpy DEPENDS ${TEXTURE_SOURCES})

add_custom_command(TARGET texture_cpy POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/viking_room.png
        $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/textures
	COMMENT "Copying textures to output directory")