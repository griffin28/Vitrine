set(LOADER_SRCS
    DataLoader.h
    DataLoaderFactory.h
    DataLoaderFactory.cpp
    ObjDataLoader.h
    ObjDataLoader.cpp
)

add_library(loader OBJECT ${LOADER_SRCS})

target_compile_options(loader
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(loader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
        ${CMAKE_CURRENT_SOURCE_DIR}/../camera
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${stb_INCLUDE_DIRS}
        ${tinyobjloader_INCLUDE_DIRS}
        ${glm_INCLUDE_DIRS}
)

target_link_libraries(loader
    PUBLIC
        anari::anari
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets)
