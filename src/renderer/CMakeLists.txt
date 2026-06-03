set(RENDERER_SRCS
    AnariRenderer.h
    AnariRenderer.cpp
)

add_library(renderer OBJECT ${RENDERER_SRCS})

target_compile_options(renderer
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../core>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../loader
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${stb_INCLUDE_DIRS}
)

# camera is PUBLIC: AnariRenderer.h includes Camera.h, so the camera include
# dir (and glm, transitively) must reach renderer's consumers (ui, loader).
target_link_libraries(renderer
    PUBLIC
        anari::anari
        Qt6::Core
        Qt6::Gui
        camera)
