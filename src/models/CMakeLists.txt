set(MODEL_SOURCES
    viking_room.obj
)

add_custom_target(model_cpy DEPENDS ${MODEL_SOURCES})

add_custom_command(TARGET model_cpy POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/viking_room.obj
        $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/models
	COMMENT "Copying models to output directory")