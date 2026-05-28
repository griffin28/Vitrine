set (SRCS
    main.cpp
    resources/resources.qrc
    resources/style/qdarkstyle/dark/darkstyle.qrc
    resources/style/qdarkstyle/light/lightstyle.qrc
)

add_subdirectory(renderer)
add_subdirectory(ui)
add_subdirectory(core)
add_subdirectory(textures)
add_subdirectory(models)

add_executable(${CMAKE_PROJECT_NAME} ${SRCS})

add_dependencies(${CMAKE_PROJECT_NAME} texture_cpy)
add_dependencies(${CMAKE_PROJECT_NAME} model_cpy)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        anari::anari
        renderer
        ui
        core
        compiler_props
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                   COMMAND md5sum
                   ARGS -b "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" > "${CMAKE_PROJECT_NAME}.md5")
