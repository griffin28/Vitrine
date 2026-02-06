set (SRCS
    main.cpp
    HelloTriangle.cpp
    AppUtils.h
    resources/resources.qrc
    resources/style/qdarkstyle/dark/darkstyle.qrc
    resources/style/qdarkstyle/light/lightstyle.qrc
)

# add_subdirectory(<sourcedir> [<binarydir>])
add_subdirectory(renderer)
add_subdirectory(ui)

add_executable(${CMAKE_PROJECT_NAME} ${SRCS})

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Vulkan::cppm
        renderer
        ui
        compiler_props
)

# Generate MD5 hash of the executable
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                   COMMAND md5sum
                   ARGS -b "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" > "${CMAKE_PROJECT_NAME}.md5")