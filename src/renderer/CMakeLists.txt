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
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${stb_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
)

target_link_libraries(renderer
    PUBLIC
        anari::anari
        Qt6::Core
        Qt6::Gui)
